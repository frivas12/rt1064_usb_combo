import serial;
import struct;
import datetime;
import time;

class pnp_mcm_controller:
    _port : serial.Serial = None
    _time_of_next_send : datetime.datetime = datetime.datetime.now()
    timeout_period : datetime.timedelta = datetime.timedelta(milliseconds=10)

    def _safe_read(self, num) -> bytes:
        try:
            return self._port.read(num)
        except serial.SerialException:
            return bytes()
        except TypeError:
            self.close()
            raise

    def _receive_command(self) -> bytes:
        rt = self._safe_read(6)
        if rt[4] & 0x80 > 0:
            arr = bytearray(rt)
            extended_length = int(rt[2]) | (int(rt[3]) << 8)
            extended = self._safe_read(extended_length)

            arr.extend(extended)
            rt = bytes(arr)

        return rt

    def _send(self, data):
        to_wait = self._time_of_next_send - datetime.datetime.now()
        if to_wait > datetime.timedelta(seconds=0):
            time.sleep(to_wait.total_seconds())
            

        buffer = data
        to_send = len(data)
        while to_send > 0:
            try:
                written = self._port.write(buffer)
            except serial.SerialException:
                self.close()
                raise
            to_send -= written
            buffer = buffer[written:]
        self._time_of_last_send = datetime.datetime.now()
        self._time_of_next_send = self._time_of_last_send + self.timeout_period


    @staticmethod
    def _build_command(command : int, slot : int | None, param1 : int, param2 : int) -> bytes:
        rt = bytearray(6)
        rt[0] = command & 0xFF
        rt[1] = (command >> 8) & 0xFF
        rt[2] = param1
        rt[3] = param2
        rt[4] = (slot is not None) and (0x20 + slot) or 0x11
        rt[5] = 0x01

        return bytes(rt)

    @staticmethod
    def _build_extended_command(command : int, slot : int | None, extended_data : bytearray) -> bytes:
        rt = bytearray(6)
        rt[0] = command & 0xFF
        rt[1] = (command >> 8) & 0xFF
        rt[2] = (len(extended_data) & 0xFF)
        rt[3] = ((len(extended_data) >> 8) & 0xFF)
        rt[4] = (slot is not None) and (0xA0 + slot) or 0x91
        rt[5] = 0x01
        rt.extend(extended_data)

        return bytes(rt)
        

    def is_open(self) -> bool:
        return self._port.is_open

    def close(self):
        self._port.close()

    def __init__(self, port : serial.Serial):
        self._port = port

    def is_device_connected(self, slot : int) -> bool:
        tx = self._build_command(0x4006, None, slot - 1, 0)
        self._send(tx)
        rx = self._receive_command()
        return rx[36] == 1

    def is_cable_connected(self, slot : int) -> bool:
        return self.get_pnp_status(slot) & 0x01 == 0

    # Gets the name of the stage.
    def get_device_name(self, slot : int) -> str:
        tx = self._build_command(0x4006, None, slot - 1, 0)
        self._send(tx)
        rx = self._receive_command()
        return rx[20:36].decode('ascii').replace('\0', '')

    # Gets the position of the stage in encoder counts.
    def get_position(self, slot : int) -> int:
        tx = self._build_command(0x0480, slot, 0, 0)
        self._send(tx)
        rx = self._receive_command()
        return struct.unpack('<i', rx[12:16])[0]

    def set_position(self, slot : int, position : int):
        extended = bytearray()
        extended.extend((slot - 1).to_bytes(2, 'little'))
        extended.extend(position.to_bytes(4, 'little'))
        tx = self._build_extended_command(0x0409, slot, extended)
        self._send(tx)

    # Sets the position of the stage in encoder counts.
    def move_to(self, slot : int, position : int):
        extended = bytearray()
        extended.extend((slot - 1).to_bytes(2, 'little'))
        extended.extend(position.to_bytes(4, 'little'))
        tx = self._build_extended_command(0x0453, slot, extended)
        self._send(tx)

    def get_nm_per_encoder_count(self, slot : int) -> float:
        tx = self._build_command(0x4042, slot, slot - 1, 0)
        self._send(tx)
        rx = self._receive_command()
        return struct.unpack('f', rx[74:78])[0]


    # Gets if a stage is homed.
    def is_homed(self, slot : int) -> bool:
        tx = self._build_command(0x0480, slot, 0, 0)
        self._send(tx)
        rx = self._receive_command()
        return (struct.unpack('<i', rx[16:20])[0] & 0x400) > 0

    # Homes a stage.
    def home(self, slot : int):
        tx = self._build_command(0x0443, slot, 0, 0)
        self._send(tx)

    def is_moving(self, slot : int) -> bool:
        tx = self._build_command(0x0480, slot, 0, 0)
        self._send(tx)
        rx = self._receive_command()
        return (struct.unpack('<i', rx[16:20])[0] & 0x30) > 0

    def is_one_wire_programming(self, slot : int) -> bool:
        extended = (slot - 1).to_bytes(2, 'little')
        tx = self._build_extended_command(0x40ED, None, extended)
        self._send(tx)
        rx = self._receive_command()
        return rx[8] == 3

    def set_one_wire_programming(self, slot : int, is_programming : bool) -> bool:
        extended = bytearray()
        extended.extend((slot - 1).to_bytes(2, 'little'))
        byte_cast = is_programming and b'\x01' or b'\x00'
        extended.extend(byte_cast)
        tx = self._build_extended_command(0x40EC, None, extended)
        self._send(tx)
        rx = self._receive_command()
        return rx[8] == 0

    def get_one_wire_chunk_size(self) -> int:
        tx = self._build_command(0x40F0, None, 0, 0)
        self._send(tx)
        rx = self._receive_command()
        return struct.unpack('<H', rx[6:8])[0]


    @staticmethod
    def parse_one_wire_code(status : int) -> str:
        if status == 0:
            return "No error"
        elif status == 1:
            return 'Device not detected'
        elif status == 2:
            return 'Device does not support one-wire'
        elif status == 4:
            return 'Device is not programming'
        elif status == 5:
            return 'Too much memory for the device to handle'
        elif status == 6:
            return 'Packet was too large to process'

    def program_one_wire(self, slot : int, data : bytearray) -> int:
        extended = bytearray()
        extended.extend((slot - 1).to_bytes(2, 'little'))
        extended.extend(data)
        tx = self._build_extended_command(0x40EF, None, extended)
        self._send(tx)
        rx = self._receive_command()
        return rx[8]

    @staticmethod
    def parse_pnp_status(status : int) -> str:
        if status == 1:
            return 'No device is connected'
        else:
            return 'Unknown status'

    @staticmethod
    def test_port_is_valid(port : serial.Serial) -> bool:
        try:
            port.timeout = 1
            port.write(pnp_mcm_controller._build_command(0x0005, None, 0, 0))
            rt = port.read(6)
            if len(rt) == 6 and rt[4] & 0x80 > 0:
                extended_length = int(rt[2]) | (int(rt[3]) << 8)
                port.timeout = 3
                extended = port.read(extended_length)

                if len(extended) >= 12:
                    model_no = extended[4:12].decode().replace('\0','')

                    ACCEPTED_MODELS = [
                        "MCM301"
                    ]

                    return model_no in ACCEPTED_MODELS
        except:
            pass
        return False

    def get_pnp_status(self, slot : int) -> int:
        tx = self._build_command(0x4108, None, slot - 1, 0)
        self._send(tx)
        rx = self._receive_command()
        return struct.unpack('<I', rx[8:12])[0]
