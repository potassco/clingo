#include "stats.hh"
#include "util.hh"

namespace Clingo::Python {

auto StatsArray::len() -> size_t {
    size_t size = 0;
    clingo_stats_array_size(stats_, key_, &size);
    return size;
}

auto StatsMap::len() -> size_t {
    size_t size = 0;
    clingo_stats_map_size(stats_, key_, &size);
    return size;
}

auto Stats::type() -> StatsType {
    clingo_stats_type_t type = 0;
    clingo_stats_type(stats_, key_, &type);
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
        clingo_stats_value_get(stats_, key_, &value);
        return value;
    }
    throw py::type_error{"not a value"};
}

void Stats::set_value(double value) {
    if (type() == StatsType::value) {
        clingo_stats_value_set(stats_, key_, value);
    }
    throw py::type_error{"not a value"};
}

auto Stats::as_dict() -> py::dict {
    static_cast<void>(key_);
    static_cast<void>(stats_);
    return py::dict{};
}

void register_stats(pybind11::module &m) {
    auto module = m.def_submodule("stats", R"(
Functions and classes related to solver stats.

Examples
--------
The following example shows how to add custom stats and dump the
stats in json format:

```python
>>> from json import dumps
>>> from clingo.control import Control
>>>
>>> def on_stats(step, accu):
...     accu["example"] = 42
...
>>> ctl = Control(['--stats'])
>>> ctl.add("base", [], "{a}.")
>>> ctl.ground([("base", [])])
>>> print(ctl.solve(on_stats=on_stats))
SAT
>>> print(dumps(ctl.stats['user_accu'], sort_keys=True,
...             indent=4, separators=(',', ': ')))
{
    "example": 42.0
}
>>> print(dumps(ctl.stats['summary']['times'], sort_keys=True,
...             indent=4, separators=(',', ': ')))
{
    "cpu": 0.000785999999999995,
    "sat": 7.867813110351562e-06,
    "solve": 2.288818359375e-05,
    "total": 0.0007848739624023438,
    "unsat": 0.0
}
```

Note that the control object is created passing options `--stats`. Without this
option only basic stats are reported.
)"_d);

    py::enum_<StatsType>(module, "StatsType", "The type of a stats object.")
        .value("Map", StatsType::map, R"(Indicate a map of stats.)")
        .value("Array", StatsType::array, R"(Indicate an array of stats.)")
        .value("Value", StatsType::value, R"(Indicate a value of stats.)")
        .value("Empty", StatsType::empty, R"(Indicate a stats object that is empty.)");

    auto stats = py::class_<Stats>(module, "Stats", R"(Class representing solver stats.)");

    py::class_<StatsArray>(module, "StatsArray", R"(Class representing an array of stats.)")
        .def("__len__", &StatsArray::len, "Get the length of the array.");

    py::class_<StatsMap>(module, "StatsMap", R"(Class representing a map of stats.)")
        .def("__len__", &StatsMap::len, "Get the length of the map.");

    stats.def_property_readonly("type", &Stats::type, R"(Get the type of the stats object.)");
    stats.def_property_readonly("array", &Stats::array, R"(Get an array of stats objects.)");
    stats.def_property_readonly("map", &Stats::map, R"(Get a map of stats objects.)");
    stats.def_property("value", &Stats::get_value, &Stats::set_value, R"(Get/set the value of the stats object.)");
}

} // namespace Clingo::Python
