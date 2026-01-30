# Defines the structure of the database, and the XPath conversion to the field.
from apt import *
import re
import pyodbc

# Reads in the supported structures in the database.
def get_supported_structures(cursor : pyodbc.Cursor):
    rt : dict[StructID, str] = {}

    rows = cursor.execute("SELECT StructID, TableName FROM db_mappings;").fetchall()
    
    for row in rows:
        try:
            sid = StructID(int(row[0]))
            rt[sid] = row[1]
        except:
            pass

    return rt

# Given the xpath to a data field in the struct_id, convert it into
# the field in the table.
# If the attribute field is added, then the db is storing the attribute
# value for that field.
def xpath_to_field(struct_id : StructID, xpath : str,
    attribute : tuple[str, str] | None = None) -> str | None:
    att = attribute is not None and attribute[0] or None
    return _convert_xpath_default(xpath, att)

def _convert_xpath_default(xpath : str, attribute : str | None):
    match = re.match(r"./Data/(.*)", xpath)
    appended : str
    if attribute is None:
        appended = ""
    else:
        appended = "_%s" % attribute
    return match[1] + appended