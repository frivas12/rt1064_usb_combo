# Stepper APT XML Converter

from gettext import find
import xml.etree.ElementTree as ET
from .util import *

ROOT_TAG = "StageParameters"
def convert(input_xml : ET.ElementTree):
    output_xml = ET.Element(ROOT_TAG)
    root = input_xml.getroot()

    dest_config = ET.SubElement(output_xml, "Config")
    dest_drive = ET.SubElement(output_xml, "Drive")
    dest_flags = ET.SubElement(output_xml, "Flags")
    dest_limit = ET.SubElement(output_xml, "Limit")
    dest_homing = ET.SubElement(output_xml, "Homing")
    dest_jog = ET.SubElement(output_xml, "Jog")
    dest_encoder = ET.SubElement(output_xml, "Encoder")
    dest_pid = ET.SubElement(output_xml, "PID")
    dest_store = ET.SubElement(output_xml, "Store", {"ignore": "1"})



    # Limit Parsing
    source_limit = root.find("LimitParameters")
    if source_limit != None:
        find_and_convert_parameter(source_limit, dest_limit, {
            "cw_hardlimit": ("HardLimit", None, "cw"),
            "ccw_hardlimit": ("HardLimit", None, "ccw"),
            "cw_softlimit": ("SoftLimit", None, "cw"),
            "ccw_softlimit": ("SoftLimit", None, "ccw"),
            "abs_hi_limit": ("AbsLimit", None, "high"),
            "abs_low_limit": ("AbsLimit", None, "low"),
            "limit_mode": "Mode",
        })

    # Homing Parsing
    source_homing = root.find("HomingParameters")
    if source_homing != None:
        find_and_convert_parameter(source_homing, dest_homing,{
            "home_mode": "Mode",
            "home_direction": "Direction",
            "limit_switch": "LimitSwitch",
            "home_velocity": "Velocity",
            "offset_distance": "OffsetDistance",
        })

    # Motor Parsing
    source_motor = root.find("MotorParameters")
    if (source_motor != None):
        dest_kval = ET.SubElement(dest_drive, "KVal")
        dest_slope = ET.SubElement(dest_drive, "Slope")
        find_and_convert_parameter(source_motor, dest_config, {
            "stage_id": "StageID",
            "axis_id": "AxisID",
            "axis_serial_no": "AxisSerial",
            "counts_per_unit": "CountsPerUnit",
            "min_pos": ("Position", None, "min"),
            "max_pos": ("Position", None, "max"),
            "collision_threshold" : "CollisionThreshold",
        })
        cpu = dest_config.find("CountsPerUnit")
        if cpu is not None:
            cpu.attrib["value"] = str(int(cpu.attrib["value"]) / 100000)
        find_and_convert_parameter(source_motor, dest_drive, {
            "max_acc": ("Acceleration", None, "max"),
            "max_dec": ("Acceleration", None, "min"),
            "min_speed": ("Speed", None, "min"),
            "max_speed": ("Speed", None, "max"),
            "fs_spd" : "FullStepSpeed",
            "int_speed": "IntersectSpeed",
            "stall_th" : "StallThreshold",
            "ocd_th": "OCDThreshold",
            "step_mode": "StepMode",
            "config": "ICConfig",
            "gate_config": "GateDriverConfig",
            "stepper_approach_vel" : "ApproachVelocity",
            "stepper_deadband" : "Deadband",
            "stepper_backlash" : "Backlash",
            "kickout_time" : "KickoutTime",
        })
        find_and_convert_parameter(source_motor, dest_kval, {
            "kval_hold": "Hold",
        })
        find_and_convert_parameter(source_motor, dest_slope, {
            "st_slp": "Start",
            "fn_slp_acc" : "FinalAccel",
            "fn_slp_dec" : "FinalDecel",
        })
        find_and_convert_parameter(source_motor, dest_flags, {
            "flags" : "Flags",
        })
        find_and_convert_parameter(source_motor, dest_encoder, {
            "encoder_type" : "EncoderType",
            "index_delta_min" : ("IndexDelta", None, "min"),
            "index_delta_incr" : ("IndexDelta", None, "step"),
            "nm_per_count" : "NmPerCount",
        })

        for elem in source_motor.findall("Parameter"):
            if elem.get("name") == "kval_run":
                if elem.get("offset") == "61":
                    ET.SubElement(dest_kval, "Run", {"value": elem.get("value")})
                elif elem.get("offset") == "62":
                    ET.SubElement(dest_kval, "Accel", {"value": elem.get("value")})
                elif elem.get("offset") == "63":
                    ET.SubElement(dest_kval, "Decel", {"value": elem.get("value")})

    # PID Parsing
    source_pid = root.find("PIDParameters")
    if (source_pid != None):
        find_and_convert_parameter(source_pid, dest_pid, {
            "kp": "Kp",
            "ki": "Ki",
            "kd": "Kd",
            "iMax": "IMax",
            "FilterControl": "FilterControl",
        })

    # Calibratd Positions Parsing
    source_cal_pos = root.find("CalibratedPositions")
    source_cal_pos_deadband = root.find("CalibratedPositionsDeadband")
    if (source_cal_pos != None and source_cal_pos_deadband != None):
        find_and_convert_parameter(source_cal_pos, dest_store, {
            "position_1": ("Position", {"index" : "1"}),
            "position_2": ("Position", {"index" : "2"}),
            "position_3": ("Position", {"index" : "3"}),
            "position_4": ("Position", {"index" : "4"}),
            "position_5": ("Position", {"index" : "5"}),
            "position_6": ("Position", {"index" : "6"}),
        })
        find_and_convert_parameter(source_cal_pos_deadband, dest_store, {
            "deadband": "Deadband",
        })

    # Write default jog
    # TODO: Verify Mode, Velocity, Accel, and StopMode
    # ET.SubElement(dest_jog, "Mode", {"value": "0"}) # Jog Idle
    # ET.SubElement(dest_jog, "StepSize", {"value": "1000"})
    # ET.SubElement(dest_jog, "Velocity", {"value": "0"}) # Unused data
    # ET.SubElement(dest_jog, "Accel", {"value": "0"}) # Unused data
    # ET.SubElement(dest_jog, "StopMode", {"value": "0"}) # Unused data


    return output_xml