# Commonly-shared utility functions

from xml.dom.minidom import Element
import xml.etree.ElementTree as ET

# Conversion Dict:
# tag_name -> (destination_tag, additional_attribues, value_to_attribute)
# OR
# tag_name -> (destingation_tag, additional_attributes), value -> value
# OR
# tag_name -> destination_tag, value -> value
def find_and_convert_parameter (target_node : ET.ElementTree,
    destination_node : ET.Element, conversion_dict : dict):

    tmp = {}
    for key in conversion_dict:
        if type(conversion_dict[key]) == tuple:
            if len(conversion_dict[key]) > 2:
                tmp[key] = conversion_dict[key]
            else:
                tmp[key] = (conversion_dict[key][0], conversion_dict[key][1], "value")
        else:
            tmp[key] = (conversion_dict[key], None, "value")
    conversion_dict = tmp

    for elem in target_node.findall("Parameter"):
        key = elem.get("name")
        if (key in conversion_dict):
            attr = {}
            if conversion_dict[key][1] != None:
                for key2 in conversion_dict[key][1]:
                    attr[key2] = conversion_dict[key][1][key2]
            attr[conversion_dict[key][2]] =  elem.get("value")
            tag = conversion_dict[key][0]
            elem = destination_node.find(tag)
            if elem != None and conversion_dict[key][1] == None: #If it is not none, then the additional attributes provide signifiers
                for key in attr:
                    elem.attrib[key] = attr[key]
            else:
                ET.SubElement(destination_node, tag, attr)

def add_config(target_node : Element):
    ET.SubElement(target_node, "ConfigID", {"value": "0xFFFF"})