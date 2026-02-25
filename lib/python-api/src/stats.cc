#include "stats.hh"
#include "core.hh"

#include <pybind11/native_enum.h>

namespace PyClingo {

namespace {

auto get_type(py::handle value, std::optional<Stats> old_value) -> std::pair<StatsType, py::object> {
    py::object new_value = py::none{};
    if (PyCallable_Check(value.ptr()) == 1) {
        new_value = value(old_value ? old_value->nestify() : py::none{});
    } else {
        new_value = py::reinterpret_borrow<py::object>(value);
    }
    if (py::isinstance<py::dict>(new_value)) {
        return {StatsType::map, std::move(new_value)};
    }
    if (py::isinstance<py::sequence>(new_value)) {
        // NOTE: we check for an items method to also support user defined maps
        auto t = py::hasattr(new_value, "items") ? StatsType::map : StatsType::array;
        return {t, std::move(new_value)};
    }
    if (py::isinstance<py::float_>(new_value) || py::isinstance<py::int_>(new_value) ||
        py::hasattr(new_value, "__float__")) {
        return {StatsType::value, std::move(new_value)};
    }
    throw py::type_error{"expected sequence, mapping, or float"};
}

} // namespace

auto StatsArray::get(size_t index) -> Stats {
    uint64_t subkey = 0;
    handle_error(clingo_stats_array_at(stats_, key_, index, &subkey));
    return {stats_, subkey};
}

auto StatsArray::len() -> size_t {
    size_t size = 0;
    handle_error(clingo_stats_array_size(stats_, key_, &size));
    return size;
}

void StatsArray::set(size_t index, py::handle value) {
    if (index < len()) {
        get(index).update_(value, false);
    } else {
        throw py::index_error{"array index out of bounds"};
    }
}

void StatsArray::append(py::handle value) {
    uint64_t subkey = 0;
    auto [subtype, subval] = get_type(value, std::nullopt);
    handle_error(clingo_stats_array_push(stats_, key_, static_cast<clingo_stats_type_t>(subtype), &subkey));
    Stats{stats_, subkey}.update_(std::move(subval), true);
}

auto StatsMap::len() -> size_t {
    size_t size = 0;
    handle_error(clingo_stats_map_size(stats_, key_, &size));
    return size;
}

auto StatsMap::get(std::string_view name) -> Stats {
    if (auto stats = try_get(name); stats.has_value()) {
        return *stats;
    }
    throw py::index_error{"invalid key"};
}

auto StatsMap::contains(std::string_view name) -> bool {
    auto res = false;
    handle_error(clingo_stats_map_has_subkey(stats_, key_, name.data(), name.size(), nullptr, &res));
    return res;
}

auto StatsMap::try_get(std::string_view name) -> std::optional<Stats> {
    uint64_t subkey = 0;
    auto found = false;
    handle_error(clingo_stats_map_has_subkey(stats_, key_, name.data(), name.size(), &subkey, &found));
    return found ? std::make_optional<Stats>(stats_, subkey) : std::nullopt;
}

void StatsMap::set(std::string_view name, py::handle value) {
    auto old = try_get(name);
    auto [subtype, subval] = get_type(value, old);
    uint64_t subkey = 0;
    handle_error(clingo_stats_map_add_subkey(stats_, key_, name.data(), name.size(),
                                             static_cast<clingo_stats_type_t>(subtype), &subkey));
    Stats{stats_, subkey}.update_(std::move(subval), !old.has_value());
}

auto Stats::type() -> StatsType {
    clingo_stats_type_t type = 0;
    handle_error(clingo_stats_type(stats_, key_, &type));
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
    } else {
        throw py::type_error{"not a value"};
    }
}

auto Stats::str() -> std::string_view {
    auto *bld = string_builder();
    handle_error(clingo_stats_to_string(stats_, key_, bld));
    clingo_string_t res;
    handle_error(clingo_string_builder_string(bld, &res));
    return {res.data, res.size};
}

