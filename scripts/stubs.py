"""
Generate the stubs of the clingo.ast submodules and install clingo's stubs.

The stubs of all modules are maintained by hand in `lib/python-api/stubs`
except for the AST submodules, which are generated from the description of the
AST structure provided by `clingo.ast._type_info()`.

Commands:
- ast: regenerate the stubs of the AST submodules
- check: check that the stubs of the AST submodules are up to date
- strip: copy the stubs removing the implementations of overloaded functions
  (used by CMake to install the stubs)
- install: install the stubs and link the built module into the active
  environment
- doc: copy the stubs for the documentation turning references like
  `clingo.ast.AST` into links

The generated stubs are formatted with black and isort if they are available.
"""

import argparse
import ast as pyast
import importlib
import os
import re
import shutil
import sys
import sysconfig
import tempfile
from collections.abc import Callable
from glob import glob
from pathlib import Path
from typing import Any

STUBS = Path("lib", "python-api", "stubs")
# NOTE: mkdocstrings searches the stubs directory for the clingo package
DOC_STUBS = Path("doc", "python-api", "stubs", "clingo")
BUILD = Path("build", "debug", "bin", "python")
LINE_LENGTH = 88

GROUP_DOCS = {
    "terms": "AST nodes for terms.",
    "literals": "AST nodes for literals.",
    "aggregates": "AST nodes for aggregates.",
    "theory": "AST nodes for theory terms and atoms.",
    "body": "AST nodes for body literals.",
    "head": "AST nodes for head literals.",
    "statements": "AST nodes for statements.",
}


def camel_case(name: str) -> str:
    """
    Convert a snake case name into camel case (like the bindings do).
    """
    return "".join(part[:1].upper() + part[1:] for part in name.split("_"))


def docstring(doc: str, indent: str) -> list[str]:
    """
    Format a docstring as a list of lines.
    """
    lines = [f'{indent}"""']
    for line in doc.replace("\\", "\\\\").replace('"""', '\\"\\"\\"').split("\n"):
        lines.append(f"{indent}{line}" if line else "")
    lines.append(f'{indent}"""')
    return lines


def summary(doc: str) -> str:
    """
    Get the first paragraph of a documentation string.
    """
    return doc.split("\n\n", 1)[0]


def wrap_def(head: str, params: list[str], tail: str, indent: str) -> list[str]:
    """
    Format a function definition like black does.
    """
    line = f"{indent}{head}({', '.join(params)}){tail}"
    if len(line) <= LINE_LENGTH:
        return [line]
    line = f"{indent}    {', '.join(params)}"
    if len(line) <= LINE_LENGTH:
        return [f"{indent}{head}(", line, f"{indent}){tail}"]
    return [
        f"{indent}{head}(",
        *(f"{indent}    {param}," for param in params),
        f"{indent}){tail}",
    ]


def wrap_union(name: str, types: list[str]) -> list[str]:
    """
    Format a union type alias like black does.
    """
    line = f"{name}: TypeAlias = {' | '.join(types)}"
    if len(line) <= LINE_LENGTH:
        return [line]
    return [
        f"{name}: TypeAlias = (",
        f"    {types[0]}",
        *(f"    | {t}" for t in types[1:]),
        ")",
    ]


