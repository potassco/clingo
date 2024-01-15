#!/usr/bin/env python3
"""
Script to generate python ast module.
"""

import jinja2
import yaml
from clingo.ast import _type_info_yaml


def camel(name):
    """
    Convert snake to camel case.
    """
    return "".join(x.title() for x in name.split("_"))


def cref(name):
    """
    Convert the given type to a C++ const reference.
    """
    if name == "location":
        return "clingo_location_t const &"
    if name == "string":
        return "char const *"
    if name == "number":
        return "int "
    if name == "bool":
        return "bool "
    if name == "symbol":
        return "Symbol const &"
    return f"{camel(name)} const &"


def c_cast(arguments, type_map):
    """
    Convert the given C++ types to the clingo C API equivalent.
    """
    res = []
    for argument in arguments:
        if argument["type"] == "location":
            res.append(f'&{argument["name"]}')
        elif argument["type"] == "string":
            res.append(f'{argument["name"]}')
        elif argument["type"] == "number":
            res.append(f'{argument["name"]}')
        elif argument["type"] == "bool":
            res.append(f'static_cast<int>({argument["name"]})')
        elif argument["type"] == "symbol":
            res.append(f'{argument["name"]}.handle()')
        elif type_map[argument["type"]]["type"] == "enum":
            res.append(f'static_cast<int>({argument["name"]})')
        elif type_map[argument["type"]]["type"] == "union":
            res.append(f'c_cast({argument["name"]})')
        elif type_map[argument["type"]]["type"] == "array":
            res.append(f'c_cast({argument["name"]}).data()')
            res.append(f'{argument["name"]}.size()')
        else:
            assert False
    return res


def flatten_types(types, type_map):
    """
    Flatten an array of nested union types.
    """
    res = []

    def flatten_type(name):
        type_info = type_map[name]
        if type_info["type"] in ("record", "forward"):
            res.append(name)
        elif type_info["type"] == "union":
            for u in type_info["types"]:
                flatten_type(u)
        else:
            raise RuntimeError("unhandled type")

    for name in types:
        flatten_type(name)

    return res


def property_attribute_reg(type_name, arg):
    return f"""\
        .def_property_readonly("{arg["name"]}", &{type_name}::{arg["name"]})
"""


def record_reg(type_dict, type_name, args):
    attr = ""
    attr += f"""\
        .def(py::init(&{type_name}::construct))
        .def("__str__", &{type_name}::to_string)
"""
    for arg in args:
        attr += property_attribute_reg(type_name, arg)
    return f"""\
    py::class_<{type_name}>(ast, "{type_name}", R"(TODO.)")
{attr}\
        .def("__hash__", &{type_name}::hash)
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

"""


def generate():
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(searchpath="scripts/"))
    env.filters["camel"] = camel
    env.filters["cref"] = cref
    env.filters["c_cast"] = c_cast
    env.filters["flatten_types"] = flatten_types

    types = yaml.safe_load(_type_info_yaml())
    register = ""

    type_map = {type_attr["name"]: type_attr for type_attr in types}

    for type_attr in types:
        type_attr["pyname"] = camel(type_attr["name"])
        type_name = camel(type_attr["name"])

        if type_attr["type"] == "enum":
            register += f"""    py::enum_<{type_name}>(ast, "{type_name}", R"({type_attr["doc"]})")\n"""
            for value_name, value_attr in type_attr["values"].items():
                value_name = camel(value_name)
                value = f"{type_name}::{value_name}"
                register += f"""        .value("{value_name}", {value}, R"({value_attr["doc"]})")\n"""
            register += "        ;\n\n"

        if type_attr["type"] == "record":
            register += record_reg(type_map, type_name, type_attr["arguments"])

    return env.get_template("module.j2").render(
        register=register, types=types, type_map=type_map
    )


print(generate())