auto Stats::nestify() -> py::object {
    switch (type()) {
        case StatsType::value: {
            return py::float_{get_value()};
        }
        case StatsType::array: {
            auto x = array();
            auto n = x.len();
            auto res = py::list{n};
            for (size_t i = 0; i < n; ++i) {
                res[i] = x.get(i).nestify();
            }
            return res;
        }
        case StatsType::map: {
            auto x = map();
            auto n = x.len();
            auto res = py::dict{};
            for (size_t i = 0; i < n; ++i) {
                clingo_string_t name;
                handle_error(clingo_stats_map_subkey_name(stats_, key_, i, &name));
                res[py::str{name.data, name.size}] = x.get(std::string_view{name.data, name.size}).nestify();
            }
            return res;
        }
    }
    unreachable();
}

void Stats::update_(py::handle value, bool init) {
    switch (type()) {
        case StatsType::value: {
            if (PyCallable_Check(value.ptr()) == 1) {
                set_value(cast<double>(value(init ? static_cast<py::object>(py::none{}) : py::float_{get_value()})));
            } else {
                set_value(cast<double>(value));
            }
            break;
        }
        case StatsType::array: {
            auto x = array();
            size_t i = 0;
            size_t j = x.len();
            for (auto elem : value) {
                if (i < j) {
                    x.set(i, elem);
                    ++i;
                } else {
                    x.append(elem);
                }
            }
            break;
        }
        case StatsType::map: {
            auto x = map();
            for (auto elem : py::getattr(value, "items")()) {
                auto key = elem[py::int_{0}];
                auto val = elem[py::int_{1}];
                x.set(py::cast<std::string>(key), val);
            }
            break;
        }
    }
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
...     accu.update({"example": [21]})
...     accu.update({"example": [lambda x: (x or 0) + 21]})
...
>>> lib = Library()
>>> ctl = Control(lib, ['--stats'])
>>> ctl.parse_string("{a}.")
>>> ctl.ground()
>>> print(ctl.solve(on_stats=on_stats))
SAT
>>> print(ctl.solve(on_stats=on_stats))
SAT
>>> ctl.stats['user_step']
{ "example": [21.0] }
>>> ctl.stats['user_accu']
{ "example": [42.0] }
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

    py::native_enum<StatsType>(module, "StatsType", "enum.IntEnum", "The type of a stats object.")
        .value("Map", StatsType::map, R"(Indicate a map of stats.)")
        .value("Array", StatsType::array, R"(Indicate an array of stats.)")
        .value("Value", StatsType::value, R"(Indicate a value of stats.)")
        .finalize();

    auto stats = py::class_<Stats>(module, "Stats", R"(Class representing solver stats.)");

    py::class_<StatsArray>(module, "StatsArray", R"(
Class representing an array of stats.

This class partially implements the mutable sequence protocol - elements of
arrays can be modified but they cannot be deleted. Modifications are
implemented via `Stats.update`.

Most use cases should be implementable just using the update function of the
top-level statistics object.
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
can be modified but they cannot be deleted. Modifications are implemented via
`Stats.update`.

Most use cases should be implementable just using the update function of the
top-level statistics object.
)"_d)
        .def("__len__", &StatsMap::len, "Get the length of the map.")
        .def("__setitem__", &StatsMap::set, py::arg("key"), py::arg("value"), "Set the value at the given key.")
        .def("__getitem__", &StatsMap::get, py::arg("key"), "Lookup the value with the given key.");

    stats
        .def("nestify", &Stats::nestify, R"(
Convert the statistics object into a nested structure consisting of sequencens,
mappings with string keys, and floats.
)"_d)
        .def("update", &Stats::update, py::arg("values"), R"(
Update the statistics with the given values.

Note that values can be inserted and changed but they cannot be deleted nor can
their type be changed.

Args:
    values: A nested structure consisting of sequencens, mappings with string
        keys, floats, and functions. The latter can be used to update
        existing values. They receive the previous values as argument and must
        return an updated value. If there is no previous value, `None` is
        passed as argument.
)"_d)
        .def("__str__", &Stats::str, R"(A readable representation to inspect the statistics.)")
        .def_property_readonly("type", &Stats::type, R"(Get the type of the stats object.)")
        .def_property_readonly("array", &Stats::array, R"(Get an array of stats objects.)")
        .def_property_readonly("map", &Stats::map, R"(Get a map of stats objects.)")
        .def_property("value", &Stats::get_value, &Stats::set_value, R"(Get/set the value of the stats object.)");
}

} // namespace PyClingo
