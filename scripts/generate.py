import json

from clingo.ast import _type_info_json


def snake_to_camel(name):
    return "".join(x.title() for x in name.split("_"))


def generate():
    types = json.loads(_type_info_json())
    preamble = ""
    register = ""

    for type_attr in types:
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
            register += "    ;\n\n"
            preamble += "};\n\n"

        if type_attr["type"] == "record":
            preamble += f"class {type_name} {{\n"
            preamble += "public:\n"
            preamble += "private:\n"
            preamble += "    clingo_ast_t *ast_; // NOLINT\n"
            preamble += "};\n\n"

    result = (
        """\
#pragma once

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include "core.hh"

namespace Clingo::AST {

namespace py = pybind11;

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

"""
        + register
        + "}\n\n}"
    )

    return result


print(generate())
