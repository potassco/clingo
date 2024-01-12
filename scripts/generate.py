import json

from clingo.ast import _type_info_json


def snake_to_camel(name):
    return "".join(x.title() for x in name.split("_"))


def generate_location_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> clingo_location_t;"""


def generate_location_define_cpp(type_name, arg):
    return f"""\
auto {type_name}::{arg["name"]}() -> clingo_location_t {{
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
        throw std::runtime_error("could not get location attribute");
    }}
    return ret;
}}
"""


def generate_number_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> int;"""


def generate_number_define_cpp(type_name, arg):
    return f"""\
auto {type_name}::{arg["name"]}() -> int {{
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
        throw std::runtime_error("could not get number attribute");
    }}
    return ret;
}}
"""


def generate_bool_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> bool;"""


def generate_bool_define_cpp(type_name, arg):
    return f"""\
auto {type_name}::{arg["name"]}() -> bool {{
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
        throw std::runtime_error("could not get number attribute");
    }}
    return ret != 0;
}}
"""


def generate_enum_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> {snake_to_camel(arg["type"])};"""


def generate_enum_define_cpp(type_name, arg):
    type_ = snake_to_camel(arg["type"])
    return f"""\
auto {type_name}::{arg["name"]}() -> {type_} {{
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
        throw std::runtime_error("could not get number attribute");
    }}
    return static_cast<{type_}>(ret);
}}
"""


def generate_string_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> char const *;"""


def generate_string_define_cpp(type_name, arg):
    return f"""\
auto {type_name}::{arg["name"]}() -> char const * {{
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
        throw std::runtime_error("could not get string attribute");
    }}
    return ret;
}}
"""


def generate_symbol_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> Symbol;"""


def generate_symbol_define_cpp(type_name, arg):
    return f"""\
auto {type_name}::{arg["name"]}() -> Symbol {{
    clingo_symbol_t ret;
    if (!clingo_ast_attribute_get_symbol(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
        throw std::runtime_error("could not get symbol attribute");
    }}
    return Symbol::acquire(ret);
}}
"""


def attribute_array_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> {snake_to_camel(arg["type"])};"""


def attribute_array_define_cpp(type_name, arg):
    return f"""\
auto {type_name}::{arg["name"]}() -> {snake_to_camel(arg["type"])} {{
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_{arg["name"]}, &ast, &size)) {{
        throw std::runtime_error("could not get ast array attribute");
    }}
    return construct_{arg["type"]}(ast, size);
}}
"""


def union_attribute_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> {snake_to_camel(arg["type"])};"""


def union_attribute_define_cpp(type_name, type_attr, arg):
    return f"""\
auto {type_name}::{arg["name"]}() -> {snake_to_camel(arg["type"])} {{
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_{arg["name"]}, &ast)) {{
        throw std::runtime_error("could not get ast attribute");
    }}
    return construct_{arg["type"]}(ast);
}}
"""


def generate_record_define_cpp(type_dict, type_name, args):
    decl = ""
    defs = ""
    for arg in args:
        indent = "    "
        if arg["type"] == "location":
            decl += indent + generate_location_declare_cpp(arg) + "\n"
            defs += generate_location_define_cpp(type_name, arg)
        elif arg["type"] == "string":
            decl += indent + generate_string_declare_cpp(arg) + "\n"
            defs += generate_string_define_cpp(type_name, arg)
        elif arg["type"] == "number":
            decl += indent + generate_number_declare_cpp(arg) + "\n"
            defs += generate_number_define_cpp(type_name, arg)
        elif arg["type"] == "bool":
            decl += indent + generate_bool_declare_cpp(arg) + "\n"
            defs += generate_bool_define_cpp(type_name, arg)
        elif arg["type"] == "symbol":
            decl += indent + generate_symbol_declare_cpp(arg) + "\n"
            defs += generate_symbol_define_cpp(type_name, arg)
        elif type_dict[arg["type"]]["type"] == "enum":
            decl += generate_enum_declare_cpp(arg) + "\n"
            defs += indent + generate_enum_define_cpp(type_name, arg)
        elif type_dict[arg["type"]]["type"] == "union":
            decl += indent + union_attribute_declare_cpp(arg) + "\n"
            defs += union_attribute_define_cpp(type_name, type_dict[arg["type"]], arg)
        elif type_dict[arg["type"]]["type"] == "array":
            decl += indent + attribute_array_declare_cpp(arg) + "\n"
            defs += attribute_array_define_cpp(type_name, arg)
        else:
            pass
            # print("handle:", arg["type"])
    return (
        f"""
class {type_name} {{
public:
{decl}\
    static auto acquire(clingo_ast_t *ast) -> {type_name} {{
        return {{ast}};
    }}
    ~{type_name}() {{
        clingo_ast_free(ast_);
    }}
private:
    {type_name}(clingo_ast_t *ast) : ast_{{ast}} {{}}
    clingo_ast_t *ast_;
}};
""",
        defs,
    )


def generate_property_reg(type_name, arg):
    return f"""\
        .def_property_readonly("{arg["name"]}", &{type_name}::{arg["name"]})
"""


def generate_record_reg(type_dict, type_name, args):
    attr = ""
    for arg in args:
        if arg["type"] in (
            "location",
            "string",
            "number",
            "bool",
            "symbol",
        ) or type_dict[arg["type"]]["type"] in ("enum", "union", "array"):
            attr += generate_property_reg(type_name, arg)
    return f"""\
    py::class_<{type_name}>(ast, "{type_name}", R"(TODO.)")
{attr}        ;

"""


