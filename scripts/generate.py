import json

from clingo.ast import _type_info_json


def snake_to_camel(name):
    return "".join(x.title() for x in name.split("_"))


def generate_location_pre(arg):
    return f"""\
    auto {arg["name"]}() -> clingo_location_t {{
        clingo_location_t ret;
        if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
            throw std::runtime_error("could not get location attribute");
        }}
        return ret;
    }}
"""


def generate_number_pre(arg):
    return f"""\
    auto {arg["name"]}() -> int {{
        int ret;
        if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
            throw std::runtime_error("could not get number attribute");
        }}
        return ret;
    }}
"""


def generate_bool_pre(arg):
    return f"""\
    auto {arg["name"]}() -> bool {{
        int ret;
        if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
            throw std::runtime_error("could not get number attribute");
        }}
        return ret != 0;
    }}
"""


def generate_enum_pre(arg):
    type_ = snake_to_camel(arg["type"])
    return f"""\
    auto {arg["name"]}() -> {type_} {{
        int ret;
        if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
            throw std::runtime_error("could not get number attribute");
        }}
        return static_cast<{type_}>(ret);
    }}
"""


def generate_string_pre(arg):
    return f"""\
    auto {arg["name"]}() -> char const * {{
        char const *ret;
        if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
            throw std::runtime_error("could not get string attribute");
        }}
        return ret;
    }}
"""


def generate_symbol_pre(arg):
    return f"""\
    auto {arg["name"]}() -> Symbol {{
        clingo_symbol_t ret;
        if (!clingo_ast_attribute_get_symbol(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
            throw std::runtime_error("could not get symbol attribute");
        }}
        return Symbol::acquire(ret);
    }}
"""


def generate_union_pre(type_attr, arg):
    # TODO: needs to be declared then defined
    type_ = snake_to_camel(arg["type"])
    cases = ""
    for result_type in type_attr["types"]:
        cases += f"""\
            case clingo_ast_type_{result_type}: {{
                return {snake_to_camel(result_type)}::acquire(ret);
            }}
"""
    return f"""\
    auto {arg["name"]}() -> {type_} {{
        clingo_ast_t *ret;
        if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
            throw std::runtime_error("could not get ast attribute");
        }}
        clingo_ast_type_t type;
        if (!clingo_ast_get_type(ret, &type)) {{
            clingo_ast_free(ret);
            throw std::runtime_error("could not get type");
        }}
        switch (type) {{
{cases}        }}
        throw std::runtime_error("unexpected ast type");
    }}
"""


def generate_record_pre(type_dict, type_name, args):
    attr = ""
    for arg in args:
        if arg["type"] == "location":
            attr += generate_location_pre(arg)
        elif arg["type"] == "string":
            attr += generate_string_pre(arg)
        elif arg["type"] == "number":
            attr += generate_number_pre(arg)
        elif arg["type"] == "bool":
            attr += generate_bool_pre(arg)
        elif arg["type"] == "symbol":
            attr += generate_symbol_pre(arg)
        elif type_dict[arg["type"]]["type"] == "enum":
            attr += generate_enum_pre(arg)
        elif type_dict[arg["type"]]["type"] == "union":
            attr += generate_union_pre(type_dict[arg["type"]], arg)
        else:
            pass
            # print("handle:", arg["type"])
    return f"""
class {type_name} {{
public:
{attr}\
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
"""


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
        ) or type_dict[arg["type"]]["type"] in ("enum", "union"):
            attr += generate_property_reg(type_name, arg)
    return f"""\
    py::class_<{type_name}>(ast, "{type_name}", R"(TODO.)")
{attr}        ;

"""


def generate():
    types = json.loads(_type_info_json())
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

        if type_attr["type"] == "union":
            types = ", ".join(snake_to_camel(x) for x in type_attr["types"])
            preamble += f"using {type_name} = std::variant<{types}>;\n\n"

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
            preamble += generate_record_pre(
                type_dict, type_name, type_attr["arguments"]
            )
            register += generate_record_reg(
                type_dict, type_name, type_attr["arguments"]
            )

    result = (
        """\
#pragma once

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include "core.hh"
#include "symbol.hh"

namespace Clingo::AST {

namespace py = pybind11;

using Clingo::Symbol::Symbol;

struct Position {
    char const *file;
    size_t line;
    size_t column;
};

"""
        + preamble
        + """\
void register_module(pybind11::module &m) {
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
        .def_property_readonly("begin", [](clingo_location_t const &loc) {
            return Position{loc.begin_file, loc.begin_line, loc.begin_column}; })
        .def_property_readonly("end", [](clingo_location_t const &loc) {
            return Position{loc.end_file, loc.end_line, loc.end_column}; })
        ;
"""
        + register
        + "}\n\n}"
    )

    return result


print(generate())
