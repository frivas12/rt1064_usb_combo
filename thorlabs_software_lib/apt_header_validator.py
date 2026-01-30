import re
import sys
import os.path

DEFINE_MACRO=re.compile(r'^#define\s+(M\S+)\s+(0x[\dabcdefABCDEF]+)')
IGNORED_INCLUDE_MACRO=re.compile(r'^#include\s*"(.*)"\s*\/\/\s*No region')
INCLUDE_MACRO=re.compile(r'^#include\s*"(.*)"\s*(\/\/\s*Region:\s*0x(.{4})\s*-\s*0x(.{4}))?')

class parser_error:
    file : str
    line_number : int
    message : str

    def __init__(self, file : str, line_number : int, message : str):
        self.file = file
        self.line_number = line_number
        self.message = message

    def __str__(self):
        return f"({self.file}:{self.line_number}) {self.message}"

class command_parser_error(parser_error):
    name : str
    id : int
    
    def __init__(self, file : str, line_number : int, command_name : str, command_id : int, message : str):
        super().__init__(file, line_number, message)
        self.name = command_name
        self.id = command_id

    def __str__(self):
        return f"({self.file}:{self.line_number}) {self.name}(0x{self.id:04X}): {self.message}"


class parser_state:
    _command_set : set[str]
    _id_map : dict[int, str]
    _first_defined_line : dict[int, int]
    _last_id_read : int
    _line_number : int
    _file_path : str
    _min_value : int
    _max_value : int
    _ran : bool
    error : bool

    def __init__(self, file_to_parse : str):
        self._command_set = set()
        self._id_map = {}
        self._first_defined_line = {}
        self._last_id_read = -1
        self._line_number = 1
        self._file_path = file_to_parse
        self._min_value = -1
        self._max_value = 0x10000
        self._ran = False
        self.error = False

    def run(self) -> list[parser_error]:
        file = None
        try:
            file = open(self._file_path, 'r')
            for line in file.readlines():
                for error in self._add_line(line):
                    yield error
        finally:
            self._ran = True
            if file is not None:
                file.close()

    def get_commands(self) -> list[(str, int)]:
        if not self._ran:
            for _ in self.run():
                pass

        return [(self._id_map[id], id) for id in self._id_map]


    def _add_line(self, line : str) -> list[parser_error]:
        for error in self._add_line_helper(line):
            self.error = True
            yield error
        self._line_number += 1
    
    def _add_line_helper(self, line : str) -> list[parser_error]:
        match = DEFINE_MACRO.match(line)
        if match is not None:
            return self._add_define_line(match)
        match = INCLUDE_MACRO.match(line)
        if match is not None and IGNORED_INCLUDE_MACRO.match(line) is None:
            return self._add_include_line(match)
        
        return []

    def _add_define_line(self, match : re.Match[str]) -> list[parser_error]:
        name = match.group(1)
        id = int(match.group(2)[2:], base=16)
        if name in self._command_set:
            if id in self._id_map:
                if self._id_map[id] == name:
                    return [self._generate_error_command_redefined(name, id)]
                else:
                    return [self._generate_error_collision(name, id)]
            else:
                return [self._generate_error_command_redefined(name, id)]

        if id in self._id_map:
            return [self._generate_error_collision(name, id)]

        rt : list[parser_error] = []

        self._command_set.add(name)
        self._id_map[id] = name
        self._first_defined_line[id] = self._line_number

        if self._last_id_read >= id:
            rt.append(command_parser_error(self._file_path, self._line_number, name, id, f"ascending order violation"))
        self._last_id_read = id

        if id < self._min_value or id > self._max_value:
            rt.append(command_parser_error(self._file_path, self._line_number, name, id, \
                                           f"outside of region bound [0x{self._min_value:04X},0x{self._max_value:04X}]"))
        
        return rt



    def _add_include_line(self, match : re.Match[str]) -> list[parser_error]:
        rt : list[parser_error] = []
        path = os.path.join(os.path.dirname(self._file_path), match.group(1))
        file_path = self._file_path
        line_number = self._line_number
        min_value = self._min_value
        max_value = self._max_value
        self._file_path = path
        self._line_number = 1
        if match.group(2) == None:
            rt.append(parser_error(self._file_path, self._line_number, "did not find include range expression"))
        else:
            self._min_value = int(match.group(3), base=16)
            self._max_value = int(match.group(4), base=16)

        try:
            rt.extend(self.run())

        except FileNotFoundError:
            rt.append(parser_error(self._file_path, self._line_number, f"could not open \"{match.group(1)}\""))
        finally:
            self._file_path = file_path
            self._line_number = line_number
            self._min_value = min_value
            self._max_value = max_value
            self._ran = False

        return rt


    def _generate_error_command_redefined(self, name : str, id : int) -> command_parser_error:
        return command_parser_error(self._file_path, self._line_number, name, id, f"redefined (first defined on line {self._first_defined_line[id]})" )

    def _generate_error_collision(self, name : str, id : int) -> command_parser_error:
        return command_parser_error(self._file_path, self._line_number, name, id, f"collision with {self._id_map[id]} (defined on line {self._first_defined_line[id]})")




if __name__ == "__main__":

    def print_usage(argv_0 : str):
        print(f"Usage: python {argv_0} <path to \"apt.h\">")

    if len(sys.argv) != 2:
        print_usage()
        exit(-1)

    state = parser_state(sys.argv[1])

    try:
        for error in state.run():
            print(error)
    except FileNotFoundError:
        print(f"Error: cannot open \"{sys.argv[1]}\"")
        exit(-1)

    if state.error:
        exit(1)