class ASTStubs:
    """
    Generate the stubs of the AST submodules from the AST description.
    """

    def __init__(self, info: dict[str, Any]):
        self._info = info
        self._groups: dict[tuple[str, str], str] = {}
        for kind, key in (
            ("record", "records"),
            ("union", "unions"),
            ("enum", "enums"),
        ):
            for item in info[key]:
                self._groups[(kind, item["name"])] = item["group"]
        self._imports: dict[str, set[str]] = {}
        self._group = ""

    def groups(self) -> list[str]:
        """
        Get the groups in the order of their first appearance.
        """
        res: list[str] = []
        for key in ("enums", "records", "unions"):
            for item in self._info[key]:
                if item["group"] not in res:
                    res.append(item["group"])
        return res

    def _import(self, module: str, name: str) -> str:
        if module != self._group:
            self._imports.setdefault(module, set()).add(name)
        return name

    def _ref(self, ref: tuple[str, str]) -> str:
        kind, name = ref
        group = self._groups[(kind, name)]
        return self._import(f"clingo.ast.{group}", camel_case(name))

    def _type(self, arg: dict[str, Any]) -> str:
        kind = arg["type"]
        if kind == "number":
            return "int"
        if kind == "bool":
            return "bool"
        if kind == "string":
            return "str"
        if kind == "symbol":
            return self._import("clingo.symbol", "Symbol")
        if kind == "location":
            return self._import("clingo.core", "Location")
        if kind == "string_array":
            return f"{self._import('collections.abc', 'Sequence')}[str]"
        if kind == "symbol_array":
            seq = self._import("collections.abc", "Sequence")
            return f"{seq}[{self._import('clingo.symbol', 'Symbol')}]"
        ref = self._ref(arg["ref"])
        if kind == "optional_ast":
            return f"{ref} | None"
        if kind == "ast_array":
            return f"{self._import('collections.abc', 'Sequence')}[{ref}]"
        return ref

    def _default(self, arg: dict[str, Any]) -> str:
        if not arg["has_default"]:
            return ""
        if arg["type"] == "bool":
            return " = True" if arg["default"] else " = False"
        if arg["type"] == "optional_ast":
            return " = None"
        if arg["type"] in ("string_array", "symbol_array", "ast_array"):
            return " = ()"
        return f" = {arg['default']}"

    @staticmethod
    def _arg_doc(name: str, doc: str) -> list[str]:
        paragraphs = doc.split("\n\n") if doc else [""]
        lines = [f"    {name}: {paragraphs[0]}".rstrip()]
        for paragraph in paragraphs[1:]:
            lines.extend(["", f"        {paragraph}"])
        return lines

    def _enum(self, enm: dict[str, Any]) -> list[str]:
        name = camel_case(enm["name"])
        lines = [f"class {name}({self._import('enum', 'IntEnum')}):"]
        lines.extend(docstring(enm["doc"] or f"The {name} enumeration.", "    "))
        lines.append("")
        for value_name, value, doc in enm["values"]:
            lines.append(f"    {camel_case(value_name)} = {value}")
            if doc:
                lines.extend(docstring(doc, "    "))
        return lines

    def _record(self, rec: dict[str, Any]) -> list[str]:
        name = camel_case(rec["name"])
        args = rec["arguments"]
        library = self._import("clingo.core", "Library")
        lines = [
            f"@{self._import('typing', 'final')}",
            f"class {name}({self._import('clingo.ast', 'AST')}):",
        ]
        lines.extend(docstring(rec["doc"] or f"The {name} node.", "    "))
        lines.append("")
        match_args = ", ".join(f'"{arg["name"]}"' for arg in args)
        lines.append(
            f"    __match_args__ = ({match_args}{',' if len(args) == 1 else ''})"
        )
        lines.extend(
            docstring("The names of the arguments for positional patterns.", "    ")
        )
        params = ["self", f"lib: {library}"]
        params.extend(
            f"{arg['name']}: {self._type(arg)}{self._default(arg)}" for arg in args
        )
        lines.extend(wrap_def("def __init__", params, " -> None:", "    "))
        init_doc = [
            "Construct the node.",
            "",
            "Args:",
            "    lib: The library object for storing symbols.",
        ]
        for arg in args:
            init_doc.extend(self._arg_doc(arg["name"], arg["doc"]))
        lines.extend(docstring("\n".join(init_doc), "        "))
        lines.append("")
        params = ["self", f"lib: {library}", "*"]
        params.extend(f"{arg['name']}: {self._type(arg)} = ..." for arg in args)
        lines.extend(wrap_def("def update", params, f" -> {name}:", "    "))
        update_doc = [
            "Create a copy of the node with the given arguments replaced.",
            "",
            "Arguments that are not given keep their current values.",
            "",
            "Args:",
            "    lib: The library object for storing symbols.",
        ]
        for arg in args:
            update_doc.extend(self._arg_doc(arg["name"], summary(arg["doc"])))
        update_doc.extend(["", "Returns:", "    The updated node."])
        lines.extend(docstring("\n".join(update_doc), "        "))
        for arg in args:
            lines.append("")
            lines.append("    @property")
            lines.append(f"    def {arg['name']}(self) -> {self._type(arg)}:")
            lines.extend(
                docstring(arg["doc"] or f"The {arg['name']} of the node.", "        ")
            )
        return lines

    def _union(self, uni: dict[str, Any]) -> list[str]:
        name = camel_case(uni["name"])
        types = [self._ref(tuple(ref)) for ref in uni["types"]]
        self._import("typing", "TypeAlias")
        lines = wrap_union(name, types)
        if uni["doc"]:
            lines.extend(docstring(uni["doc"], ""))
        return lines

    def _header(self) -> list[str]:
        std = [m for m in sorted(self._imports) if not m.startswith("clingo")]
        local = [m for m in sorted(self._imports) if m.startswith("clingo")]
        lines: list[str] = []
        for block in (std, local):
            for module in block:
                lines.append(
                    f"from {module} import {', '.join(sorted(self._imports[module]))}"
                )
            if block:
                lines.append("")
        return lines

    def generate(self, group: str) -> str:
        """
        Generate the stubs of the given group.
        """
        self._group = f"clingo.ast.{group}"
        self._imports = {}
        blocks: list[list[str]] = []
        for enm in self._info["enums"]:
            if enm["group"] == group:
                blocks.append(self._enum(enm))
        for rec in self._info["records"]:
            if rec["group"] == group:
                blocks.append(self._record(rec))
        unions = [
            self._union(uni) for uni in self._info["unions"] if uni["group"] == group
        ]
        lines = docstring(GROUP_DOCS.get(group, "AST nodes."), "")
        lines.append("")
        lines.extend(self._header())
        for block in blocks:
            lines.extend(block)
            lines.append("")
        for union in unions:
            lines.extend(union)
        return "\n".join(lines).rstrip() + "\n"


