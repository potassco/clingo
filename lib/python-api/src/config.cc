#include "config.hh"
#include "core.hh"

#include <iomanip>
#include <sstream>

namespace PyClingo {

auto Config::type_() -> clingo_config_type_bitset_t {
    clingo_config_type_bitset_t type = 0;
    handle_error(clingo_config_type(config_, key_, &type));
    return type;
}

auto Config::is_sequence() -> bool {
    return (type_() & clingo_config_type_array) != 0;
}

auto Config::is_map_() -> bool {
    return (type_() & clingo_config_type_map) != 0;
}

auto Config::is_value() -> bool {
    return (type_() & clingo_config_type_value) != 0;
}

auto Config::has_subkey_(std::string_view name) -> bool {
    if (is_map_()) {
        auto result = false;
        handle_error(clingo_config_map_has_subkey(config_, key_, name.data(), name.size(), &result));
        return result;
    }
    throw py::attribute_error{"invalid attribute"};
}

auto Config::at_sequence(size_t index) -> Config {
    if (index < len_sequence()) {
        clingo_id_t subkey = 0;
        handle_error(clingo_config_array_at(config_, key_, index, &subkey));
        return {config_, subkey};
    }
    throw py::index_error{"invalid index"};
}

auto Config::get(std::string_view name) -> Config {
    if (has_subkey_(name)) {
        clingo_id_t subkey = 0;
        handle_error(clingo_config_map_at(config_, key_, name.data(), name.size(), &subkey));
        return {config_, subkey};
    }
    throw py::attribute_error{"invalid attribute"};
}

void Config::set_value(pybind11::handle value) {
    if (is_value()) {
        auto val = py::str(value).cast<std::string>();
        handle_error(clingo_config_value_set(config_, key_, val.data(), val.size()));
    } else {
        throw py::attribute_error{"invalid attribute"};
    }
}

void Config::set(std::string_view name, pybind11::handle value) {
    auto self = py::cast(this);
    if (auto attr = py::getattr(py::type::of(self), py::str(name), py::none{}); !attr.is_none()) {
        py::getattr(attr, "__set__", py::none{})(self, value);
    } else {
        get(name).set_value(value);
    }
}

auto Config::len_sequence() -> size_t {
    if (is_sequence()) {
        size_t size = 0;
        handle_error(clingo_config_array_size(config_, key_, &size));
        return size;
    }
    throw py::attribute_error{"invalid attribute"};
}

auto Config::get_value() -> std::optional<std::string_view> {
    clingo_string_t value;
    bool has_value = false;
    handle_error(clingo_config_value_get(config_, key_, &value, &has_value));
    return has_value ? std::make_optional<std::string_view>(value.data, value.size) : std::nullopt;
}

auto Config::attrs() -> TypeHint<"Sequence[str]"> {
    py::list res;
    if (is_map_()) {
        size_t size = 0;
        handle_error(clingo_config_map_size(config_, key_, &size));
        for (size_t i = 0; i < size; ++i) {
            clingo_string_t name;
            handle_error(clingo_config_map_subkey_name(config_, key_, i, &name));
            res.append(py::cast(std::string_view{name.data, name.size}));
        }
    }
    return res;
}

auto Config::desc() -> std::string_view {
    clingo_string_t desc;
    clingo_config_description(config_, key_, &desc);
    return {desc.data, desc.size};
}

auto Config::str() -> std::string_view {
    auto *bld = string_builder();
    handle_error(clingo_config_to_string(config_, key_, bld));
    clingo_string_t res;
    handle_error(clingo_string_builder_string(bld, &res));
    return {res.data, res.size};
}

void register_config(pybind11::module &m) {
    auto config = m.def_submodule("config", R"d(
Functions and classes related to configuration.

Examples
--------
The following example shows how inpsect the configuration and modify it to
enumerate all models:

```python
>>> from clingo.core import Library
>>> from clingo.control import Control
>>>
>>> lib = Library()
>>> ctl = Control(lib)
>>> ctl.config.attributes
['tester', 'solve', 'asp', 'solver', 'configuration', 'share',
 'learn_explicit', 'sat_prepro', 'stats', 'parse_ext', 'parse_maxsat']
>>> ctl.config.solve.attributes
['solve_limit', 'parallel_mode', 'global_restarts', 'distribute',
 'integrate', 'enum_mode', 'project', 'models', 'opt_mode', 'opt_stop']
>>> str(ctl.config.solve)
"""\
solve_limit: "umax,umax"
parallel_mode: "1,compete"
global_restarts: "no"
distribute: "no,conflict,global,4,4194303"
integrate: "gp,1024,all"
enum_mode: "auto"
project: "no"
models: "-1"
opt_mode: "-1,opt"
opt_stop: "-1,opt,no"\
"""
>>> ctl.config.solve.models.description
"Compute at most %A models (0 for all)"
>>> ctl.config.solve.models = 0
>>> ctl.parse_string("1 {a; b}.")
>>> ctl.ground()
>>> with ctl.solve(on_model=print) as hnd:
...     print(hnd.get())
b
a
a b
SAT
```
)d"_d);
    py::class_<Config>(config, "Config", R"(
Allows for changing the configuration of the underlying solver.

Options are organized hierarchically. To get or change the value of an option
(identified by `is_value`) use:

```python
config.group.subgroup.option = "value"        # variant 1 (short)
config.group.subgroup.option.value = "value"  # variant 2
value = config.group.subgroup.option.value
```

There are also sequences of option groups (identified by `is_sequence`):

```python
config.group.subgroup[0].option = "value1"
config.group.subgroup[1].option = "value2"
```

Use the `attributes` member to list subgroups of an option group. Meta options
with key `configuration` set multiple related options when assigned. Use
`description` for more information about an option or option group.

Notes:
- The first element of a sequence can be accessed directly without index 0.
- Config objects have a YAML-like string representation for inspection.
- In string representations of sequences, attributes (for index 0) are only
  added if the sequence is empty.
- Option values are always strings; assigned values are automatically converted
  to strings.
)"_d)
        .def("__str__", &Config::str, R"(A readable representation to inspect the configuration.)")
        .def_property_readonly("description", &Config::desc, R"(Get the description of a configuration entry.)")
        // value interface
        .def_property_readonly("is_value", &Config::is_value, R"(Whether the configuration entry is a value.)")
        .def_property("value", &Config::get_value, &Config::set_value,
                      R"(Get/set the string value of the configuration entry.)")
        // sequence interface
        .def_property_readonly("is_sequence", &Config::is_sequence, R"(Whether the configuration is a sequence.)")
        .def("__len__", &Config::len_sequence, R"(Get the length of an array config.)")
        .def("__getitem__", &Config::at_sequence, py::arg("index"), R"(Get the index-th element of a sequence.)")
        // attribute access
        .def("__getattr__", &Config::get, py::arg("name"), R"(Get the configuration entry with the given name.)")
        .def("__setattr__", &Config::set, py::arg("name"), py::arg("value"), R"(Set the value with the given name.)")
        .def_property_readonly("attributes", &Config::attrs, R"(Get the attribute names of nested configurations.)");
}

} // namespace PyClingo
