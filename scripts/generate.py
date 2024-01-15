import jinja2
import yaml
from clingo.ast import _type_info_yaml


def snake_to_camel(name):
    return "".join(x.title() for x in name.split("_"))


def cref(name):
    if name == "location":
        return "clingo_location_t const &"
    if name == "string":
        return "char const *"
    if name == "number":
        return "int "
    if name == "bool":
        return "bool "
    if name == "symbol ":
        return "Symbol const &"
    return f"{snake_to_camel(name)} const &"


def flatten_types(types, type_map):
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


ENV = jinja2.Environment(loader=jinja2.FileSystemLoader(searchpath="scripts/"))
ENV.filters["camel"] = snake_to_camel
ENV.filters["cref"] = cref
ENV.filters["flatten_types"] = flatten_types


def location_attribute_define_cpp(type_name, arg):
    return f"""\
auto {type_name}::{arg["name"]}() -> clingo_location_t {{
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
        throw std::runtime_error("could not get location attribute");
    }}
    return ret;
}}

"""


def number_attribute_define_cpp(type_name, arg):
    return f"""\
auto {type_name}::{arg["name"]}() -> int {{
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
        throw std::runtime_error("could not get number attribute");
    }}
    return ret;
}}

"""


def bool_attribute_define_cpp(type_name, arg):
    return f"""\
auto {type_name}::{arg["name"]}() -> bool {{
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
        throw std::runtime_error("could not get number attribute");
    }}
    return ret != 0;
}}

"""


def enum_attribute_define_cpp(type_name, arg):
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


def string_attribute_define_cpp(type_name, arg):
    return f"""\
auto {type_name}::{arg["name"]}() -> char const * {{
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
        throw std::runtime_error("could not get string attribute");
    }}
    return ret;
}}

"""


def symbol_attribute_define_cpp(type_name, arg):
    return f"""\
auto {type_name}::{arg["name"]}() -> Symbol {{
    clingo_symbol_t ret;
    if (!clingo_ast_attribute_get_symbol(ast_, clingo_ast_attribute_{arg["name"]}, &ret)) {{
        throw std::runtime_error("could not get symbol attribute");
    }}
    return Symbol::acquire(ret);
}}

"""


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


def record_define_cpp(type_dict, type_name, type_attr):
    args = type_attr["arguments"]
    defs = ""
    cons_args = ["Library const &lib"]
    cons_def_args = ["lib", f'clingo_ast_type_{type_attr["name"]}', "&res_"]
    for arg in args:
        if arg["type"] == "location":
            cons_args.append("clingo_location_t const &" + arg["name"])
            cons_def_args.append(f'&{arg["name"]}')
            defs += location_attribute_define_cpp(type_name, arg)
        elif arg["type"] == "string":
            cons_args.append("char const *" + arg["name"])
            cons_def_args.append(f'{arg["name"]}')
            defs += string_attribute_define_cpp(type_name, arg)
        elif arg["type"] == "number":
            cons_args.append("int " + arg["name"])
            cons_def_args.append(f'{arg["name"]}')
            defs += number_attribute_define_cpp(type_name, arg)
        elif arg["type"] == "bool":
            cons_args.append("bool " + arg["name"])
            cons_def_args.append(f'static_cast<int>({arg["name"]})')
            defs += bool_attribute_define_cpp(type_name, arg)
        elif arg["type"] == "symbol":
            cons_args.append("Symbol const &" + arg["name"])
            cons_def_args.append(f'{arg["name"]}.handle()')
            defs += symbol_attribute_define_cpp(type_name, arg)
        elif type_dict[arg["type"]]["type"] == "enum":
            cons_args.append(snake_to_camel(arg["type"]) + " const &" + arg["name"])
            cons_def_args.append(f'static_cast<int>({arg["name"]})')
            defs += enum_attribute_define_cpp(type_name, arg)
        elif type_dict[arg["type"]]["type"] == "union":
            cons_args.append(snake_to_camel(arg["type"]) + " const &" + arg["name"])
            cons_def_args.append(f'c_cast({arg["name"]})')
            defs += union_attribute_define_cpp(type_name, type_dict[arg["type"]], arg)
        elif type_dict[arg["type"]]["type"] == "array":
            cons_args.append(snake_to_camel(arg["type"]) + " const &" + arg["name"])
            cons_def_args.append(f'c_cast({arg["name"]}).data()')
            cons_def_args.append(f'{arg["name"]}.size()')
            defs += attribute_array_define_cpp(type_name, arg)
        else:
            pass
            # print("handle:", arg["type"])
    defs += f"""\
auto {type_name}::construct({", ".join(cons_args)}) -> {type_name} {{
    clingo_ast_t *res_;
    if (!clingo_ast_construct({", ".join(cons_def_args)})) {{
        throw std::runtime_error("better handle error properly here");
    }}
    return {type_name}::acquire(res_);
}}

"""
    return (
        ENV.get_template("record_declare_cpp.j2").render(
            type=type_attr, types=type_dict
        ),
        defs,
    )


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
    types = yaml.safe_load(_type_info_yaml())
    register = ""

    type_map = {type_attr["name"]: type_attr for type_attr in types}

    for type_attr in types:
        type_attr["pyname"] = snake_to_camel(type_attr["name"])
        type_name = snake_to_camel(type_attr["name"])

        if type_attr["type"] == "enum":
            register += f"""    py::enum_<{type_name}>(ast, "{type_name}", R"({type_attr["doc"]})")\n"""
            for value_name, value_attr in type_attr["values"].items():
                value_name = snake_to_camel(value_name)
                value = f"{type_name}::{value_name}"
                register += f"""        .value("{value_name}", {value}, R"({value_attr["doc"]})")\n"""
            register += "        ;\n\n"

        if type_attr["type"] == "record":
            decl, defs = record_define_cpp(type_map, type_name, type_attr)
            type_attr["decl"] = decl
            register += record_reg(type_map, type_name, type_attr["arguments"])
            type_attr["define"] = defs

    return ENV.get_template("module.j2").render(
        register=register, types=types, type_map=type_map
    )


print(generate())