def format_code(path: Path, content: str) -> str:
    """
    Format the given code with isort and black if available.
    """
    try:
        import black  # pylint: disable=import-outside-toplevel
        import isort  # pylint: disable=import-outside-toplevel
    except ImportError:
        return content
    try:
        content = isort.code(content, profile="black")
        return black.format_file_contents(
            content, fast=False, mode=black.FileMode(is_pyi=True)
        )
    except black.NothingChanged:
        return content
    except Exception as exc:  # pylint: disable=broad-except
        sys.exit(f"failed to format {path}: {exc}")


def import_clingo(build: Path):
    """
    Import the clingo module from the build directory.
    """
    sys.path.insert(0, str(build))
    return importlib.import_module("clingo.ast")


def generate_ast(build: Path, out: Path) -> list[Path]:
    """
    Generate the stubs of the AST submodules into the given directory.
    """
    info = import_clingo(build)._type_info()
    gen = ASTStubs(info)
    out.mkdir(parents=True, exist_ok=True)
    paths = []
    for group in gen.groups():
        path = out / f"{group}.pyi"
        content = gen.generate(group)
        path.write_text(format_code(path, content), encoding="utf8")
        paths.append(path)
    return paths


def is_overload(node: pyast.expr) -> bool:
    """
    Check whether the decorator is `overload` or `typing.overload`.
    """
    return (isinstance(node, pyast.Name) and node.id == "overload") or (
        isinstance(node, pyast.Attribute) and node.attr == "overload"
    )


def strip_overload_implementations(content: str) -> str:
    """
    Remove the implementations following overloaded functions.

    The source stubs keep them because mkdocstrings shows their documentation.
    Type checkers do not accept implementations in stubs though.
    """
    tree = pyast.parse(content)
    remove: list[tuple[int, int]] = []

    def visit(body: list[pyast.stmt]):
        overloaded: set[str] = set()
        for node in body:
            if isinstance(node, pyast.ClassDef):
                visit(node.body)
            elif isinstance(node, (pyast.FunctionDef, pyast.AsyncFunctionDef)):
                if any(is_overload(dec) for dec in node.decorator_list):
                    overloaded.add(node.name)
                elif node.name in overloaded:
                    start = min(
                        [node.lineno, *(dec.lineno for dec in node.decorator_list)]
                    )
                    assert node.end_lineno is not None
                    remove.append((start, node.end_lineno))
                    overloaded.discard(node.name)

    visit(tree.body)
    lines = content.splitlines(keepends=True)
    for start, end in sorted(remove, reverse=True):
        del lines[start - 1 : end]
    return "".join(lines)


