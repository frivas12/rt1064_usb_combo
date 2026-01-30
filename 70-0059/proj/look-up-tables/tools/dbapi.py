import os.path
import pathlib
import sqlite3

from typing import Optional

import xmlapi


class device:
    key: xmlapi.device_key = xmlapi.device_key()
    description: str = ''
    is_active: bool = False


class device_usage:
    part_number: str = ''
    index: int = 0
    key: xmlapi.device_key = xmlapi.device_key()
    is_active: bool = False
    description: str = ''


class api:
    def __init__(self, dbpath: str):
        luri = pathlib.Path(os.path.abspath(dbpath)).as_uri()
        self._connection = sqlite3.connect(
            str(luri), uri=True, autocommit=False)

    def __enter__(self):
        return self

    def __exit__(self, _1, _2, _3):
        del self._connection

    def get_device(self, key: xmlapi.device_key) -> device | None:
        cur = self._connection.cursor()
        cur.execute("SELECT SlotType, DeviceID, Description, "
                    "IsActive FROM Devices WHERE "
                    f"SlotType={key.type_id} AND DeviceID={key.index};")

        row = cur.fetchone()

        if row is None:
            return None
        else:
            return api._parse_get_device_row(row)

    def get_devices(self) -> list[device]:
        rt = []
        cur = self._connection.cursor()
        cur.execute("SELECT SlotType, DeviceID, Description, "
                    "IsActive FROM Devices;")

        for row in cur:
            rt.append(api._parse_get_device_row(row))

        return rt

    def get_device_path(self, key: xmlapi.device_key,
                        attached_slot: Optional[int] = None) -> \
            xmlapi.device_path | None:
        if attached_slot is None:
            attached_slot = key.type_id
        cur = self._connection.cursor()
        cur.execute("SELECT SlotType, DeviceID, AttachedSlot, Path "
                    f"FROM DeviceFiles WHERE SlotType={key.type_id} AND "
                    f"DeviceID={key.index} AND "
                    f"AttachedSlot={attached_slot};")
        row = cur.fetchone()
        if row is None:
            return None
        else:
            return api._parse_get_device_path_row()

    def get_device_paths(self, key: xmlapi.device_key) -> \
            list[xmlapi.device_path]:
        cur = self._connection.cursor()
        cur.execute("SELECT SlotType, DeviceID, AttachedSlot, Path "
                    f"FROM DeviceFiles WHERE SlotType={key.type_id} AND "
                    f"DeviceID={key.index};")

        return [api._parse_get_device_path_row(row) for row in cur]

    def get_all_device_paths(self) -> \
            dict[xmlapi.device_key, list[xmlapi.device_path]]:
        cur = self._connection.cursor()
        cur.execute("SELECT SlotType, DeviceID, AttachedSlot, Path "
                    "FROM DeviceFiles;")
        rt = {}
        for row in cur:
            entry = api._parse_get_device_path_row(row)
            if entry in rt:
                rt[entry].append(entry)
            else:
                rt[entry] = [entry]
        return rt

    def get_usages_by_device(self, key: xmlapi.device_key) -> \
            list[device_usage]:
        cur = self._connection.cursor()
        cur.execute("SELECT PartNumber, UsageID, SlotType, "
                    "DeviceID, IsActive, UsageDescription from "
                    f"PartNumbers WHERE SlotType={key.type_id} AND "
                    f"DeviceID={key.index};")

        return list(api._parse_get_usages(cur))

    def get_usages_by_part_number(self, pn: str) -> list[device_usage]:
        cur = self._connection.cursor()
        cur.execute("SELECT PartNumber, UsageID, SlotType, "
                    "DeviceID, IsActive, UsageDescription from "
                    f"PartNumbers WHERE PartNumber={pn};")

        return list(api._parse_get_usages(cur))

    def get_usages(self) -> dict[str, list[device_usage]]:
        rt = {}
        cur = self._connection.cursor()
        cur.execute("SELECT PartNumber, UsageID, SlotType, "
                    "DeviceID, IsActive, UsageDescription from "
                    "PartNumbers;")

        for entry in api._parse_get_usages(cur):
            if entry.part_number in rt:
                rt[entry.part_number].append(entry)
            else:
                rt[entry.part_number] = [entry]
        return rt

    def get_struct_keys(self, obj: device | xmlapi.device_path) -> \
            list[xmlapi.struct_key]:
        def impl_device_key(o: xmlapi.device_key) -> list[xmlapi.struct_key]:
            s = set()
            for pathobj in self.get_device_paths(o):
                for path in pathobj.struct_keys():
                    s.add(path)
            return [x for x in s]

        if isinstance(obj, xmlapi.device_path):
            return obj.struct_keys()
        elif isinstance(obj, xmlapi.device_key):
            return impl_device_key(obj)
        elif isinstance(obj, device):
            return impl_device_key(obj.key)
        else:
            raise TypeError()

    @staticmethod
    def _parse_get_device_row(row) -> device:
        entry = device()
        entry.key = xmlapi.device_key(row[0], row[1])
        entry.description = row[2] is not None and row[2] or ''
        entry.is_active = row[3] != 0
        return entry

    @staticmethod
    def _parse_get_device_path_row(row) -> xmlapi.device_path:
        rt = xmlapi.device_path()
        rt.key = xmlapi.device_key(row[0], row[1])
        rt.attached_slot = row[2]
        rt.path = row[3]
        return rt

    @staticmethod
    def _parse_get_usages(cursor):
        for row in cursor:
            entry = device_usage()
            entry.part_number = row[0]
            entry.index = row[1]
            entry.key = xmlapi.device_key(row[2], row[3])
            entry.is_active = row[4] != 0
            entry.description = row[5] is not None and row[5] or 'Default'
            yield entry
