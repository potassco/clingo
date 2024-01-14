import yaml
from clingo.ast import _type_info_yaml


def snake_to_camel(name):
    return "".join(x.title() for x in name.split("_"))


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
        f"""\
class {type_name} {{
public:
    // Note: for pybind
    {type_name}() = default;

    {type_name}({type_name} const &x) {{
        if (!clingo_ast_copy(x.ast_, &ast_)) {{
            throw std::runtime_error("could not copy ast");
        }}
    }}

    {type_name}({type_name} &&x) noexcept {{
        std::swap(ast_, x.ast_);
    }}

    auto operator=({type_name} const &x) -> {type_name}& {{
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {{
            throw std::runtime_error("could not copy ast");
        }}
        return *this;
    }}

    auto operator=({type_name} &&x) noexcept -> {type_name}& {{
        std::swap(ast_, x.ast_);
        return *this;
    }}

    [[nodiscard]] auto hash() const -> size_t {{
        return clingo_ast_hash(ast_);
    }}

    friend auto operator==({type_name} const &a, {type_name} const &b) -> bool {{
        return clingo_ast_equal(a.ast_, b.ast_);
    }}

    friend auto operator<({type_name} const &a, {type_name} const &b) -> bool {{
        return clingo_ast_less_than(a.ast_, b.ast_);
    }}

    CLINGO_CPP_TOTAL_ORDER(friend, {type_name})

    auto to_string() -> std::string {{
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {{
            throw std::runtime_error("could convert to string");
        }}
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {{
            throw std::runtime_error("could convert to string");
        }}
        if (!str.empty() && str.back() == '\\0') {{
            str.pop_back();
        }}
        return str;
    }}

    ~{type_name}() {{
        clingo_ast_free(ast_);
    }}

{decl}\
    static auto acquire(clingo_ast_t *ast) -> {type_name} {{
        return {{ast}};
    }}

    {cons_decl};

    friend auto c_cast({type_name} const &x) -> clingo_ast_t *;

private:
    {type_name}(clingo_ast_t *ast) : ast_{{ast}} {{}}

    clingo_ast_t *ast_ = nullptr;
}};

inline auto c_cast({type_name} const &x) -> clingo_ast_t * {{
    return x.ast_;
}}

""",
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
            preamble += f"using {type_name} = std::vector<{value_type}>;\n\n"
            preamble += array_declare_cpp(type_attr) + "\n\n"
            defines += array_define_cpp(type_attr, type_dict)

        if type_attr["type"] == "union":
            types = ", ".join(snake_to_camel(x) for x in type_attr["types"])
            preamble += f"using {type_name} = std::variant<{types}>;\n\n"
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
            decl, defs = record_define_cpp(type_dict, type_name, type_attr)
            preamble += decl
            defines += defs
            register += record_reg(type_dict, type_name, type_attr["arguments"])

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

template <class... Ts>
auto c_cast(std::variant<Ts...> const &var) -> clingo_ast_t*;

template <class T>
auto c_cast(std::vector<T> const &arr) -> std::vector<clingo_ast_t*>;\
{preamble}\
{defines}\
template <class... Ts>
auto c_cast(std::variant<Ts...> const &var) -> clingo_ast_t* {{
    return std::visit([](auto const &x) {{ return c_cast(x); }}, var);
}}

template <class T>
auto c_cast(std::vector<T> const &arr) -> std::vector<clingo_ast_t*> {{
    std::vector<clingo_ast_t*> ret;
    ret.reserve(arr.size());
    for (auto const &x : arr)  {{
        ret.emplace_back(c_cast(x));
    }}
    return ret;
}}

void register_module(pybind11::module &m) {{
    auto ast = m.def_submodule("ast", doc(R"(
TODO
)"));

    ast.def("_type_info_yaml", &clingo_ast_type_info_yaml, doc(R"(
TODO
)"));

{register[:-1]}\
}}

}}\
"""

    return result


print(generate())
