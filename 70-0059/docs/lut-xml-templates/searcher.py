# Python file that scans the config DB for matching configs.
from typing import Dict
import pyodbc
from apt import config_signature, StructID
import xml.etree.ElementTree as ET
import db_structure

class xml_db_match:
    file_path : str
    signature : config_signature
    description : str | None

    def __init__(self, file_path, signature, description):
        self.file_path = file_path
        self.signature = signature
        self.description = description

    def __str__(self):
        appended_str : str
        if self.description is None:
            appended_str = ""
        else:
            appended_str = ": %s" % self.description
        return "%s -> %s%s" % (self.file_path, str(self.signature), appended_str)

def get_xpaths_of_children(elem : ET.Element, base_path="/") -> list[str]:
    rt = []
    for child in elem:
        xpath = base_path + child.tag
        child_list = get_xpaths_of_children(child, xpath + "/")
        rt.append(xpath)
        rt.extend(child_list)
    return rt

def get_attributed_paths(tree : ET.ElementTree, xpaths : list[str]):
    rt : list[tuple[str, str, str | None | None]]= []
    for path in xpaths:
        elem = tree.find(path)
        for attr in elem.attrib:
            val = elem.attrib[attr]
            rt.append((path, val, attr != "value" and attr or None))
    return rt

# Returns data on a db match or None if no match was found.
def find_db_match(file_path : str, db_configs : Dict[StructID, str],
    cursor : pyodbc.Cursor) -> xml_db_match | None:
    try:
        tree = ET.parse(file_path)

        sid = StructID(int(tree.find("./Signature/StructID").attrib["value"]))
        # Assuming sid is in dictionary
        db_table = db_configs[sid]

        # Scan for matching rows in the DB
        xpaths : list[str] = get_xpaths_of_children(tree.find("./Data"), "./Data/")
        attributed_paths = get_attributed_paths(tree, xpaths)
        db_paths : list[str] = [
            db_structure.xpath_to_field(sid, apath[0], apath[2] is not None and (apath[2], apath[1]) or None)
            for apath in attributed_paths
        ]
        
        query = "SELECT ID, Description FROM [%s] WHERE %s;" % (
            db_table,
            " AND ".join([
                "[%s]=%s" % (db_paths[i], attributed_paths[i][1])
                for i in range(len(db_paths))
            ])
        )

        row = cursor.execute(query).fetchone()

        if row is None:
            return None
        else:
            id = row[0]
            desc = row[1]
            return xml_db_match(file_path, config_signature(sid, id), desc)

    except:
        return None

def print_usage():
    import sys
    print("Usage: python %s <database path> <xml to lookup>..." % sys.argv[0])

if __name__ == "__main__":
    import sys
    import getopt
    # Grab the command-line arguments.
    (opts, args) = getopt.getopt(sys.argv[1:], "")

    if len(args) < 2:
        print_usage()
        exit(-1)

    # Open the DB connection.
    BASE_CONN_STR = r'Driver={Microsoft Access Driver (*.mdb, *.accdb)};DBQ='
    db_conn : pyodbc.Connection
    try:
        db_conn = pyodbc.connect("%s%s;" % (BASE_CONN_STR, args[0]), readonly=True)
    except:
        print("Could not connect to the database at \"%s\"." % args[0])
        exit(1)

    cursor = db_conn.cursor()

    # Find what structures are defined in the DB.
    supported_structs = db_structure.get_supported_structures(cursor)


    # Scan for matching rows in the DB.
    matches = []
    for i in range(1, len(args)):
        xml_path = args[i]
        match = find_db_match(xml_path, supported_structs, cursor)
        if match is not None:
            matches.append(match)

    cursor.close()
    db_conn.close()

    # Report the signature and description for the row in the DB.
    for match in matches:
        print(match)

# EOF
