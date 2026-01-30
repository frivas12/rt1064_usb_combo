import xml.etree.ElementTree as ElementTree


class struct_key:
    struct_type: int
    struct_index: int

    @staticmethod
    def load(file):
        tree = ElementTree.parse(file)
        return struct_key(
            int(tree.find('./Signature/StructID').attrib['value'], 0),
            int(tree.find('./Signature/ConfigID').attrib['value'], 0)
        )

    def __init__(self, type_value=0, index_value=0):
        self.struct_type = type_value
        self.struct_index = index_value

    def __eq__(self, o):
        return isinstance(o, struct_key) and \
            self.struct_type == o.struct_type and \
            self.struct_index == o.struct_index

    def __lt__(self, o):
        if not isinstance(o, struct_key):
            raise TypeError(f'{type(o)} is not a struct_key')
        return self.struct_type < o.struct_type or \
            self.struct_type == o.struct_type and \
            self.struct_index < o.struct_index

    def __gt__(self, o):
        if not isinstance(o, struct_key):
            raise TypeError(f'{type(o)} is not a struct_key')
        return self.struct_type > o.struct_type or \
            self.struct_type == o.struct_type and \
            self.struct_index > o.struct_index

    def __str__(self):
        return f'{self.struct_type:04X}:{self.struct_index:04X}'

    def __hash__(self):
        return self.uid()

    def uid(self) -> int:
        return self.struct_type << 16 + self.struct_index


class device_key:
    type_id: int
    index: int

    def __init__(self, type_value=0, index_value=0):
        self.type_id = type_value
        self.index = index_value

    def __eq__(self, o):
        return isinstance(o, device_key) and \
            self.type_id == o.type_id and \
            self.index == o.index

    def __lt__(self, o):
        if not isinstance(o, device_key):
            raise TypeError(f'{type(o)} is not a struct_key')
        return self.type_id < o.type_id or \
            self.type_id == o.type_id and \
            self.index < o.index

    def __gt__(self, o):
        if not isinstance(o, device_key):
            raise TypeError(f'{type(o)} is not a struct_key')
        return self.type_id > o.type_id or \
            self.type_id == o.type_id and \
            self.index > o.index

    def __str__(self):
        return f'{self.type_id:04X}:{self.index:04X}'

    def __hash__(self):
        return self.uid()

    def uid(self) -> int:
        return self.type_id << 16 + self.index


class device_path:
    key: device_key = device_key()
    attached_slot: int = 0
    path: str = ''

    @staticmethod
    def load(path):
        tree = ElementTree.parse(path)
        rt = device_path()
        rt.path = path
        rt.key = device_key(
            int(tree.find('./Signature/DefaultSlotType').attrib['value'],
                0),
            int(tree.find('./Signature/DeviceID').attrib['value'],
                0))
        rt.attached_slot = int(
            tree.find('./Signature/RunningSlotType').attrib['value'],
            0)

        return rt

    def __str__(self):
        return f'{self.attached_slot:04X}:{self.key}'

    def struct_keys(self) -> list[struct_key]:
        rt = []
        tree = ElementTree.parse(self.path)
        for sig in tree.findall('./Configs/Signature'):
            element = struct_key()
            element.struct_type = int(
                sig.find('StructID').attrib['value'], 0)
            element.struct_index = int(
                sig.find('ConfigID').attrib['value'], 0)
            rt.append(element)

        return rt
