# A script that generates a one-wire file for a number of input files.
import apt
import re
import xml.etree.ElementTree as ET
import os
import os.path
from splitter import prettify

def print_usage():
    import sys
    print(
        "Usage: python %s [options] <input files>...\n"
        "Options:\n"
        "-d/--delete        Deletes the input files after generating the one-wire file.\n"
        "-t=/--template=    Defines an ordering template for the passed input files.\n"
        "-o=/--output=      Defines the name of the output file.\n"
        "--slot-type=       Defines the slot type of the device.\n"
        "--device-id=       Defines the device id of the device.\n"
        "--part-number=     Defines the part number of the device."
        % sys.argv[0]
    )

class OneWireFilter:
    _struct_id : apt.StructID | None
    _name_regex : re.Pattern | None
    _content_regex : re.Pattern | None

    def __init__(self, struct_id : apt.StructID | None = None,
        name_regex : re.Pattern | None = None,
        content_regex : re.Pattern | None = None):
        self._struct_id = struct_id
        self._name_regex = name_regex
        self._content_regex = content_regex
        if struct_id is None and name_regex is None and content_regex is None:
            raise "Must have at least one filter."

    @staticmethod
    def from_xml(tree : ET.ElementTree) -> dict[int, "OneWireFilter"]:
        out_of_order_list = {}
        root = tree.getroot()
        if root.tag != "Ordering":
            raise "OneWireFilters must be in the \"Ordering\" root tag."
        items = root.findall("./Item")
        last_index = -1
        for item in items:
            index : int
            if "index" in item.attrib:
                index = int(item.attrib["index"])
            else:
                index = last_index + 1
            last_index = index

            struct_id = None
            name_regex = None
            content_regex = None

            struct_id_element = item.find("./StructID")
            name_regex_element = item.find("./NameRegex")
            content_regex_element = item.find("./ContentRegex")

            if struct_id_element is not None:
                struct_id = apt.StructID(int(struct_id_element.attrib["value"]))
            if name_regex_element is not None:
                name_regex = re.compile(name_regex_element.attrib["value"])
            if content_regex_element is not None:
                content_regex = re.compile(content_regex_element.attrib["value"])


            out_of_order_list[index] = OneWireFilter(struct_id, name_regex, content_regex)

        return out_of_order_list


    # Given an input file path, does the XML match the filters?
    def check_xml(self, file_path : str) -> bool:
        rt = True

        file = None

        try:
            if self._name_regex is not None:
                rt = rt and self._name_regex.match(os.path.split(file_path)[1]) is not None

            file = open(file_path, 'r')
            if self._struct_id is not None:
                xml_tree = ET.parse(file)
                sid_node = xml_tree.getroot().find("./Signature/StructID")
                sid_val = apt.StructID(int(sid_node.attrib["value"]))
                rt = rt and sid_val == self._struct_id

            if self._content_regex is not None:
                # I don't know if parsing the file also moves the input head, so I'm forcing
                # it to move back.
                file.seek(0)
                file_content = file.read()
                rt = rt and self._content_regex.search(file_content) is not None
        except:
            rt = False

        if file is not None:
            file.close()
    
        return rt



if __name__ == "__main__":
    import sys
    import getopt

    slot_type = 0xFFFF
    device_id = 0xFFFF
    part_number : str | None = None
    delete_after = False
    template_path = ""
    output_path = "one-wire-output.xml"

    # Parse and Verify arguments.
    try:
        (opts, args) = getopt.getopt(sys.argv[1:], "dt:o:",
            [
                "delete", "template=", "output=", "slot-type=",
                "device-id=", "part-number="
            ])
    except:
        print("Failed to parse inputs")
        print_usage()
        exit(-1)

    if len(args) < 1:
        print_usage()
        exit(-1)

    for (opt, val) in opts:
        if opt == "-d" or opt == "--delete":
            delete_after = True
        elif opt == "-t" or opt == "--template":
            template_path = val
        elif opt == "-o" or opt == "--output":
            output_path = val
        elif opt == "--slot-type":
            try:
                slot_type = int(val)
            except:
                slot_type = int(val, 16)
        elif opt == "--device-id":
            try:
                device_id = int(val)
            except:
                device_id = int(val, 16)
        elif opt == "--part-number":
            part_number = val

    # Load the ordering template (if it exists).
    ordering_template : dict[int, OneWireFilter] | None = None
    if template_path != "":
        try:
            file = open(template_path, 'r')
            tree = ET.parse(file)
            ordering_template = OneWireFilter.from_xml(tree)
            file.close()
        except:
            print("Could not open template file \"%s\"." % template_path)
            exit(-1)

    # Create the output file.

    try:
        out_file = open(output_path, 'w')
    except:
        print("Could not open output file\"%s\"." % output_path)
        exit(-1)

    # Populate the template for the output file.
    out_root = ET.Element("OneWire")
    sig_element = ET.SubElement(out_root, "Signature")
    ET.SubElement(sig_element, "DefaultSlotType", {
        "value": str(slot_type)
    })
    ET.SubElement(sig_element, "DeviceID", {
        "value": str(device_id)
    })
    if part_number is not None:
        ET.SubElement(out_root, "PartNumber", {
            "value": part_number
        })
    device_configs_elem = ET.SubElement(out_root, "DeviceConigurations")
    config_entries_elem = ET.SubElement(out_root, "ConfigEntries")

    # Read in each file.
    
    occupied = ["" for _ in range(0, 15)]
    index_counter : int = 0
    for arg in args:
        try:
            index : int
            allowed = False
            if ordering_template is None:
                # If no ordering template is defined, accept them in order.
                allowed = True
                index = index_counter
            else:
                # If an ordering template is defined, check each ordering filter to see
                # where this files goes (if at all).
                for a_index in ordering_template:
                    if ordering_template[a_index].check_xml(arg):
                        if allowed:
                            print("Input file \"%s\" matched against two order indexes, %d and %d" %
                                (arg, index, a_index)
                            )
                            exit(-1)
                        else:
                            allowed = True
                            index = a_index
            if allowed:
                if index >= 15:
                    print("Too many input files.")
                    exit(-1)
                if occupied[index] != "":
                    print("Input file \"%s\" and \"%s\" collide at index %d." % (arg, occupied[index],
                    index))
                    exit(-1)
                index_counter = index + 1
                occupied[index] = arg
                try:
                    # Change the config id and append it
                    tree = ET.parse(arg)
                    conf = tree.find("./Signature/ConfigID")
                    conf.attrib["value"] = str(0xFFF0 + index)
                    tree.getroot().attrib["binary"] = "$PATH:bin/%d-%d-%04x.bin" % (slot_type, device_id, abs(hash(arg)))
                    config_entries_elem.append(tree.getroot())
                except:
                    print("Failed to open and modify struct tree in \"%s\"." % arg)
                    exit(-1)
                
        except:
            print("Failed to parse input file \"%s\"." % arg)
            exit(-1)

    # Close the output file.
    try:
        out_tree = ET.ElementTree(out_root)
        out_file.write(prettify(out_tree.getroot()))
    except:
        print("Could not write generated XML to file.")
        exit(-1)
    out_file.close()

    # Delete the input files (if declared).
    if delete_after:
        for obj in occupied:
            if obj != "":
                os.remove(obj)