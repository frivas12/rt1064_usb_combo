import sys
import os.path
import re
import graph_template_dependencies as gtd
import getopt

# Extensions for source files.
valid_source_exts = set([".c", ".cc", ".cpp", ".cxx"])

# extensions for template files.
valid_template_exts = set([".tcc"])

# Returns three lists, the header files, source files, and template files in that order.
def separate_files_from_args(args : list[str]) -> (list[str], list[str], list[str]):
    s = ""
    for arg in args:
        s += str(arg) + " "

    header_files = list(gtd.process_c_include_parameter(s))
    source_files : list[str] = []
    template_files : list[str] = []

    for arg in args:
        if not re.match(r"-I.*", arg):
            ext = os.path.splitext(arg)[1]
            if ext in valid_source_exts:
                source_files.append(arg)
            elif ext in valid_template_exts:
                template_files.append(arg)

    return header_files, source_files, template_files

def process_command_line_string(input : str) -> list[str]:
    args = list()

    quote_regex = re.compile("(?:\")[^\"]+(?:\")")
    normal_regex = re.compile("(?:\\\\ |[^\\s])+")

    quoted_args = []
    for arg in quote_regex.findall(input):
        quoted_args.append(arg[1:-1])

    non_quoted_input = quote_regex.sub("", input)

    non_quoted_args = normal_regex.findall(non_quoted_input)

    args.extend(quoted_args)
    args.extend(non_quoted_args)

    return args


if __name__ == "__main__":

    OUTPUT_TEMPLATE="{object_name}.o: {source_path} {template_paths}\n\t$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@"

    try:
        opts, args = getopt.getopt(sys.argv[1:], "", ["template="])
    except:
        exit(1)
    
    for opt, arg in opts:
        if opt == "--template":
            file = None
            try:
                file = open(arg, 'r')
            except:
                pass
            if file is not None:
                OUTPUT_TEMPLATE = file.read()

    header_files, source_files, template_files = separate_files_from_args(args)

    all_files = []
    all_files.extend(header_files)
    all_files.extend(source_files)
    all_files.extend(template_files)

    g = gtd.SourceGraph()

    for file in source_files:
        g = gtd.path_to_graph(file, all_files, g)

    for file in template_files:
        g = gtd.path_to_graph(file, all_files, g)

    # Now that the graph has been made, it's time to identify the template dependencies.

    gflip = g.reverse()

    template_dependency_dict = {}

    for file in template_files:
        node = gtd.SourceNode(file)

        nodes = gflip.bfs(node)

        for i in nodes:
            ext = os.path.splitext(i.path)[1]
            if ext in valid_source_exts:
                if i.path in template_dependency_dict:
                    template_dependency_dict[i.path].add(file)
                else:
                    template_dependency_dict[i.path] = set([file])

    for key in template_dependency_dict:
        tmp_paths = ""
        for temp in template_dependency_dict[key]:
            tmp_paths += " " + temp
        
        src_path = key
        object_path = os.path.splitext(src_path)[0]

        print(OUTPUT_TEMPLATE.format(object_name = object_path, source_path = src_path, template_paths = tmp_paths))