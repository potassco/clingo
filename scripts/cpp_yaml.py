import re
from dataclasses import dataclass

import yaml
from clingo.ast import _type_info_yaml


@dataclass
class Node:
    name: str
    arguments: list[tuple[str, str]]


def parse_types():
    pattern = re.compile(r"\bclingo_ast_type_[a-zA-Z0-9_]+(?=,|\s*})")
    matches = pattern.findall(open("lib/c-api/include/clingo/ast.h").read())
    i = 0
    types = {}
    for m in matches:
        types[m.replace("clingo_ast_type_", "")] = i
        i += 1
    return types


def parse_nodes() -> list[Node]:
    types = yaml.safe_load(_type_info_yaml())

    nodes = []
    type_map = {}

    for node_name, node in types.items():
        type_map[node_name] = node

    type_remap = {
        "enum": "integer",
        "number": "integer",
        "bool": "integer",
        "record": "node",
        "union": "node",
        "array": "node_array",
        "optional": "optional_node",
    }

    for node_name, node in types.items():
        if node["type"] == "record":
            arguments = []
            for argument_name, argument in node["arguments"].items():
                argument_type = argument["type"]
                if argument_type in type_map:
                    argument_type = type_map[argument_type]["type"]
                arguments.append(
                    (argument_name, type_remap.get(argument_type, argument_type))
                )

            nodes.append(Node(node_name, arguments))

    return nodes


def main():
    types = parse_types()
    nodes = sorted(parse_nodes(), key=lambda x: types[x.name])
    print(
        "enum class ArgType { node, node_array, optional_node, integer, symbol, symbol_array, string, string_array, location };"
    )
    print("struct Arg { clingo_ast_attribute_t attr; ArgType type; };")
    for node in nodes:
        args = []
        for an, at in node.arguments:
            args.append(f"Arg{{clingo_ast_attribute_{an}, ArgType::{at}}}")
        print(f"constexpr auto const_{node.name} = std::array{{{', '.join(args)}}};")
    print("};")
    print("constexpr auto cons = std::array{")
    for node in nodes:
        print(f"    std::span{{cons_{node.name}.data(), cons_{node.name}.size()}},")
    print("};")


main()
