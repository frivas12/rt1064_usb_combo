import sys
import traceback

import dbapi
import tomlapi
import xmlapi


# Usage: <toml-file text> | python config1_builder.py

def print_warning(message: str):
    print("warning: " + message, file=sys.stderr)


def print_fatal(message: str, error_code: int = -1):
    print(f"error ({error_code}): " + message, file=sys.stderr)
    sys.exit(error_code)


try:
    if len(sys.argv) != 1:
        print('usage: python config1_builder.py', file=sys.stderr)
        sys.exit(1)

    devices: dict[xmlapi.device_key, dbapi.device | None] = {}
    filters: dict[xmlapi.device_key, list[int]] = {}
    device_paths: set[str] = set()

    cfg = tomlapi.load(sys.stdin.buffer)

    for device in cfg.devices:
        devices[device.key] = None
        if device.filter is not None:
            filters[device.key] = device.filter

    with dbapi.api('./build/global.db') as api:
        for key in devices:
            devices[key] = api.get_device(key)
            if devices[key] is None:
                print_fatal(
                    f'device key "{key}" was not found in the database')
            if not devices[key].is_active:
                print_warning(f'device "{key}" is not active')

            paths: list[xmlapi.device_path]
            if key in filters:
                paths = []
                for attached_slot in filters[key]:
                    paths.extend(api.get_device_path(key, attached_slot))
            else:
                paths = api.get_device_paths(key)
            if len(paths) == 0:
                print_warning(f'device "{key}" has no file paths')
            for pathobj in paths:
                device_paths.add(pathobj.path)

    print(f'PACKAGE_NAME := {cfg.name}')
    print(f'VERSION := {cfg.version}')
    print('DEVICE_SRCS := \\')
    for path in device_paths:
        if path.startswith('./'):
            path = path[2:]
        print(f'\t{path} \\')
    print()

except Exception:
    print_fatal(f"an unexpected error occurred\n{traceback.format_exc()}")
