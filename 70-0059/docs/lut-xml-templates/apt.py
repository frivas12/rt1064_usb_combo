from enum import Enum

from sympy import sturm

class StructID (Enum):
    ENCODER = 0
    
    LIMIT = 1

    STEPPER_CONFIG = 2
    STEPPER_DRIVE = 3
    STEPPER_FLAGS = 4
    STEPPER_HOME = 5
    STEPPER_JOG = 6
    STEPPER_PID = 7
    STEPPER_STORE = 8

    INVALID = 0xFFFF

    def __str__(self):
        if self == StructID.ENCODER:
            return "Encoder"
        elif self == StructID.LIMIT:
            return "Limit"
        elif self == StructID.STEPPER_CONFIG:
            return "Config"
        elif self == StructID.STEPPER_DRIVE:
            return "Drive"
        elif self == StructID.STEPPER_FLAGS:
            return "Flags"
        elif self == StructID.STEPPER_HOME:
            return "Homing"
        elif self == StructID.STEPPER_JOG:
            return "Jog"
        elif self == StructID.STEPPER_PID:
            return "PID"
        elif self == StructID.STEPPER_STORE:
            return "Store"
        else:
            return self.name

class CardID (Enum):
    NO_CARD_IN_SLOT = 0
    CARD_IN_SLOT_NOT_PROGRAMMED = 1
    ST_Stepper_type = 2
    High_Current_Stepper_Card = 3
    Servo_type = 4
    Shutter_type = 5
    OTM_Dac_type = 6
    OTM_RS232_type = 7
    High_Current_Stepper_Card_HD = 8
    Slider_IO_type = 9
    Shutter_4_type = 10
    Piezo_Elliptec_type = 11
    ST_Invert_Stepper_BISS_type = 12
    ST_Invert_Stepper_SSI_type = 13
    Piezo_type = 14
    MCM_Stepper_Internal_BISS_L6470 = 15
    MCM_Stepper_Internal_SSI_L6470 = 16
    MCM_Stepper_L6470_MicroDB15 = 17
    Shutter_4_type_REV6 = 18
    MCM_Stepper_LC_HD_DB15 = 19

class device_signature:
    slot_type : CardID
    device_id : int

    def __init__(self, slot_type : CardID, device_id : int):
        self.slot_type = slot_type
        self.device_id = device_id

    def __str__(self):
        return "<%d,%d>" % (self.slot_type.value, self.device_id)

class config_signature:
    struct_id : StructID
    config_id : int

    def __init__(self, struct_id : StructID, config_id : int):
        self.struct_id = struct_id
        self.config_id = config_id

    def __str__(self):
        return "<%d,%d>" % (self.struct_id.value, self.config_id)

# EOF