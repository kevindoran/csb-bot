import re
import os
import sys

local_include_regex = r'#include "([^"]+)"'
cpp_extension = "cpp"


def topo_dependencies(main_file):
    seen = set()
    sorted_deps = []
    dfs(main_file, seen, sorted_deps)
    return sorted_deps


def dfs(file, seen, res):
    if file not in seen:
        seen.add(file)
        for dep in dependencies(file):
            dfs(dep, seen, res)
        res.append(file)


def dependencies(file):
    with open(file) as f:
        filetext = f.read()
        file_names = re.findall(local_include_regex, filetext)
    cpp_file = matching_cpp_file(file)
    if os.path.exists(cpp_file):
        with open(cpp_file) as cf:
            filetext = cf.read()
            file_names_cpp = re.findall(local_include_regex, filetext)
            file_names.extend(file_names_cpp)
    return file_names

def matching_cpp_file(header_file):
    cpp_file = header_file.split('.')[0] + '.' + cpp_extension
    return cpp_file


def merge(main_file):
    header_files = topo_dependencies(main_file)
    cpp_files = []
    out = ""
    for header in header_files:
        with open(header) as hf:
            filetext = hf.read()
            filetext = re.sub(local_include_regex, '', filetext)
            out += filetext

            cpp_file = matching_cpp_file(header)
            if os.path.exists(cpp_file):
                cpp_files.append(cpp_file)

    for cpp_file in cpp_files:
        if cpp_file == main_file:
            continue
        with open(cpp_file) as f:
            filetext = f.read()
            filetext = re.sub(local_include_regex, '', filetext)
            out += filetext
    return out

cwd = os.getcwd()
(head, tail) = os.path.split(sys.argv[1])
if head:
    os.chdir(os.path.join(cwd, head))
combined = merge(tail)
os.chdir(cwd)
with open(sys.argv[2], 'w') as f:
    f.write(combined)

