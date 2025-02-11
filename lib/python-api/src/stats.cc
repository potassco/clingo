#include "stats.hh"
#include "util.hh"

namespace Clingo::Python {

namespace {

auto get_type(py::handle value) {
    if (py::isinstance<py::sequence>(value)) {
        return py::hasattr(value, "items") ? StatsType::map : StatsType::array;
    }
    if (py::isinstance<py::float_>(value)) {
        return StatsType::value;
    }
    throw py::type_error{"expected sequence, mapping, or float"};
}

} // namespace

auto StatsArray::get(size_t index) -> Stats {
    uint64_t subkey = 0;
    clingo_stats_array_at(stats_, key_, index, &subkey);
    return {stats_, subkey};
}

auto StatsArray::len() -> size_t {
    size_t size = 0;
    clingo_stats_array_size(stats_, key_, &size);
    return size;
}

void StatsArray::set(size_t index, py::handle value) {
    if (index < len()) {
        get(index).update(value);
    }
    throw py::index_error{"array index out of bounds"};
}

void StatsArray::append(py::handle value) {
    uint64_t subkey = 0;
    handle_error(clingo_stats_array_push(stats_, key_, static_cast<clingo_stats_type_t>(get_type(value)), &subkey));
    Stats{stats_, subkey}.update(value);
}

auto StatsMap::len() -> size_t {
    size_t size = 0;
    clingo_stats_map_size(stats_, key_, &size);
    return size;
}

auto StatsMap::get(char const *name) -> Stats {
    uint64_t subkey = 0;
    handle_error(clingo_stats_map_at(stats_, key_, name, &subkey));
    return {stats_, subkey};
}

void StatsMap::set(char const *name, py::handle value) {
    uint64_t subkey = 0;
    handle_error(
        clingo_stats_map_add_subkey(stats_, key_, name, static_cast<clingo_stats_type_t>(get_type(value)), &subkey));
    Stats{stats_, subkey}.update(value);
}

auto Stats::type() -> StatsType {
    clingo_stats_type_t type = 0;
    clingo_stats_type(stats_, key_, &type);
    if (type == clingo_stats_type_empty) {
        throw std::logic_error{"invalid stats object"};
    }
    return static_cast<StatsType>(type);
}

auto Stats::array() -> StatsArray {
    if (type() == StatsType::array) {
        return {stats_, key_};
    }
    throw py::type_error{"not an array"};
}

auto Stats::map() -> StatsMap {
    if (type() == StatsType::map) {
        return {stats_, key_};
    }
    throw py::type_error{"not a map"};
}

auto Stats::get_value() -> double {
    if (type() == StatsType::value) {
        double value = 0;
        handle_error(clingo_stats_value_get(stats_, key_, &value));
        return value;
    }
    throw py::type_error{"not a value"};
}

void Stats::set_value(double value) {
    if (type() == StatsType::value) {
        handle_error(clingo_stats_value_set(stats_, key_, value));
    }
    throw py::type_error{"not a value"};
}

auto Stats::to_py() -> py::object {
    switch (type()) {
        case StatsType::value: {
            return py::float_{get_value()};
        }
        case StatsType::array: {
            auto x = array();
            auto n = x.len();
            auto res = py::list{n};
            for (size_t i = 0; i < n; ++i) {
                res[i] = x.get(i).to_py();
            }
            return res;
        }
        case StatsType::map: {
            auto x = map();
            auto n = x.len();
            auto res = py::dict{};
            for (size_t i = 0; i < n; ++i) {
                char const *name = "";
                handle_error(clingo_stats_map_subkey_name(stats_, key_, i, &name));
                res[name] = x.get(name).to_py();
            }
            return res;
        }
    }
    unreachable();
}

void Stats::update(py::handle value) {
    static_cast<void>(value);
    static_cast<void>(this);
    throw std::logic_error{"implement me!!!"};
}
void register_stats(pybind11::module &m) {
    auto module = m.def_submodule("stats", R"(
Functions and classes related to solver stats.

Examples
--------
The following example shows how to add custom stats and access the stats:

```python
>>> from clingo.core import Library
>>> from clingo.control import Control
>>>
>>> def on_stats(step, accu):
...     accu["example"] = 42
...
>>> lib = Library()
>>> ctl = Control(lib, ['--stats'])
>>> ctl.parse_string("{a}.")
>>> ctl.ground()
>>> with ctl.solve(on_stats=on_stats) as hnd:
...     print(hnd.get())
SAT
>>> ctl.stats['user_accu']
{ "example": 42.0 }
>>> ctl.stats['summary']['times']
{ "cpu": 0.000785999999999995,
  "sat": 7.867813110351562e-06,
  "solve": 2.288818359375e-05,
  "total": 0.0007848739624023438,
  "unsat": 0.0 }
```

Note that the control object is created passing options `--stats`; without this
option only basic stats are reported.
)"_d);

    py::enum_<StatsType>(module, "StatsType", "The type of a stats object.")
        .value("Map", StatsType::map, R"(Indicate a map of stats.)")
        .value("Array", StatsType::array, R"(Indicate an array of stats.)")
        .value("Value", StatsType::value, R"(Indicate a value of stats.)");

    auto stats = py::class_<Stats>(module, "Stats", R"(Class representing solver stats.)");

    py::class_<StatsArray>(module, "StatsArray", R"(
Class representing an array of stats.

This class partially implements the mutable sequence protocol - elements of
arrays can be modified but they cannot be deleted.
)"_d)
        .def("__len__", &StatsArray::len, "Get the length of the array.")
        .def("__setitem__", &StatsArray::set, "Set the element at the given index to the given value.")
        .def("__getitem__", &StatsArray::get, "Get the element at the given index.")
        .def("append", &StatsArray::append, py::arg("value"), R"(
Append the given value to the array.

Args:
	value: The value to append.
)"_d);

    py::class_<StatsMap>(module, "StatsMap", R"(
Class representing a map of stats.

This class partially implements the mutable mapping protocol - value of keys
can be modified but they cannot be deleted.
)"_d)
        .def("__len__", &StatsMap::len, "Get the length of the map.")
        .def("__setitem__", &StatsMap::set, py::arg("key"), py::arg("value"), "Set the value at the given key.")
        .def("__getitem__", &StatsMap::get, py::arg("key"), "Lookup the value with the given key.");

    stats
        .def("update", &Stats::update, py::arg("values"), R"(
Update the statistics with the given values

Args:
	values: A nested structure consisting of sequencens, mappings with string keys, and floats.
)"_d)
        .def_property_readonly("type", &Stats::type, R"(Get the type of the stats object.)")
        .def_property_readonly("array", &Stats::array, R"(Get an array of stats objects.)")
        .def_property_readonly("map", &Stats::map, R"(Get a map of stats objects.)")
        .def_property("value", &Stats::get_value, &Stats::set_value, R"(Get/set the value of the stats object.)");
}

} // namespace Clingo::Python
