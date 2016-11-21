#!/usr/bin/env python

import os
import os.path
import re
import sys

def split_path(path):
    components = os.path.normpath(path).split(os.sep)
    if len(components) > 0 and components[0] == ".": del components[0]
    return components

def find(path, target):
    header = {}
    output = ""
    for root, dirnames, filenames in os.walk(path):
        if "tests" in dirnames:
            dirnames.remove("tests")
        if target == "header" and "src" in dirnames:
            dirnames.remove("src")
        for filename in sorted(filenames):
            components = split_path(root)
            components.append(filename)
            if re.match(r"^.*\.(h|hh|hpp|c|cc|cpp)$", filename):
                header.setdefault(root, "")
                header[root] += "    \"${{{}_path}}/{src}\"\n".format(target, src="/".join(components))
            if re.match(r"^.*\.(yy)$", filename):
                name, ext = os.path.splitext(filename)
                path = ''.join([d + "/" for d in components[:-1]])
                header[os.path.join(root, name)] = '''\
    "${{{target}_path}}/{path}{name}{ext}"
    ${{BISON_{name}_OUTPUTS}}
'''.format(name=name,target=target,ext=ext,path=path)
                output += '''\
file(MAKE_DIRECTORY "${{CMAKE_CURRENT_BINARY_DIR}}/{path}{name}")
bison_target("{name}" "${{{target}_path}}/{path}{name}{ext}" "${{CMAKE_CURRENT_BINARY_DIR}}/{path}{name}/grammar.cc")
if(MSVC)
    set_source_files_properties("${{CMAKE_CURRENT_BINARY_DIR}}/{path}{name}/grammar.cc"
        PROPERTIES COMPILE_FLAGS "/wd4065")
endif()
'''.format(name=name, path=path, ext=ext, target=target)

    output+= 'set(ide_{}_group "{} Files")\n'.format(target, target.title())

    groups = []
    for root in sorted(header):
        components = split_path(root)
        if len(components) > 0 and components[0] == "src": del components[0]
        groups.append('-'.join(["{}-group".format(target)] + components))
        output += "set({0}\n".format(groups[-1])
        for value in header[root]:
            output += value
        output = output[:-1] + ")\n"
        output+= """source_group("{}" FILES ${{{}}})\n""".format(r"\\".join(["${{ide_{}_group}}".format(target)] + components), groups[-1])

    output+= 'set({}\n'.format(target)
    for group in groups:
        output+= """    ${{{}}}\n""".format(group)
    output = output[:-1] + ")\n"
    return output

def rep(m):
    path = m.group("path")
    target = m.group("target")
    content = find(path, target)
    return "# [[[{}: {}\n{}# ]]]".format(target, path, content)

files = [os.path.abspath(f) for f in sys.argv[1:]]

for f in files:
    os.chdir(os.path.dirname(f))

    content = open(f, "r").read();
    replace = re.sub(r"#[ ]*\[\[\[(?P<target>[^:]*): (?P<path>[^\n\]]*)(.|\n)*?\]\]\]", rep, content, 0, re.MULTILINE)

    if content != replace:
        sys.stderr.write("File changed!\n")
        sys.stderr.flush()
        open(f, "w").write(replace)

