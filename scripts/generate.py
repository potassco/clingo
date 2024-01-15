import jinja2
import yaml
from clingo.ast import _type_info_yaml


def snake_to_camel(name):
    return "".join(x.title() for x in name.split("_"))


ENV = jinja2.Environment(loader=jinja2.FileSystemLoader(searchpath="scripts/"))
ENV.filters["camel"] = snake_to_camel


def location_attribute_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> clingo_location_t;"""


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


def number_attribute_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> int;"""


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


def bool_attribute_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> bool;"""


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


def enum_attribute_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> {snake_to_camel(arg["type"])};"""


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


def string_attribute_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> char const *;"""


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


def symbol_attribute_declare_cpp(arg):
    return f"""auto {arg["name"]}() -> Symbol;"""


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


def record_define_cpp(type_dict, type_name, type_attr):
    args = type_attr["arguments"]
    decl = ""
    defs = ""
    cons_args = ["Library const &lib"]
    cons_def_args = ["lib", f'clingo_ast_type_{type_attr["name"]}', "&res_"]
    for arg in args:
        indent = "    "
        if arg["type"] == "location":
            cons_args.append("clingo_location_t const &" + arg["name"])
            cons_def_args.append(f'&{arg["name"]}')
            decl += indent + location_attribute_declare_cpp(arg) + "\n\n"
            defs += location_attribute_define_cpp(type_name, arg)
        elif arg["type"] == "string":
            cons_args.append("char const *" + arg["name"])
            cons_def_args.append(f'{arg["name"]}')
            decl += indent + string_attribute_declare_cpp(arg) + "\n\n"
            defs += string_attribute_define_cpp(type_name, arg)
        elif arg["type"] == "number":
            cons_args.append("int " + arg["name"])
            cons_def_args.append(f'{arg["name"]}')
            decl += indent + number_attribute_declare_cpp(arg) + "\n\n"
            defs += number_attribute_define_cpp(type_name, arg)
        elif arg["type"] == "bool":
            cons_args.append("bool " + arg["name"])
            cons_def_args.append(f'static_cast<int>({arg["name"]})')
            decl += indent + bool_attribute_declare_cpp(arg) + "\n\n"
            defs += bool_attribute_define_cpp(type_name, arg)
        elif arg["type"] == "symbol":
            cons_args.append("Symbol const &" + arg["name"])
            cons_def_args.append(f'{arg["name"]}.handle()')
            decl += indent + symbol_attribute_declare_cpp(arg) + "\n\n"
            defs += symbol_attribute_define_cpp(type_name, arg)
        elif type_dict[arg["type"]]["type"] == "enum":
            cons_args.append(snake_to_camel(arg["type"]) + " " + arg["name"])
            cons_def_args.append(f'static_cast<int>({arg["name"]})')
            decl += indent + enum_attribute_declare_cpp(arg) + "\n\n"
            defs += enum_attribute_define_cpp(type_name, arg)
        elif type_dict[arg["type"]]["type"] == "union":
            cons_args.append(snake_to_camel(arg["type"]) + " const &" + arg["name"])
            cons_def_args.append(f'c_cast({arg["name"]})')
            decl += indent + union_attribute_declare_cpp(arg) + "\n\n"
            defs += union_attribute_define_cpp(type_name, type_dict[arg["type"]], arg)
        elif type_dict[arg["type"]]["type"] == "array":
            cons_args.append(snake_to_camel(arg["type"]) + " const &" + arg["name"])
            cons_def_args.append(f'c_cast({arg["name"]}).data()')
            cons_def_args.append(f'{arg["name"]}.size()')
            decl += indent + attribute_array_declare_cpp(arg) + "\n\n"
            defs += attribute_array_define_cpp(type_name, arg)
        else:
            pass
            # print("handle:", arg["type"])
    cons_decl = "static auto construct(" + ", ".join(cons_args) + ") -> " + type_name
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
            type_name=type_name, cons_decl=cons_decl, decl=decl
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


def array_define_cpp(arg, type_dict):
    if type_dict[arg["value_type"]]["type"] == "union":
        cons = f'construct_{arg["value_type"]}'
    elif type_dict[arg["value_type"]]["type"] in ("forward", "record"):
        cons = f'{snake_to_camel(arg["value_type"])}::acquire'
    else:
        raise RuntimeError("unhandled type")
    return f"""\
inline auto construct_{arg["name"]}(clingo_ast_t **ast, size_t size) -> {snake_to_camel(arg["name"])} {{
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
    types = yaml.safe_load(_type_info_yaml())
    defines = ""
    register = ""

    type_dict = {}

    for type_attr in types:
        type_attr["pyname"] = snake_to_camel(type_attr["name"])
        type_dict[type_attr["name"]] = type_attr
        type_name = snake_to_camel(type_attr["name"])
        if type_attr["type"] == "array":
            defines += array_define_cpp(type_attr, type_dict)

        if type_attr["type"] == "union":
            defines += union_define_cpp(type_attr, type_dict)

        if type_attr["type"] == "enum":
            register += f"""    py::enum_<{type_name}>(ast, "{type_name}", R"({type_attr["doc"]})")\n"""
            for value_name, value_attr in type_attr["values"].items():
                value_name = snake_to_camel(value_name)
                value = f"{type_name}::{value_name}"
                register += f"""        .value("{value_name}", {value}, R"({value_attr["doc"]})")\n"""
            register += "        ;\n\n"

        if type_attr["type"] == "record":
            decl, defs = record_define_cpp(type_dict, type_name, type_attr)
            type_attr["decl"] = decl
            defines += defs
            register += record_reg(type_dict, type_name, type_attr["arguments"])

    return ENV.get_template("module.j2").render(
        register=register, defines=defines, types=types
    )


print(generate())
