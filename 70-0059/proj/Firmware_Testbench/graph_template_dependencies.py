# graph_template_dependencies.py
# Graph Properties:
#   Directed
#   Nodes: files
#   Edges:
#       Head: Included File
#       Tail: Includee File

import re
import os.path
import os

class SourceNode:
    path : str = ""

    def __init__(self, path):
        self.path = path

    def __eq__ (self, other):
        return self.path == other.path

    def __hash__ (self):
        return hash(self.path)

    def __str__ (self):
        return self.path

class SourceEdge:
    head : SourceNode = None
    tail : SourceNode = None

    def __init__(self, head, tail):
        self.head = head
        self.tail = tail

    def __hash__(self):
        return hash((self.head, self.tail))

    def __str__(self):
        return str(self.tail) + " -> " + str(self.head)

class SourceGraph:
    nodes : set[SourceNode] = set()
    edges : set[SourceEdge] = set()

    # Where the target is the tail.
    def get_connected(self, target : SourceNode) -> set[SourceNode]:
        con : set[SourceNode] = set()

        for edge in self.edges:
            if (edge.tail == target):
                con.add(edge.head)

        return con

    # Where the target is the head.
    def get_connected_to(self, target : SourceNode) -> set[SourceNode]:
        con : set[SourceNode] = set()

        for edge in self.edges:
            if (edge.head == target):
                con.add(edge.tail)

        return con

    def join (self, other):
        nodes = self.nodes.union(other.nodes)
        edges = self.edges.union(other.edges)

        return SourceGraph(nodes, edges)

    def bfs (self, start : SourceNode) -> set[SourceNode]:
        targets : list[SourceNode] = []
        visited : set[SourceNode] = set()

        targets.append(start)

        while len(targets) > 0:
            target = targets.pop(0)
            
            connected_nodes = self.get_connected(target)
            for node in connected_nodes:
                if node not in visited:
                    visited.add(node)
                    targets.append(node)

        return visited

    def __init__(self, nodes=set(), edges=set()):
        self.nodes = nodes
        self.edges = edges

    def reverse(self):
        rev_edges : set[SourceEdge] = set()
        for edge in self.edges:
            rev_edges.add(SourceEdge(head=edge.tail, tail=edge.head))

        return SourceGraph(nodes=self.nodes, edges=rev_edges)

def approximate_path_correctness(path1 : str, path2 : str):
    split1 = os.path.split(path1)
    split2 = os.path.split(path2)

    basename1 = os.path.basename(split1[1])
    basename2 = os.path.basename(split2[1])

    bases_match = basename1 == basename2

    if split1[0] == '' or split2[0] == '':
        # Top level, so compare the two subs
        return bases_match
    elif bases_match:
        # Matching bases, so go up a level.
        return approximate_path_correctness(split1[0], split2[0])
    else:
        return False

def get_best_path(path : str, paths : list[str]) -> str:
    rt = None
    prev_cnt = -1
    previous = [True for i in range(len(paths))]
    c_paths = [path for path in paths]
    c_path = path
    while prev_cnt != 0:
        current = [previous[i] and approximate_path_correctness(c_path, c_paths[i]) for i in range(len(c_paths))]
        c_paths = [os.path.dirname(c_paths[i]) for i in range(len(c_paths))]
        
        cnt = 0
        for i in range(len(c_paths)):
            if current[i]:
                cnt += 1
        
        if cnt == 0:
            if prev_cnt != -1:
                for i in range(len(paths)):
                    if previous[i]:
                        rt = paths[i]
                        break

        

        prev_cnt = cnt
        previous = current

    return rt
    


def process_c_include_parameter(input : str) -> set[str]:
    includes = set()

    include_regexs = [
        re.compile("(?<=-I\\s)|(?<=-I)(?:\\\\ |[^\\s\"])+"),     # All valid paths without quotes
        re.compile("(?<=-I\\s\")|(?<=-I\").+(?=\")")               # All valid paths with quotes
    ]
    valid_exts = set([".h", ".hh", ".hxx", ".hpp"])

    tmp = [re.findall(r, input) for r in include_regexs]

    tmp[0].extend(tmp[1])

    found = tmp[0]

    for path in found:
        if os.path.isdir(path):
            for sub_path in os.listdir(path):
                ext = os.path.splitext(sub_path)[1]
                if ext in valid_exts:
                    adj_path = (path[-1] == "/") and path or (path + "/")
                    includes.add(adj_path + sub_path)
        else:
            includes.add(path)

    return includes
    

def path_to_graph(path : str, file_paths : list[str], gph : SourceGraph = SourceGraph()) -> SourceGraph:
    include_regex = re.compile(".*#include\\s*[\"'<](.+\\.+.+)[\"'>]")

    base_node = SourceNode(path)
    # Don't enter paths that have already been processed.
    if (not base_node in gph.nodes):
        file = None
        try:
            file = open(path, 'r')
        except:
            pass
        if file:
            gph.nodes.add(base_node)

            # Get all paths included in this file.
            include_paths : set[str] = set()
            for line in file.readlines():
                result = re.match(include_regex, line)
                if result:
                    file_path = get_best_path(result[1], file_paths) 
                    if (file_path):
                        include_paths.add(file_path)
            file.close()
            
            # Populate the included paths into the graph.
            for sub_path in include_paths:
                gph = path_to_graph(sub_path, file_paths, gph)

                # Connect base node to sub graph's node.
                target = SourceNode(sub_path)
                for sub_node in gph.nodes:
                    if (sub_node == target):
                        edge = SourceEdge(sub_node, base_node)
                        gph.edges.add(edge)
                        break

    return gph

def debug_print_graph(gph : SourceGraph):
    print("Nodes:")
    for node in gph.nodes:
        print("\t(%s)" % (node))

    print("\nEdges:")
    for edge in gph.edges:
        print("\t(%s) -> (%s)" % (edge.tail, edge.head))


