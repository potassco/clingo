"""
Simple hack to better document pybind11 generated enums with pdoc.
"""

import re
import sys

for path in sys.argv[1:]:
    lines = []
    with open(path, "r") as hnd:
        mem = False
        com = False
        dct = {}
        for line in hnd:
            if not com:
                lines.append(line)
                com = re.match(' *"""$', line) is not None
                if not com:
                    for key, val in dct.items():
                        if line.find(f"{key}: typing.ClassVar") >= 0:
                            lines.append('    """\n')
                            lines.append(f"    {val}\n")
                            lines.append('    """\n')
            else:
                if line.find('"""') >= 0:
                    lines.append(line)
                    mem = com = False
                else:
                    if not mem:
                        mem = line.find("Members:") >= 0
                        if mem:
                            dct = {}
                            lines.pop()
                        else:
                            lines.append(line)
                    else:
                        elm = re.match(r" *([^ :]*) : (.*)", line)
                        if elm is not None:
                            dct[elm[1]] = elm[2]
    with open(path, "w") as hnd:
        hnd.write("".join(lines))