def union_declare_cpp(arg):
    return f"""auto construct_{arg["name"]}(clingo_ast_t *ast) -> {snake_to_camel(arg["name"])};"""


def union_define_cpp(arg, type_dict):
    type_ = snake_to_camel(arg["name"])
    cases = ""
    types = []

    def flatten_type(t):
        type_info = type_dict[t]
        if type_info["type"] in ("record", "forward"):
            types.append(t)
        elif type_info["type"] == "union":
            for u in type_info["types"]:
                flatten_type(u)
        else:
            raise RuntimeError("unhandled type")

    for result_type in arg["types"]:
        flatten_type(result_type)

    for result_type in types:
        cases += f"""\
        case clingo_ast_type_{result_type}: {{
            return {snake_to_camel(result_type)}::acquire(ast);
        }}
"""
    return f"""\
auto construct_{arg["name"]}(clingo_ast_t *ast) -> {type_} {{
    clingo_ast_type_t type;
    if (!clingo_ast_get_type(ast, &type)) {{
        clingo_ast_free(ast);
        throw std::runtime_error("could not get type");
    }}
    switch (type) {{
{cases}\
    }}
    throw std::runtime_error("unexpected ast type");
}}
"""


def array_declare_cpp(arg):
    return f"""auto construct_{arg["name"]}(clingo_ast_t **ast, size_t size) -> {snake_to_camel(arg["name"])};"""


def array_define_cpp(arg, type_dict):
    if type_dict[arg["value_type"]]["type"] == "union":
        cons = f'construct_{arg["value_type"]}'
    elif type_dict[arg["value_type"]]["type"] in ("forward", "record"):
        cons = f'{snake_to_camel(arg["value_type"])}::acquire'
    else:
        raise RuntimeError("unhandled type")
    return f"""\
auto construct_{arg["name"]}(clingo_ast_t **ast, size_t size) -> {snake_to_camel(arg["name"])} {{
    {snake_to_camel(arg["name"])} ret;
    try {{
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg){{
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back({cons}(tmp));
        }});
        clingo_ast_array_free(ast, size);
    }}
    catch (...) {{
        clingo_ast_array_free(ast, size);
        throw;
    }}
    return ret;
}}
"""


def generate():
    types = json.loads(_type_info_json())
    defines = ""
    preamble = ""
    register = ""

    type_dict = {}

    for type_attr in types:
        type_dict[type_attr["name"]] = type_attr
        type_name = snake_to_camel(type_attr["name"])
        if type_attr["type"] == "forward":
            preamble += f"class {type_name};\n\n"

        if type_attr["type"] == "optional":
            value_type = snake_to_camel(type_attr["value_type"])
            preamble += f"using {type_name} = std::optional<{value_type}>;\n\n"

        if type_attr["type"] == "array":
            value_type = snake_to_camel(type_attr["value_type"])
            preamble += f"using {type_name} = std::vector<{value_type}>;\n"
            preamble += array_declare_cpp(type_attr) + "\n\n"
            defines += array_define_cpp(type_attr, type_dict)

        if type_attr["type"] == "union":
            types = ", ".join(snake_to_camel(x) for x in type_attr["types"])
            preamble += f"using {type_name} = std::variant<{types}>;\n"
            preamble += union_declare_cpp(type_attr) + "\n\n"
            defines += union_define_cpp(type_attr, type_dict)

        if type_attr["type"] == "enum":
            preamble += f"enum class {type_name} {{\n"
            register += f"""    py::enum_<{type_name}>(ast, "{type_name}", R"({type_attr["doc"]})")\n"""
            for value_name, value_attr in type_attr["values"].items():
                value_name = snake_to_camel(value_name)
                value = f"{type_name}::{value_name}"
                register += f"""        .value("{value_name}", {value}, R"({value_attr["doc"]})")\n"""
                preamble += f'    {value_name} = {value_attr["value"]},\n'
            register += "        ;\n\n"
            preamble += "};\n\n"

        if type_attr["type"] == "record":
            decl, defs = generate_record_define_cpp(
                type_dict, type_name, type_attr["arguments"]
            )
            preamble += decl
            defines += defs
            register += generate_record_reg(
                type_dict, type_name, type_attr["arguments"]
            )

    result = f"""\
#pragma once

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include "core.hh"
#include "symbol.hh"

namespace Clingo::AST {{

namespace py = pybind11;

using Clingo::Symbol::Symbol;

struct Position {{
    char const *file;
    size_t line;
    size_t column;
}};
\
{preamble}
{defines}
void register_module(pybind11::module &m) {{
    auto ast = m.def_submodule("ast", doc(R"(
TODO
)"));

    ast.def("_type_info_json", &clingo_ast_type_info_json, doc(R"(
TODO
)"));

    py::class_<Position>(ast, "Position", R"(Position tracking object.)")
        .def_readonly("file", &Position::file)
        .def_readonly("line", &Position::line)
        .def_readonly("column", &Position::column)
        ;

    py::class_<clingo_location_t>(ast, "Location", R"(Location tracking object.)")
        .def_property_readonly("begin", [](clingo_location_t const &loc) {{
            return Position{{loc.begin_file, loc.begin_line, loc.begin_column}}; }})
        .def_property_readonly("end", [](clingo_location_t const &loc) {{
            return Position{{loc.end_file, loc.end_line, loc.end_column}}; }})
        ;\
{register}\
}}

}}\
"""

    return result


print(generate())
