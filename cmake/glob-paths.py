#!/usr/bin/env python

import os
import os.path
import re
import sys
import textwrap


def dedent(x):
    """
    Patched dedent that also takes the last empty line into account.
    """
    return textwrap.dedent(x + "x")[:-1]


def split_path(path):
    """
    Normalize the given path and return a list of its components.

    A leading '.' of a relative path is removed.
    """
    components = os.path.normpath(path).split(os.sep)
    if len(components) > 0 and components[0] == ".":
        del components[0]
    return components


def find(path, target):
    """
    Find and replace target sources markers.
    """
    header = {}
    output = ""
    for root, _, filenames in os.walk(path):
        for filename in sorted(filenames):
            components = split_path(root)
            components.append(filename)
            if re.match(r"^.*\.(h|hh|hpp|c|cc|cpp)$", filename):
                header.setdefault(root, "")
                header[root] += f'    "{"/".join(components)}"\n'
            elif re.match(r"^.*\.(yy)$", filename):
                name, ext = os.path.splitext(filename)
                path = "".join([d + "/" for d in components[:-1]])
                header[os.path.join(root, name)] = dedent(f"""\
                    "{path}{name}{ext}"
                    ${{BISON_{name}_OUTPUTS}}
                """)
                output += f'bison_target_or_gen("{path}{name}{ext}")\n'
            elif re.match(r"^.*\.(xh|xch)$", filename):
                header.setdefault(root, "")
                name, ext = os.path.splitext(filename)
                path = "/".join(components[:-1])
                header[root] += dedent(f"""\
                    "{path}/{name}{ext}"
                    ${{RE2C_{name}_OUTPUT}}
                """)
                output += f're2c_target_or_gen("{path}/{name}{ext}")\n'

    output += f'set(ide_{target}_group "{target.title()} Files")\n'

    groups = []
    for root in sorted(header):
        components = split_path(root)
        if len(components) > 0 and components[0] in ("src", "include", "tests"):
            del components[0]
        groups.append("-".join([f"{target}-group"] + components))
        output += f"set({groups[-1]}\n"
        for value in header[root]:
            output += value
        output = output[:-1] + ")\n"
        target_group = r"\\".join([f"${{ide_{target}_group}}"] + components)
        output += f'source_group("{target_group}" FILES ${{{groups[-1]}}})\n'

    output += f"set({target}\n"
    for group in groups:
        output += f"    ${{{group}}}\n"
    output = output[:-1] + ")\n"
    return output


def rep(m):
    """
    Generate file list.
    """
    path = m.group("path")
    target = m.group("target")
    content = find(path, target)
    return f"# [[[{target}: {path}\n{content}# ]]]"


def run():
    """
    Entry point of the script.
    """
    files = [os.path.abspath(f) for f in sys.argv[1:]]

    for f in files:
        os.chdir(os.path.dirname(f))

        with open(f, "r", encoding="utf8") as hnd:
            content = hnd.read()
        replace = re.sub(
            r"#[ ]*\[\[\[(?P<target>[^:]*): (?P<path>[^\n\]]*)(.|\n)*?\]\]\]",
            rep,
            content,
            0,
            re.MULTILINE,
        )

        if content != replace:
            sys.stderr.write(f"File {f} changed!\n")
            sys.stderr.flush()
            with open(f, "w", encoding="utf8") as hnd:
                hnd.write(replace)


if __name__ == "__main__":
    run()
