"""
Print the constructor argument tables of the C++ API (cxx-api/include/clingo/detail/ast.hh).

The tables are derived from the description of the AST structure provided by
`clingo.ast._type_info()`, which lists the records in the order of
`clingo_ast_type_e`.
"""

from clingo.ast import _type_info

TYPE_MAP = {
    "number": "integer",
    "bool": "integer",
    "enum": "integer",
    "symbol": "symbol",
    "symbol_array": "symbol_array",
    "string": "string",
    "string_array": "string_array",
    "location": "location",
    "ast": "node",
    "optional_ast": "optional_node",
    "ast_array": "node_array",
}


def main():
    """
    Print the tables.
    """
    records = _type_info()["records"]
    for rec in records:
        args = [
            f"Arg{{clingo_ast_attribute_{arg['name']}, Arg::{TYPE_MAP[arg['type']]}}}"
            for arg in rec["arguments"]
        ]
        print(f"constexpr auto cons_{rec['name']} = std::array{{{', '.join(args)}}};")
    print("constexpr auto cons = std::array{")
    for rec in records:
        print(f"    std::span{{cons_{rec['name']}.data(), cons_{rec['name']}.size()}},")
    print("};")


main()
