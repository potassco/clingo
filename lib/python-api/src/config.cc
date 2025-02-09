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

auto Config::is_array() -> bool {
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

auto Config::has_value() -> bool {
    if (is_value()) {
        bool assigned = false;
        handle_error(clingo_config_value_is_assigned(config_, key_, &assigned));
        return assigned;
    }
    throw py::attribute_error{"invalid attribute"};
}

auto Config::array_at(size_t index) -> Config {
    if (index < len_array()) {
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
        auto val = value.cast<std::string>();
        handle_error(clingo_config_value_set(config_, key_, val.c_str()));
    }
    throw py::attribute_error{"invalid attribute"};
}

void Config::set(char const *name, pybind11::handle value) {
    get(name).set_value(value);
}

auto Config::len_array() -> size_t {
    if (is_array()) {
        size_t size = 0;
        handle_error(clingo_config_array_size(config_, key_, &size));
        return size;
    }
    throw py::attribute_error{"invalid attribute"};
}

auto Config::get_value() -> char const * {
    if (is_value()) {
        char const *value = nullptr;
        handle_error(clingo_config_value_get(config_, key_, &value));
        return value;
    }
    // NOTE: could be written much nicer using std::format
    auto buf = std::array<char, 3 + (2 * sizeof(void *))>{};
    snprintf(buf.data(), buf.size(), "%p", static_cast<void *>(config_));
    static thread_local auto rep = std::string{};
    rep = "<object Config at ";
    rep += buf.data();
    rep += " with key ";
    rep += std::to_string(key_);
    rep += ">";
    return rep.c_str();
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
        if (has_value()) {
            out << fi() << std::quoted(get_value()) << "\n";
        } else {
            out << fi() << "null\n";
        }
    }
    if (is_map_() && (!is_array() || len_array() == 0)) {
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
    if (is_array()) {
        if (size_t e = len_array(); e > 0) {
            for (size_t i = 0, e = len_array(); i != e; ++i) {
                out << fi() << "- ";
                array_at(i).str_(out, 0, indent + 2);
            }

        } else {
            out << fi() << "[]\n";
        }
    }
}

auto Config::str() -> std::string {
    auto out = std::ostringstream{};
    str_(out, 0, 0);
    return std::move(out).str();
}

void register_config(pybind11::module &m) {
    auto config = m.def_submodule("config", R"(
Functions and classes related to configuration.

Examples
--------
The following example shows how to modify the configuration to enumerate all
models:

```python
>>> from clingo.control import Control
>>>
>>> ctl = Control()
>>> ctl.config.attributes
['tester', 'solve', 'asp', 'solver', 'configuration', 'share',
 'learn_explicit', 'sat_prepro', 'stats', 'parse_ext', 'parse_maxsat']
>>> ctl.config.solve.attributes
['solve_limit', 'parallel_mode', 'global_restarts', 'distribute',
 'integrate', 'enum_mode', 'project', 'models', 'opt_mode']
>>> str(ctl.config.solve)
'TODO'
>>> ctl.config.solve.models.description
'Compute at most %A models (0 for all)\\n'
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
    py::class_<Config>(config, "SolveResult", R"(
Allows for changing the configuration of the underlying solver.

Options are organized hierarchically. To change and inspect an option use:

```python
config.group.subgroup.option = "value"
value = config.group.subgroup.option.value
```

There are also arrays of option groups that can be accessed using integer
indices:

```python
config.group.subgroup[0].option = "value1"
config.group.subgroup[1].option = "value2"
```

To list the subgroups of an option group, use the `Configuration.attributes`
member. Array option groups, like solver, can be iterated. Furthermore, there
are meta options having key `configuration`. Assigning a meta option sets a
number of related options. To get further information about an option or
option group, use `Configuration.description`.

Note that the first element of array at index zero can be accessed directly.
Furthermore, config objects have a YAML-like string representation to inspect
the current configuration. To provide full information and avoid duplication in
the string representation of arrays, attributes are only printed if the array
is currently emtpy.

Notes
-----
The value of an option is always a string and any value assigned to an option
is automatically converted into a string.
)"_d)
        .def("__str__", &Config::str, R"(A readable representation to inspect the configuration)");
    /*
    py::class_<SolveResult>(solving, "SolveResult", R"(A solve result captures information about a solve call.)")
        .def("__str__", &SolveResult::str, R"(Get a string representation of the solve result.)")
        .def_property_readonly("satisfiable", &SolveResult::satisfiable, R"(Whether there was at least one model.)")
        .def_property_readonly("unsatisfiable", &SolveResult::unsatisfiable, R"(Whether there was no model.)")
        .def_property_readonly("unknown", &SolveResult::unknown, R"(Whether the satisfiablity could be determined.)")
        .def_property_readonly("exhausted", &SolveResult::exhausted, R"(Whether all models have been enumerated.)")
        .def_property_readonly("interrupted", &SolveResult::interrupted, R"(Whether the search was interrupted.)");

    py::class_<Model>(solving, "Model", R"(A view on the solver's current solution.)")
        .def("symbols", &Model::symbols, py::arg("shown") = false, py::arg("atoms") = false, py::arg("terms") = false,
             py::arg("theory") = false, R"(
Get the symbols in the model.

Args:
  shown: Include shown atoms and terms.
  atoms: Include all true atoms including hidden ones.
  terms: Include shown terms.
  theory: Include terms added by external theories.
)"_d)
        .def("__str__", &Model::str, "Get a string representation of the model.");

    py::class_<SolveHandle, SSolveHandle>(solving, "SolveHandle", R"(
An object to interact with a running search.

It can be used to control solving, like, retrieving models or cancelling a
search.

A SolveHandle is a context manager and must be used with Python's with
statement.

Blocking functions in this object release the GIL. They are not thread-safe
though.

See also: `clingo.control.Control.solve`
)"_d)
        .def("get", &SolveHandle::get, R"(
Get the solve result.

This is always the last function that should be called on a handle to ensure
that the search is properly terminated. It might be preceded by a call to
cancel to stop a running search.
)"_d)
        .def("core", &SolveHandle::core, R"(Get the subset of assumptions that made the problem unsatisfiable.)")
        .def("model", &SolveHandle::model, R"(Get the current model if there is any.)")
        .def("last", &SolveHandle::last, R"(
Get the last computed model if there is any.

If the search is not completed yet or the problem is unsatisfiable, the
function returns `None`.
)"_d)
        .def("resume", &SolveHandle::resume, R"(
Discards the last model and starts searching for the next one.

If the search has been started asynchronously, this function starts the search
in the background.
)"_d)
        .def("wait", &SolveHandle::wait, py::arg("timeout") = std::nullopt, R"(
Wait for solve call to finish or the next result with an optional timeout.

If a timeout is given, the behavior of the function changes depending on the
sign of the timeout. If a postive timeout is given, the function blocks for the
given amount time or until a result is ready. If the timeout is negative, the
function will block until a result is ready, which also corresponds to the
behavior of the function if no timeout is given. A timeout of zero can be used
to poll if a result is ready.

Args:
  timeout:
    If a timeout is given, the function blocks for at most timeout seconds.

Returns:
  Indicates whether the solve call has finished or the next result is ready.
)"_d)
        .def("cancel", &SolveHandle::cancel, R"(
Cancel the running search.

See also: `clingo.control.Control.interrupt`
)"_d)
        .def(
            "__enter__", [&](SSolveHandle hnd) -> SSolveHandle { return hnd; }, "Start the search.")
        .def(
            "__exit__",
            [&](SSolveHandle const &hnd, [[maybe_unused]] const std::optional<pybind11::type> &type,
                [[maybe_unused]] const std::optional<pybind11::object> &value,
                [[maybe_unused]] const std::optional<pybind11::object> &traceback) { hnd->close(); },
            "Stop the search closing the handle.")
        .def(
            "__iter__",
            [&](SSolveHandle const &hnd) { return pybind11::make_iterator(ModelIterator{hnd}, ModelIterator{}); },
            "Get an iterator over the models.");
    */
}
} // namespace Clingo::Python
