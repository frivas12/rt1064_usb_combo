# Automation tool that converts a previous APT command configuration into a LUT Entry.
# Usage: python converter.py <APT XML Path> [Output XML Path]
# The default output is in the current directory.
from sys import argv
import os
import xml.etree.ElementTree as ET
from conversion_modules import *
from conversion_modules import joystick

def get_usage_str():
    return "Usage: python converter.py <APT XML Path> [Output XML Path]"

# Thanks to Erick M. Sprengel for this spacing code!
def indent(elem, level=0):
    i = "\n" + level*"  "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "  "
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
        for elem in elem:
            indent(elem, level+1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i

def convert_apt_to_lut(input_file, output_file):
    input_xml = ET.parse(input_file)
    output_xml = None

    if (input_xml != None):
        tag = input_xml.getroot().tag
        if tag == stepper.ROOT_TAG:
            output_xml = stepper.convert(input_xml)
        elif tag == servo.ROOT_TAG:
            output_xml = servo.convert(input_xml)
        elif tag == joystick.ROOT_TAG:
            output_xml = joystick.convert(input_xml)
        else:
            print("Tag Type \"%s\" is not recognized." % tag)

        if (output_xml != None):
            tree = ET.ElementTree(output_xml)
            indent(output_xml)
            tree.write(output_file)
    else:
        print("Input file could not be parsed as an XML.")



# BEGIN     Run-Time Code
if (len(argv) < 4 and len(argv) > 1):
    input_file_path = argv[1]
    output_file_path = None
    if (len(argv) == 3):
        output_file_path = argv[2]
    else:
        output_file_path = os.path.splitext(os.path.basename(input_file_path))[0] + ".xml"

    input_file = None
    try:
        input_file = open(input_file_path, 'rb')
    except:
        print("Could not open file %s." % input_file_path)
    if (input_file != None):
        output_file = None
        try:
            output_file = open(output_file_path, 'wb')
        except:
            print("Could not create output file %s." % output_file_path)
        
        if (output_file != None):
            convert_apt_to_lut(input_file, output_file)

            output_file.close()


        input_file.close()

else:
    print(get_usage_str())
# END       Run-Time Code
# EOF
