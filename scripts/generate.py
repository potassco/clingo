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


def cval(name):
    """
    Convert the given type to a C++ value.
    """
    if name == "location":
        return "Location"
    if name == "string":
        return "std::string_view"
    if name == "number":
        return "int "
    if name == "bool":
        return "bool "
    if name == "symbol":
        return "Symbol"
    return f"{camel(name)}"


def cref(name):
    """
    Convert the given type to a C++ const reference.
    """
    if name == "location":
        return "Location const &"
    if name == "string":
        return "std::string_view "
    if name == "number":
        return "int "
    if name == "bool":
        return "bool "
    if name == "symbol":
        return "Symbol const &"
    return f"{camel(name)} const &"


def cpar(name: str):
    """
    Convert the given type to a C++ const reference.
    """
    return cref(name).replace("Array", "Iterable")


def c_cast(arguments, type_map):
    """
    Convert the given C++ types to the clingo C API equivalent.
    """
    res = []
    for argument_name, argument in arguments.items():
        if argument["type"] == "location":
            res.append(f"static_cast<clingo_location_t const *>({argument_name})")
        elif argument["type"] == "string":
            res.append(f"{argument_name}.data()")
            res.append(f"{argument_name}.size()")
        elif argument["type"] == "number":
            res.append(f"{argument_name}")
        elif argument["type"] == "bool":
            res.append(f"static_cast<int>({argument_name})")
        elif argument["type"] == "symbol":
            res.append(f"{argument_name}.handle()")
        elif argument["type"] == "string_array":
            res.append(f"c_cast({argument_name}).data()")
            res.append(f"{argument_name}.size()")
        elif argument["type"] == "symbol_array":
            res.append(f"c_cast({argument_name}).data()")
            res.append(f"{argument_name}.size()")
        elif type_map[argument["type"]]["type"] == "enum":
            res.append(f"static_cast<int>({argument_name})")
        elif type_map[argument["type"]]["type"] in ("union", "record", "optional"):
            res.append(f"c_cast({argument_name})")
        elif type_map[argument["type"]]["type"] == "array":
            res.append(f"c_cast({argument_name}).data()")
            res.append(f"{argument_name}.size()")
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
        for other in type_list:
            if other == name:
                break
            if other == current_type:
                res.append(name)
                break

    if isinstance(types, str):
        check(types)
    else:
        for name in types:
            check(name)
    return res


def transform_cond(arguments, types):
    """
    Build condition when to transform.
    """
    res = []
    for argument_name, argument in arguments.items():
        if argument["type"] in types and types[argument["type"]]["type"] in (
            "union",
            "record",
            "optional",
            "array",
        ):
            res.append(f"{argument_name}_changed")
    if not res:
        res.append("false")
    return " || ".join(res)


def transform_cons(arguments, types):
    """
    Build condition when to transform.
    """
    res = ["lib"]
    for argument_name, argument in arguments.items():
        if argument["type"] in types and types[argument["type"]]["type"] in (
            "union",
            "record",
            "optional",
            "array",
        ):
            res.append(f"{argument_name}_value")
        elif argument["type"] == "string_array":
            res.append(f"to_string_array({argument_name}())")
        else:
            res.append(f"{argument_name}()")
    return ", ".join(res)


def doc(text):
    """
    Wrap a docstring.
    """
    return "\n\n".join("\n".join(wrap(par, width=70)) for par in text.splitlines())


def param_doc(text):
    """
    Wrap and indent a parameter docstring.
    """
    sps = "        "
    res = "\n\n".join(
        "\n".join(wrap(par, width=70, initial_indent=sps, subsequent_indent=sps))
        for par in text.splitlines()
    )
    return res.lstrip(" ")


def make_comp(types):
    """
    Generate comparisions operators for a given type.
    """
    unions = {}
    for name, data in types.items():
        if data["type"] == "union":
            for member in data["types"]:
                unions[member] = name

    def comp(text):
        if text in unions:
            return f"make_comparable_base<{camel(unions[text])}>"
        return "make_comparable"

    return comp


def generate():
    """
    Generate the python ast module.
    """
    types = yaml.safe_load(_type_info_yaml())

    env = jinja2.Environment(loader=jinja2.FileSystemLoader(searchpath="scripts/"))
    env.filters["camel"] = camel
    env.filters["cref"] = cref
    env.filters["cval"] = cval
    env.filters["cpar"] = cpar
    env.filters["c_cast"] = c_cast
    env.filters["flatten_types"] = flatten_types
    env.filters["doc"] = doc
    env.filters["param_doc"] = param_doc
    env.filters["forward"] = forward
    env.filters["transform_cond"] = transform_cond
    env.filters["transform_cons"] = transform_cons
    env.filters["comp"] = make_comp(types)

    return env.get_template("ast_module.j2").render(types=types)


print(generate())