REFERENCE_RE = re.compile(r"`(?P<name>clingo(?:\.[A-Za-z_][A-Za-z0-9_]*)*)`")


def link_references(content: str) -> str:
    """
    Turn references like `clingo.ast.AST` into mkdocstrings cross-references.
    """

    def replace(match: re.Match[str]) -> str:
        name = match.group("name")
        return f"[`{name.split('.')[-1]}`][{name}]"

    return REFERENCE_RE.sub(replace, content)


def cmd_ast(args):
    """
    Regenerate the stubs of the AST submodules.
    """
    for path in generate_ast(args.build, args.stubs / "ast"):
        print(f"generated {path}")


def cmd_check(args):
    """
    Check that the stubs of the AST submodules are up to date.
    """
    with tempfile.TemporaryDirectory() as tmp:
        outdated = []
        for path in generate_ast(args.build, Path(tmp)):
            current = args.stubs / "ast" / path.name
            if not current.exists() or current.read_text(
                encoding="utf8"
            ) != path.read_text(encoding="utf8"):
                outdated.append(str(current))
    if outdated:
        sys.exit(
            "outdated stubs (run `python scripts/stubs.py ast`): " + ", ".join(outdated)
        )
    print("stubs are up to date")


def copy_stubs(
    source: Path, target: Path, transform: Callable[[str], str]
) -> list[Path]:
    """
    Copy the stubs in source to target applying the transformation.

    The target directory is replaced so that removed stubs do not linger.
    """
    if target.exists():
        shutil.rmtree(target)
    paths = []
    for path in sorted(source.rglob("*.pyi")):
        dst = target / path.relative_to(source)
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_text(transform(path.read_text(encoding="utf8")), encoding="utf8")
        paths.append(dst)
    return paths


def cmd_strip(args):
    """
    Copy the stubs removing the implementations of overloaded functions.
    """
    paths = copy_stubs(args.stubs, args.output, strip_overload_implementations)
    print(f"stripped {len(paths)} stubs into {args.output}")


def cmd_install(args):
    """
    Install the stubs and link the built module into the environment.
    """
    target = Path(args.prefix) / "clingo-stubs"
    for path in copy_stubs(args.stubs, target, strip_overload_implementations):
        print(f"installed {path}")
    for lib in glob(str(args.build / "clingo*.so")):
        dst = Path(args.prefix) / os.path.basename(lib)
        if not dst.exists():
            dst.symlink_to(os.path.relpath(lib, args.prefix))
            print(f"linked {dst}")


def cmd_doc(args):
    """
    Copy the stubs for the documentation turning references into links.

    The overload implementations are kept because mkdocstrings shows their
    documentation.
    """
    for path in copy_stubs(args.stubs, args.output, link_references):
        print(f"copied {path}")


def main():
    """
    Parse the command line and run the selected command.
    """
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--build",
        type=Path,
        default=BUILD,
        help="directory containing the built module",
    )
    parser.add_argument(
        "--stubs", type=Path, default=STUBS, help="directory containing the stubs"
    )
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser(
        "ast", help="regenerate the stubs of the AST submodules"
    ).set_defaults(run=cmd_ast)
    sub.add_parser(
        "check", help="check that the AST stubs are up to date"
    ).set_defaults(run=cmd_check)
    strip = sub.add_parser(
        "strip", help="copy the stubs without overload implementations"
    )
    strip.add_argument(
        "--output", type=Path, required=True, help="directory for the stubs"
    )
    strip.set_defaults(run=cmd_strip)
    install = sub.add_parser(
        "install", help="install stubs and module into the environment"
    )
    install.add_argument(
        "--prefix", default=sysconfig.get_path("purelib"), help="installation directory"
    )
    install.set_defaults(run=cmd_install)
    doc = sub.add_parser("doc", help="copy the stubs for the documentation")
    doc.add_argument(
        "--output", type=Path, default=DOC_STUBS, help="directory for the stubs"
    )
    doc.set_defaults(run=cmd_doc)
    args = parser.parse_args()
    args.run(args)


if __name__ == "__main__":
    main()
