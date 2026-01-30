# Automation tool that unites separate config xmls into a single xml using induction.
# It requires one device instance file (i.e. slot-slot-id) and all of the structures used.
# Usage: python joiner [--output=<Output File>] <Input files>...

import getopt
import sys
import xml.etree.ElementTree as ET
from splitter import prettify
import os.path
from apt import StructID, CardID

try:
    (opts, args) = getopt.getopt(sys.argv, "", ["output="])
except:
    (opts, args) = ([], [])

# Filter out the first element (the python script's name)
if len(args) > 0:
    args = [args[i] for i in range(1, len(args))]

output_path : str = "./out.xml"
card_filter = ""

# Process arguments
for opt, param in opts:
    if opt == "--output":
        output_path = param
    if opt == "--card":
        card_filer = param

out_xml = ET.ElementTree()

class PreprocessingContext:
    path : str = "."

def preprocess_includes(node : ET.Element, context : PreprocessingContext):
    to_swap : list[(ET.Element, ET.Element)] = []

    for child in node:
        if child.tag == "Include" or child.tag == "Import":
            file = os.path.join(context.path, child.attrib["href"])
            file_cwd = os.path.split(file)[0]

            f = open(file, 'rb')

            tree = ET.parse(f)

            f.close()

            new_context = context
            new_context.path = file_cwd
            preprocess_node(tree.getroot(), new_context)

            new_node : ET.Element

            if child.tag == "Import":
                new_node = tree.find(child.attrib["xpath"])
            else:
                new_node = tree.getroot()

            to_swap.append((child, new_node))
        else:
            preprocess_includes(child, context)

    for old_node, new_node in to_swap:
        node.remove(old_node)

    for old_node, new_node in to_swap:
        node.append(new_node)

def preprocess_variables(node : ET.Element, context : PreprocessingContext) -> PreprocessingContext:
    for attribute, value in node.attrib.items():
        if len(value) > 0 and value[0] == "$":
            if len(value) > 6 and value[1:6] == "PATH:":
                # Path Operator
                node.attrib[attribute] = os.path.join(context.path, value[6:])
            elif len(value) > 1 and value[1] == '$':
                # Dollar Sign Constant
                node.attrib[attribute] = value[1:]
    
    for child in node:
        context = preprocess_variables(child, context)

    return context

def preprocess_node (node : ET.Element, context : PreprocessingContext):
    context = preprocess_variables(node, context)
    preprocess_includes(node, context)

device_sig = ("-1", "-1")
needed_configs = []
configs = []

# Load in XML files
for arg in args:
    try:
        f = open(arg, 'rb')
    except:
        f = None
    if f != None and not f.closed:
        input_xml = ET.parse(f)
        
        context = PreprocessingContext()
        context.path = os.path.split(arg)[0]
        preprocess_node(input_xml.getroot(), context)

        root = input_xml.getroot()

        if root.tag == "Device":
            dst = root.find("Signature/DefaultSlotType").attrib["value"]
            did = root.find("Signature/DeviceID").attrib["value"]
            for elem in root.findall("Card"):
                if card_filter == "" or elem.find("Type").attrib["value"] == card_filer:
                    for c in elem.findall("Configs/Signature"):
                        sid = c.find("StructID").attrib["value"]
                        cid = c.find("ConfigID").attrib["value"]
                        needed_configs.append((sid, cid))
                    break

            device_sig = (dst, did)

        elif root.tag == "Struct":
            struct_id = root.find("Signature/StructID").attrib["value"]
            config_id = root.find("Signature/ConfigID").attrib["value"]
            data = elem.find("Data")

            configs.append((struct_id, config_id, data))

        elif root.tag == "OneWire":
            if device_sig == ("-1", "-1"):
                print("One-wire xmls should come after device xml.")
                exit(-3)
            dst = root.find("Signature/DefaultSlotType").attrib["value"]
            did = root.find("Signature/DeviceID").attrib["value"]

            if (dst, did) != device_sig:
                print("Mismatch between one-wire signature and device file.")
                exit(-2)
            for elem in root.findall("ConfigEntries/Struct"):
                struct_id = elem.find("Signature/StructID").attrib["value"]
                config_id = elem.find("Signature/ConfigID").attrib["value"]
                data = elem.find("Data")

                configs.append((struct_id, config_id,data))

        f.close()

# Verify that no collisions have occurred the in the configs list
verification_set : set[tuple[int, int]] = set()
for tup in configs:
    sig = (tup[0], tup[1])
    if sig in verification_set:
        print("Duplicate config signatures found for (0x" + tup[0] + ",0x" + tup[1] + ").")
        exit(-1)
    verification_set.add(sig)
verification_set = None # Clear up the set

# TODO: Verify the sgnatu

# Verify that everything needed has been acquired
verify_counter = 0
for config in configs:
    if (config[0], config[1]) in needed_configs:
        verify_counter += 1

if verify_counter != len(needed_configs):
    print("Missing some files.")
    exit(1)

if device_sig == ("-1", "-1"):
    print("Did not find the device to unite.")
    exit(2)

# Generate the whole XML

tag = "Config"

if CardID(int(device_sig[0])) == CardID.ST_Stepper_type or \
CardID(int(device_sig[0])) == CardID.MCM_Stepper_Internal_BISS_L6470 or \
CardID(int(device_sig[0])) == CardID.MCM_Stepper_Internal_SSI_L6470 or \
CardID(int(device_sig[0])) == CardID.MCM_Stepper_L6470_MicroDB15 or \
CardID(int(device_sig[0])) == CardID.ST_Invert_Stepper_BISS_type or \
CardID(int(device_sig[0])) == CardID.ST_Invert_Stepper_SSI_type or \
CardID(int(device_sig[0])) == CardID.High_Current_Stepper_Card or \
CardID(int(device_sig[0])) == CardID.High_Current_Stepper_Card_HD or \
CardID(int(device_sig[0])) == CardID.MCM_Stepper_LC_HD_DB15:
    tag = "StageParameters"

xml_top = ET.Element(tag)
signature = ET.SubElement(xml_top, "Signature")
ET.SubElement(signature, "DefaultSlotType", {"value": device_sig[0]})
ET.SubElement(signature, "DeviceID", {"value": device_sig[1]})

def converter (id : StructID) -> str:
    return str(id)

for (sid, cid, data) in configs:
    if (sid, cid) in needed_configs:
        obj = ET.SubElement(xml_top, converter(StructID(int(sid))))
        # Clone data element to new xml.
        obj.append(data)

# Write the whole XML to a file
try:
    out_file = open(output_path, 'w')
except:
    print("Failed to open/create file at path " + output_path + ".")
    exit(3)

# Make the output pretty.
out_file.write(prettify(xml_top))

out_file.close()
