# Joystick APT XML Coverter

from gettext import find
import xml.etree.ElementTree as ET
from .util import *

ROOT_TAG = "JoystickParameters"
def convert(input_xml : ET.ElementTree):
    output_xml  = ET.Element("Joystick")
    root = input_xml.getroot()

    for child in root:
        if child.tag == "InMapParameters":
            elem = convert_in(child)
            output_xml.append(elem)

        elif child.tag == "OutMapParameters":
            elem = convert_out(child)
            output_xml.append(elem)

    return output_xml

def convert_in(in_root : ET.Element) -> ET.Element:
    rt = ET.Element("InMap")
    ctrl : int = 0

    target = ET.SubElement(rt, "ModifyControl")
    destination = ET.SubElement(rt, "Destination")

    ctrl_node = in_root.find("Parameter[@name='control_number']")
    if ctrl_node is not None:
        ctrl = int(ctrl_node.get("value"))

    find_and_convert_parameter(in_root, rt, 
    {
        "vid" : "VID",
        "pid" : "PID",
        "modify_speed" : "ModifySpeed",
        "revserse_dir" : "ReverseDir",
        "dead_band" : "Deadband",
        "mode" : "Mode",
    })

    find_and_convert_parameter(in_root, target,{
        "modify_control_port" : "Port",
        "modify_control_ctl_num" : "ControlNum"
    })

    find_and_convert_parameter(in_root, destination, {
        "destination_slot" : "Slot",
        "destination_bit" : "Bit",
        "destination_port" : "Port",
        "destination_virtual" : "Virtual",
    })

    rt.set("control", str(ctrl))
    return rt

def convert_out(out_root : ET.Element) -> ET.Element:
    rt = ET.Element("OutMap")
    ctrl : int = 0

    source = ET.SubElement(rt, "Source")

    ctrl_node = out_root.find("Parameter[@name='control_number']")
    if ctrl_node is not None:
        ctrl = int(ctrl_node.get("value"))

    find_and_convert_parameter(out_root, rt, {
        "HID_USAGE_PAGE_LED" : "Usage",
        "vid" : "VID",
        "pid" : "PID",
        "mode" : "Mode",
        "color_1_id" : ("ColorID", {"index" :"1"}),
        "color_2_id" : ("ColorID", {"index" :"2"}),
        "color_3_id" : ("ColorID", {"index" :"3"}),
    })

    find_and_convert_parameter(out_root, source, {
        "source_slot" : "Slot",
        "source_bit" : "Bit",
        "source_port" : "Port",
        "source_virtual" : "Virtual",
    })

    rt.set("control", str(ctrl))
    return rt