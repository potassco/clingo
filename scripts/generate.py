#!/usr/bin/env python3
"""
Script to generate python ast module.
"""
from textwrap import wrap

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
        elif argument["type"] == "string_array":
            res.append(f'c_cast({argument["name"]}).data()')
            res.append(f'{argument["name"]}.size()')
        elif type_map[argument["type"]]["type"] == "enum":
            res.append(f'static_cast<int>({argument["name"]})')
        elif type_map[argument["type"]]["type"] in ("union", "record", "optional"):
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


def forward(types, current_type, type_list):
    """
    Filter the types that have to be forwarded.
    """
    res = []

    def check(name):
        for t in type_list:
            if t["type"] == "forward":
                continue
            if t["name"] == name:
                break
            if t["name"] == current_type:
                res.append(name)
                break

    if isinstance(types, str):
        check(types)
    else:
        for name in types:
            check(name)
    return res


def doc(text):
    """
    Wrap a docstring.
    """
    return "\n\n".join("\n".join(wrap(par, width=70)) for par in text.splitlines())


def param_doc(text):
    """
    Wrap and indent a parameter docstring.
    """
    return "\n\n".join(
        "\n".join(wrap(par, width=70, initial_indent="    ", subsequent_indent="    "))
        for par in text.splitlines()
    )


def generate():
    """
    Generate the python ast module.
    """
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(searchpath="scripts/"))
    env.filters["camel"] = camel
    env.filters["cref"] = cref
    env.filters["c_cast"] = c_cast
    env.filters["flatten_types"] = flatten_types
    env.filters["doc"] = doc
    env.filters["param_doc"] = param_doc
    env.filters["forward"] = forward

    types = yaml.safe_load(_type_info_yaml())
    type_map = {type_attr["name"]: type_attr for type_attr in types}

    return env.get_template("ast_module.j2").render(types=types, type_map=type_map)


print(generate())
