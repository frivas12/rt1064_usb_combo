import hashlib


class lut_version_file_builder:
    _file_lengths: dict[int, int]
    _hashes: dict[int, bytes]
    _major_version: int
    _minor_version: int
    _interim_version: int

    def __init__(self):
        self._file_lengths = {}
        self._hashes = {}
        self._major_version = -1
        self._minor_version = -1
        self._interim_version = -1

    def add_file(self, efs_id: int, stream):
        if efs_id in self._hashes:
            raise Exception("%d already built" % efs_id)
        if efs_id < 0 or efs_id > 31:
            raise Exception("id %d is out of the EFS range:  [0, 31]" % efs_id)
        if efs_id == 2:  # ID 2 = LUT_ID::VERSION
            raise Exception("cannot add version file")

        stream_content = stream.read()
        hasher = hashlib.sha3_224()
        hasher.update(stream_content)
        self._file_lengths[efs_id] = len(stream_content)
        self._hashes[efs_id] = hasher.digest()

        return self

    def add_version(self, major: int, minor: int, interim: int):
        if major < 0 or major > 255:
            raise Exception("major value is out of uint8_t bounds")
        if minor < 0 or minor > 255:
            raise Exception("minor value is out of uint8_t bounds")
        if interim < 0 or interim > 65535:
            raise Exception("interim value is out of uint16_t bounds")

        self._major_version = major
        self._minor_version = minor
        self._interim_version = interim

        return self

    def build(self) -> bytes:
        if len(self._hashes) == 0:
            raise Exception("no files added")
        if self._major_version == -1:
            raise Exception("major version not set")
        if self._minor_version == -1:
            raise Exception("minor version not set")
        if self._interim_version == -1:
            raise Exception("interim version not set")
        if len(self._file_lengths) != len(self._hashes):
            raise Exception("file lengths and hash keys mismatch")

        max_id = max(max(self._hashes.keys()), 2)
        rt = bytearray()
        for i in range(0, max_id + 1, 1):
            if i in self._hashes:
                rt.extend(self._file_lengths[i].to_bytes(4, 'little'))
                rt.extend(self._hashes[i])
            elif i == 2:
                rt.extend(self._get_version_bytes())
            else:
                rt.extend(bytes([0xFF for i in range(32)]))

        return rt

    def _get_version_bytes(self) -> bytearray:
        rt = bytearray(32)
        inclusion_bitmap = 0
        for bit in range(31, -1, -1):
            inclusion_bitmap = inclusion_bitmap << 1
            v = (bit in self._hashes) and 1 or 0
            inclusion_bitmap = inclusion_bitmap + v
        inclusion_bitmap = inclusion_bitmap + 4

        inclusion_bitmap = inclusion_bitmap.to_bytes(4, "little")
        for byte in range(0, 4):
            rt[byte] = inclusion_bitmap[byte]

        rt[4] = self._major_version
        rt[5] = self._minor_version

        interim_bytes = self._interim_version.to_bytes(2, "little")
        for byte in range(6, 8):
            rt[byte] = interim_bytes[byte - 6]

        for byte in range(8, 32):
            rt[byte] = 0xFF

        return rt


class line_string_iterator:
    _list: list[str]

    def __init__(self, string: str, seperator: str = '\n'):
        self._list = string.split(seperator)

    def __iter__(self):
        return self._list.__iter__()


