from .util import *
import xml.etree.ElementTree as ET

ROOT_TAG = "ServoParameters"
def convert(input_xml : ET.ElementTree):
    output_xml = ET.Element("StepperParameters")
    root = input_xml.getroot()

    # Add device id with dummy value
    add_config(output_xml)

    dest_servo = ET.SubElement(output_xml, "Servo")
    dest_positions = ET.SubElement(output_xml, "Positions")
    dest_pwm = ET.SubElement(output_xml, "PWM")
    dest_shutter = ET.SubElement(output_xml, "Shutter")

    source_mirror = root.find("Mirror_Parameters")
    if (source_mirror != None):
        find_and_convert_parameter(source_mirror, dest_servo, {
            "TransitTime": "TransitTime"
        })

    source_shutter = root.find("Shutter_Parameters")
    if (source_shutter != None):
        find_and_convert_parameter(source_shutter, dest_shutter, {
            "shutter_initial_state": "InitialState",
            "shutter_mode" : "Mode",
            "external_trigger_mode" : "TriggerMode",
            "on_time" : "OnTime",
            "duty_cycle_pulse" : ("DutyCycle", None, "pulse"),
            "duty_cycle_hold" : ("DutyCycle", None, "hold"),
        })



    return output_xml