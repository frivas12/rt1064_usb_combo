# Automation tool that converts a previous APT command configuration into a LUT Entry.
# Usage: python converter.py <APT XML Path> [Output XML Path]
# The default output is in the current directory.
from email import encoders
from sys import argv
import os
import xml.etree.ElementTree as ET
import apt

from sympy import true
from conversion_modules import *
from xml.dom import minidom
import re

def get_usage_str():
    return "Usage: python splitter.py <APT XML Path> [Directory Path]"

# Thank you to Jedani for this code!
def prettify(elem):
     """
         Return a pretty-printed XML string for the Element.
     """
     rough_string = ET.tostring(elem, 'utf-8')
     reparsed = minidom.parseString(rough_string)
     uglyXml = reparsed.toprettyxml(indent="    ")
     pattern = re.compile('>\n\s+([^<>\s].*?)\n\s+</', re.DOTALL)
     str_l = pattern.sub('>\g<1></', uglyXml).split('\n')
     str_c = []
     for l in str_l:
        if l.strip() != "":
            str_c.append(l)
     return "\n".join(str_c)

def split_full_config(input_file_path, output_file_path):
    input_xml = None
    input_file = None
    try:
        input_file = open(input_file_path, 'rb')
        input_xml = ET.parse(input_file)
    except:
        print("Could not open file %s." % input_file_path)
        return

    count = 0
    for elem in input_xml.getroot():
        output_file = None
        path = os.path.join(output_file_path, os.path.basename(os.path.splitext(input_file_path)[0]) + "_" + elem.tag + ".xml")
        try:
            output_file = open(path, 'w')
        except:
            print("Could not open file %s." % path)
            input_file.close()
            return
        conf = ET.Element("Struct")
        sig = ET.SubElement(conf, "Signature")
        data = ET.SubElement(conf, "Data")
        sid = "0xFFFF"
        try:
            for enum in apt.StructID:
                if str(enum) == elem.tag:
                    sid = str(enum.value)
        except:
            pass
        ET.SubElement(sig, "StructID", {"value": sid})
        ET.SubElement(sig, "ConfigID", {"value": "0xFFFF"})
        for sub in elem:
            data.append(sub)
        output_file.write(prettify(conf))

        output_file.close()
        count += 1

    out_file = None
    path = os.path.join(output_file_path, os.path.basename(os.path.splitext(input_file_path)[0]) + "_Device.xml")
    try:
        out_file = open(path, 'w')
    except:
        print("Could not open file %s." % path)
        input_file.close()
        return

    dev = ET.Element("Device")
    sig = ET.SubElement(dev, "Signature")
    configs = ET.SubElement(dev, "Configs")
    ET.SubElement(sig, "SlotCard", {"value": "0xFFFF"})
    ET.SubElement(sig, "DeviceID", {"value": "0xFFFF"})

    for _ in range(count):
        base = ET.SubElement(configs, "Signature")
        ET.SubElement(base, "StructID", {"value": "0xFFFF"})
        ET.SubElement(base, "ConfigID", {"value": "0xFFFF"})


    out_file.write(prettify(dev))

    out_file.close()




# BEGIN     Run-Time Code
if __name__ == "__main__":
    if (len(argv) < 4 and len(argv) > 1):
        input_file_path = argv[1]
        output_file_path = None
        if (len(argv) == 3):
            output_file_path = argv[2]
        else:
            output_file_path = os.path.splitext(os.path.basename(input_file_path))[0]

        made_dir = False
        try:
            os.mkdir(output_file_path)
            made_dir = True
        except:
            print("Could not create output directory %s." % output_file_path)
        
        if (made_dir):
            split_full_config(input_file_path, output_file_path)

    else:
        print(get_usage_str())
# END       Run-Time Code
# EOF
