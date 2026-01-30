import tomllib
import semver

from typing import Optional

import xmlapi


class device_request:
    key = xmlapi.device_key()
    filter: Optional[list[int]] = None

    def __init__(self,
                 key: xmlapi.device_key,
                 filter: Optional[list[int]] = None):
        self.key = key
        self.filter = filter


class config:
    name: str
    version: semver.Version
    devices: list[device_request]

    def __init__(self):
        self.name = ''
        self.version = semver.Version(1)
        self.devices = []


def load(file, *kargs, **kwargs) -> config:
    rt = config()
    cfg = tomllib.load(file, *kargs, **kwargs)

    if 'name' not in cfg:
        raise KeyError('missing key "name"')
    if 'version' not in cfg:
        raise KeyError('missing key "version"')
    if 'devices' not in cfg:
        raise KeyError('missing key "devices"')

    if not isinstance(cfg['devices'], list):
        raise TypeError('key "devices" is not a list')

    rt.name = cfg['name']
    rt.version = semver.Version.parse(cfg['version'])

    for element in cfg['devices']:
        if not isinstance(element, dict):
            raise TypeError('key "devices" is not a list of tables')
        if 'slot_type' not in element:
            raise KeyError(
                'element in key "devices" missing subkey "slot_type"')
        if 'device_id' not in element:
            raise KeyError(
                'element in key "devices" missing subkey "device_id"')
        filter = None
        if 'slot_filter' in element:
            filter = element['slot_filter']
            if len(filter) == 0:
                raise RuntimeWarning('empty slot filter imported')
            if not all(map(lambda x: isinstance(x, int), filter)):
                raise TypeError(
                    'element in "devices.filter" was not an integer')
        rt.devices.append(device_request(
            xmlapi.device_key(element['slot_type'],
                              element['device_id']),
            filter))

    return rt