if __name__ == "__main__":
    import getopt
    import sys
    import csv

    OPTIONS = [
        ["o", "output", True],
        ["c", "compare", True],
        [None, "compare-min-length", False]
    ]

    opts, args = getopt.getopt(sys.argv[1:],
                               ''.join([(i[0] + (i[2] and ':' or ''))
                                       for i in OPTIONS if i[0] is not None]),
                               [(i[1] + (i[2] and '=' or '')) for i in OPTIONS if i[1] is not None])
    opts_dict = {}
    for tup in opts:
        key: str = tup[0]\
            .removeprefix('--') \
            .removeprefix('-')
        for i in OPTIONS:
            if i[2] is not None and i[2] == key:
                key = i[0]
                break

        opts_dict[key] = tup[1]

    if len(args) != 2:
        print(
            "USAGE:  python version-compiler.py [-o output-path] "
            "[-c compare-path] [--compare-min-length] <file csv> "
            "<major.minor.patch>")
        exit(-1)

    version_string = args[1].split('.')

    if len(version_string) != 3:
        print(
            f"ERROR:  version string \"{args[1]}\" was not in "
            "semantic-version order")
        exit(-1)

    version_array: list[int]
    try:
        version_array = [int(item) for item in version_string]
    except Exception:
        print("ERROR:  could not parse version string as integers")
        exit(-1)

    csv_string: str
    try:
        csv_file = open(args[0], 'r', newline="")
        csv_string = csv_file.read()
        csv_file.close()
    except FileNotFoundError:
        print("ERROR:  could not find csv file \"%s\"" % args[0])
        exit(-1)
    except Exception as e:
        print("ERROR:  error duing reading csv file\n        %s" % e)
        exit(-1)

    csv_string = line_string_iterator(csv_string.replace('\r', ''))

    csv_reader = None
    try:
        csv_reader = csv.reader(
            csv_string, lineterminator='\n', skipinitialspace=True)
    except Exception as e:
        print("ERROR:  csv file not recoginzed\n        %s" % e)
        exit(-1)

    builder = lut_version_file_builder()

    try:
        builder.add_version(
            version_array[0], version_array[1], version_array[2])
    except Exception as e:
        print("ERROR:  could not add version number\n        %s" % e)
        exit(-1)

    line_counter = 1
    for row in csv_reader:
        if len(row) == 2:
            file = None
            try:
                file = open(row[1], 'rb')
            except FileNotFoundError:
                print(f"ERROR:  could not open file \"{row[1]}\" on line "
                      f"{line_counter:d} of \"{args[0]}\"")
                exit(-1)
            except Exception as e:
                print("ERROR:  could not open \"%s\"\n        %s" %
                      (row[1], e))
                exit(-1)

            id
            try:
                id = int(row[0])
            except Exception as e:
                print(f"Error:  could not convert \"{row[0]}\" on line "
                      f"{line_counter:d} to an integer\n        {e}")
                file.close()
                exit(-1)

            try:
                builder.add_file(id, file)
            except Exception as e:
                print("Error:  could not add file \"%s\" as id %d\n        %s" % (
                    row[1], id, e))
                file.close()
                exit(-1)

            file.close()
            line_counter = line_counter + 1
        elif row == []:
            pass  # Allow empty lines
        else:
            print("ERROR:  line %d in \"%s\" has row count of %d instead of 2" % (
                line_counter, args[0], len(row)))
            exit(-1)

    byte_output: bytes
    try:
        byte_output = builder.build()
    except Exception as e:
        print("ERROR:  failed to build version file\n        %s" % e)
        exit(-1)

    if ('c' in opts_dict):
        file_contents: str
        try:
            file = open(opts_dict['c'], 'rb')
            file_contents = file.read()
            file.close()
        except FileNotFoundError:
            print("ERROR:  cannot find compare file \"%s\"" % opts_dict['c'])
            exit(-1)
        except Exception as ex:
            print("ERROR:  could not open comparison file \"%s\"\n        %s" %
                  (opts_dict['c'], ex))
            exit(-1)

        if 'compare-min-length' in opts_dict:
            min_size = min(len(file_contents), len(byte_output))
            file_contents = file_contents[0:min_size - 1]
            byte_output = byte_output[0:min_size - 1]

        if len(file_contents) != len(byte_output):
            print("MISMATCH:  compared file size of %d did not equal compiled file size of %s" % (
                len(file_contents), len(byte_output)))
            exit(1)

        rt = 0
        for i in range(len(file_contents)):
            if file_contents[i] != byte_output[i]:
                print("MISMATCH:  compiled (%d) != compared (%d) @ %d" %
                      (byte_output[i], file_contents[i], i))
                rt = 1

        exit(rt)

    output: str
    if ('o' in opts_dict):
        output = opts_dict['o']
    else:
        output = "./lut-version.bin"

    output_file = None
    try:
        output_file = open(output, 'wb')
    except Exception as e:
        print("ERROR:  could not open output file \"%s\"\n        %s" % (output, e))
        exit(-1)

    try:
        output_file.write(byte_output)
    except Exception as e:
        print("ERROR:  could not write to file\n        %s" % e)
        output_file.close()
        exit(-1)

    output_file.close()
