import sys
import os.path
import traceback

import dbapi
import tomlapi
import xmlapi


# Usage: <toml-file-text> | python config2_builder.py

def print_warning(message: str):
    print("warning: " + message, file=sys.stderr)


def print_fatal(message: str, error_code: int = -1):
    print(f"error ({error_code}): " + message, file=sys.stderr)
    sys.exit(error_code)


try:
    if len(sys.argv) != 1:
        print('usage: python config2_builder.py', file=sys.stderr)
        sys.exit(1)

    devices: dict[xmlapi.device_key, dbapi.device | None] = {}
    configs: set[xmlapi.struct_key] = set()

    cfg = tomlapi.load(sys.stdin.buffer)

    for device in cfg.devices:
        devices[device.key] = None

    with dbapi.api('./build/global.db') as api:
        for key in devices:
            devices[key] = api.get_device(key)
            if devices[key] is None:
                print_fatal(
                    f'device key "{key}" was not found in the database')
            if not devices[key].is_active:
                print_warning(f'device "{key}" is not active')
            for x in filter(lambda x: x.struct_index < 0xFFF0,
                            api.get_struct_keys(devices[key])):
                configs.add(x)

    config_paths: set[str] = set()
    for key in configs:
        file_name = f'{key.struct_type}-{key.struct_index}.xml'
        path = os.path.join('config/', file_name)
        if not os.path.exists(path):
            print_fatal(f'struct XML "{file_name}" was not found')
        config_paths.add(path)

    print('CONFIG_SRCS := \\')
    for path in config_paths:
        print(f'\t{path} \\')
    print()

except Exception:
    print_fatal(f"an unexpected error occurred\n{traceback.format_exc()}")
