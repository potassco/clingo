#include "config.hh"
#include "util.hh"

#include <iomanip>
#include <sstream>

namespace Clingo::Python {

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

auto Config::has_subkey_(char const *name) -> bool {
    if (is_map_()) {
        auto result = false;
        handle_error(clingo_config_map_has_subkey(config_, key_, name, &result));
        return result;
    }
    throw py::attribute_error{"invalid attribute"};
}

auto Config::has_value_() -> bool {
    if (is_value()) {
        bool assigned = false;
        handle_error(clingo_config_value_is_assigned(config_, key_, &assigned));
        return assigned;
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

auto Config::get(char const *name) -> Config {
    if (has_subkey_(name)) {
        clingo_id_t subkey = 0;
        handle_error(clingo_config_map_at(config_, key_, name, &subkey));
        return {config_, subkey};
    }
    throw py::attribute_error{"invalid attribute"};
}

void Config::set_value(pybind11::handle value) {
    if (is_value()) {
        auto val = py::cast<std::string>(py::str(value));
        handle_error(clingo_config_value_set(config_, key_, val.c_str()));
    } else {
        throw py::attribute_error{"invalid attribute"};
    }
}

void Config::set(char const *name, pybind11::handle value) {
    auto self = py::cast(this);
    if (auto attr = py::getattr(self.get_type(), name, py::none{}); !attr.is_none()) {
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

auto Config::get_value() -> std::optional<char const *> {
    if (has_value_()) {
        char const *value = nullptr;
        handle_error(clingo_config_value_get(config_, key_, &value));
        return value;
    }
    return std::nullopt;
}

auto Config::attrs() -> std::vector<char const *> {
    auto res = std::vector<char const *>{};
    if (is_map_()) {
        size_t size = 0;
        handle_error(clingo_config_map_size(config_, key_, &size));
        for (size_t i = 0; i < size; ++i) {
            char const *name = nullptr;
            handle_error(clingo_config_map_subkey_name(config_, key_, i, &name));
            res.emplace_back(name);
        }
    }
    return res;
}

auto Config::desc() -> char const * {
    char const *desc = nullptr;
    clingo_config_description(config_, key_, &desc);
    return desc;
}

namespace {

class fill {
  public:
    fill(size_t n, char c = ' ') : n_{n}, c_{c} {}
    friend auto operator<<(std::ostream &out, fill const &x) -> std::ostream & {
        std::fill_n(std::ostreambuf_iterator<char>(out), x.n_, x.c_);
        return out;
    }

  private:
    size_t n_;
    char c_;
};

} // namespace

void Config::str_(std::ostringstream &out, size_t first_indent, size_t indent) {
    auto fi = [&, first = true]() mutable { return fill(std::exchange(first, false) ? first_indent : indent); };
    if (is_value()) {
        if (auto val = get_value()) {
            out << fi() << std::quoted(*val) << "\n";
        } else {
            out << fi() << "null\n";
        }
    }
    if (is_map_() && (!is_sequence() || len_sequence() == 0)) {
        for (auto const *attr : attrs()) {
            out << fi() << attr << ":";
            auto cfg = get(attr);
            if (cfg.is_value()) {
                out << " ";
                cfg.str_(out, 0, indent + strlen(attr) + 2);
            } else {
                out << "\n";
                cfg.str_(out, indent + 2, indent + 2);
            }
        }
    }
    if (is_sequence()) {
        if (size_t e = len_sequence(); e > 0) {
            for (size_t i = 0, e = len_sequence(); i != e; ++i) {
                out << fi() << "- ";
                at_sequence(i).str_(out, 0, indent + 2);
            }

        } else {
            out << fi() << "[]\n";
        }
    }
}

auto Config::str() -> std::string {
    auto out = std::ostringstream{};
    str_(out, 0, 0);
    auto res = std::move(out).str();
    if (res.back() == '\n') {
        res.pop_back();
    }
    return res;
}

void register_config(pybind11::module &m) {
    auto config = m.def_submodule("config", R"(
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
opt_stop: "-1,opt,no"
"""
>>> ctl.config.solve.models.description
"""\
Compute at most %A models (0 for all)
"""
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
)"_d);
    py::class_<Config>(config, "Config", R"(
Allows for changing the configuration of the underlying solver.

Options are organized hierarchically. To get or change the value of an option
(identified by `is_value`) use:

```python
config.group.subgroup.option = "value"        # variant 1 (short)
config.group.subgroup.option.value = "value"  # variant 2
value = config.group.subgroup.option.value
```

There are also sequences of option groups (identified by the `is_sequence`
member):

```python
config.group.subgroup[0].option = "value1"
config.group.subgroup[1].option = "value2"
```

To list the subgroups of an option group, use the `attributes` member.
Furthermore, there are meta options having key `configuration`. Assigning a
meta option sets a number of related options. To get further information about
an option or option group, use `description`.

Note that the first element of a sequence can be accessed directly without
going through index 0. Furthermore, config objects have a YAML-like string
representation to inspect the current configuration. To provide full
information and avoid duplication in the string representation of sequences,
attributes are only printed if the sequence is currently emtpy.

Notes
-----
The value of an option is always a string and any value assigned to an option
is automatically converted into a string.
)"_d)
        .def("__str__", &Config::str, R"(A readable representation to inspect the configuration.)")
        .def_property_readonly("description", &Config::desc, R"(Get the description of a configuration entry.)")
        // value interface
        .def_property_readonly("is_value", &Config::is_value, R"(Whether the configuration entry is a value.)")
        .def_property("value", &Config::get_value, &Config::set_value,
                      R"(Get/set the string value of the configuration entry.)")
        // sequence interface
        .def_property_readonly("is_sequence", &Config::is_sequence, R"(Whether the configuration is a sequence.)")
        .def("__getitem__", &Config::at_sequence, py::arg("index"), R"(Get the index-th element of a sequence.)")
        // attribute access
        .def("__getattr__", &Config::get, py::arg("name"), R"(Get the configuration entry with the given name.)")
        .def("__setattr__", &Config::set, py::arg("name"), py::arg("value"), R"(Set the value with the given name.)")
        .def_property_readonly("attributes", &Config::attrs, R"(Get the attribute names of nested configurations.)");
}

} // namespace Clingo::Python
