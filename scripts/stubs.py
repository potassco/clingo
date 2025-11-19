"""
Install the package and stubs into the virtual environment `.venv`.
"""

import argparse
import os
import re
import subprocess
import sys
import sysconfig
from glob import glob

import black
import isort


class Rewriter:
    """
    Rewriter to fine-tune generated stubs.
    """

    def __init__(self):
        self.python = False

    def format(self, path: str, content: str):
        """
        Format the given code.
        """
        try:
            content = isort.code(content, profile="black")
            mode = black.FileMode(is_pyi=not self.python)
            return black.format_file_contents(content, fast=False, mode=mode)
        except:
            sys.exit(f"failed to format: {path}")

    def simplify_comparisons(self, content: str) -> str:
        """
        Simplify comparison signatures.
        """
        for method in ["__eq__", "__ne__", "__lt__", "__le__", "__gt__", "__ge__"]:
            overload_pattern = rf"(@typing\.overload\s+def\s+{method}.*?\.\.\.(?:\s*@typing\.overload\s+def\s+{method}.*?\.\.\.)*)"
            simple_pattern = rf"def\s+{method}.*?\.\.\."

            # replacement for both patterns
            replacement = f"def {method}(self, arg0: typing.Any) -> bool: ..."

            # apply both patterns
            content = re.sub(overload_pattern, replacement, content, flags=re.DOTALL)
            content = re.sub(simple_pattern, replacement, content, flags=re.DOTALL)
        return content

    def replace_list_returns(self, content: str) -> str:
        """
        Replace lists in return values by sequences.
        """
        return content.replace("-> list[", "-> typing.Sequence[")

    def extract_docstrings(self, code):
        """
        Extract and replace dostrings with a placeholder.
        """
        docstring_pattern = r'("""[\s\S]*?""")'
        docstrings = {}
        count = 0

        def replace(match):
            nonlocal count
            count += 1
            docstrings[count] = match.group(1)
            return f"#docstring[{count}]"

        code = re.sub(docstring_pattern, replace, code)
        return code, docstrings

    def reinsert_docstrings(self, code, docstrings):
        """
        Reinsert docstring placeholders.
        """
        for i, docstring in docstrings.items():
            code = code.replace(f"#docstring[{i}]", docstring)
        return code

    def patch_repr(self, class_def: str, repr_implementation: str) -> str:
        """
        Replace the implementation of the __repr__ method in a class definition.

        Args:
            class_code (str): The string containing the class definition.
            repr_implementation (str): The new implementation for the __repr__ method.

        Returns:
            str: The modified class definition with the patched __repr__ method.
        """
        repr_pattern = re.compile(r"def __repr__\(.*?\).*:\n\s+\.\.\.")
        indented_repr = "\n".join(
            f"        {line}" for line in repr_implementation.strip().splitlines()
        )

        def replace_repr(match):
            assert match
            return f"def __repr__(self) -> str:\n{indented_repr}"

        return re.sub(repr_pattern, replace_repr, class_def)

    def generate_enum_members(self, class_def):
        """
        Generate members of enums.
        """
        pattern = r"(\w+):\s*typing\.ClassVar\[(.*?)\].*?#\s*value\s*=\s*<.*?\.\w+:\s*(-?\d+)>"
        matches = re.findall(pattern, class_def, re.DOTALL)

        def repl(match):
            member, _, value = match.groups()
            return f"{member} = {value}"

        class_def = re.sub(pattern, repl, class_def)
        class_def = re.sub(r"@classmethod[\s\S]*", "", class_def)

        repr_impl = ""
        # Generate assignments
        for member, class_name, _value in matches:
            repr_impl += f'if self.{member} is self: return "{class_name}.{member}"\n'

        return self.patch_repr(class_def, repr_impl) + "\n"

    def enums_to_top(self, content: str):
        """
        Move enums to the top of the generated file.
        """
        # extract class definitions
        content, docstrings = self.extract_docstrings(content)
        content = self.simplify_comparisons(content)
        content = self.replace_list_returns(content)
        class_pattern = re.compile(r"(?m)^class\s[^:]+:\n(?:\n|(?:    .*\n))*")
        classes = class_pattern.findall(content)
        for class_def in classes:
            content = content.replace(class_def, "")

        # partition into enum and other classes
        enum_classes = []
        other_classes = []
        for class_def in classes:
            if "typing.ClassVar" in class_def:
                enum_classes.append(
                    self.generate_enum_members(class_def) if self.python else class_def
                )
            else:
                other_classes.append(class_def)

        # reorder class definitions
        return self.reinsert_docstrings(
            content.strip()
            + "\n\n"
            + "\n".join(enum_classes)
            + "\n"
            + "\n".join(other_classes),
            docstrings,
        )

    def doc_enums(self, content: str):
        """
        Document enum members.
        """
        lines: list[str] = []
        mem = False
        com = False
        dct: dict[str, str] = {}
        for line in content.splitlines():
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
        return "\n".join(lines)

    def generate(self):
        """
        Generate stubs.
        """
        # https://typing.readthedocs.io/en/latest/spec/distributing.html

        args = ["pybind11-stubgen"]
        if self.python:
            extension = ".py"
            libpath = "./pdoc"
            clingo_stubs = os.path.join(libpath, "clingo")
            args.extend(["--stub-extension=py"])
        else:
            extension = ".pyi"
            libpath = sysconfig.get_path("purelib")
            clingo_stubs = os.path.join(libpath, "clingo-stubs")
            args.extend(["--root-suffix=-stubs"])

        args.extend(
            [
                "--enum-class-locations",
                "WeightConstraintType:clingo.propagate",
                "--enum-class-locations",
                "LogLevel:clingo.core",
                "-o",
                libpath,
                "clingo",
            ]
        )
        env = os.environ.copy()
        env["PYTHONPATH"] = "./build/debug/bin/python"
        subprocess.check_call(args, env=env)

        for root, dirs, files in os.walk(clingo_stubs):
            assert dirs is not None
            for file in files:
                if file.endswith(extension):
                    sys.stderr.write(f"processing {file}...\n")
                    path = os.path.join(root, file)
                    with open(path, "r", encoding="utf8") as hnd:
                        content = hnd.read()
                    content = content.replace("typing.SupportsInt", "int")
                    content = self.enums_to_top(content)
                    if self.python:
                        content = self.doc_enums(content)
                    else:
                        content = self.format(path, content)
                        gen_content = None
                        gen_path = os.path.join("lib", "python-api", "stubs", file)
                        try:
                            with open(gen_path, "r", encoding="utf8") as hnd:
                                gen_content = hnd.read()
                        except IOError:
                            pass
                        if content != gen_content:
                            with open(gen_path, "w", encoding="utf8") as hnd:
                                hnd.write(content)
                    with open(path, "w", encoding="utf8") as hnd:
                        hnd.write(content)

        if not self.python:
            try:
                for lib in glob("./build/debug/bin/python/clingo*.so"):
                    name = os.path.basename(lib)
                    src = os.path.relpath(lib, libpath)
                    dst = os.path.relpath(os.path.join(libpath, name), ".")
                    os.symlink(src, dst)
            except FileExistsError:
                pass

    def main(self):
        """
        Parse commandline arguments.
        """
        parser = argparse.ArgumentParser(description="Generate clingo stubs.")

        parser.add_argument("--python", action="store_true", help="Enable the flag")

        args = parser.parse_args()
        self.python = args.python
        self.generate()


Rewriter().main()
