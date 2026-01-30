import dbapi

with dbapi.api('./build/global.db') as api:
    print('ID (hex)  | Description')
    print('--------- | -----------')
    devices = api.get_devices()
    devices.sort(key=lambda x: x.key)
    for device in devices:
        print(f'{device.key} | {device.description}')
