#include "stats.hh"
#include "core.hh"

#include <pybind11/native_enum.h>

namespace PyClingo {

namespace {
class StatsIterBase {
  public:
    StatsIterBase(clingo_stats_t *stats, uint64_t key, std::size_t len) : stats_(stats), key_(key), end_(len) {}
    auto operator==(const StatsIterBase &) const -> bool = default;
    friend auto operator==(const StatsIterBase &iter, [[maybe_unused]] std::default_sentinel_t s) -> bool {
        return iter.pos_ == iter.end_;
    }
    void next() { ++pos_; }
    [[nodiscard]] auto map_key() const -> std::string_view {
        clingo_string_t name;
        handle_error(clingo_stats_map_subkey_name(stats_, key_, pos_, &name));
        return {name.data, name.size};
    }
    [[nodiscard]] auto map_value() const -> ConstStats { return map_item().second; }
    [[nodiscard]] auto map_item() const -> std::pair<std::string_view, ConstStats> {
        auto name = this->map_key();
        auto value = ConstStatsMap{stats_, key_}.get(name);
        return {name, value};
    }
    [[nodiscard]] auto array_item() const -> ConstStats { return ConstStatsArray{stats_, key_}.get(pos_); }

  private:
    clingo_stats_t *stats_;
    uint64_t key_;
    std::size_t pos_{0};
    std::size_t end_;
};
template <auto Pmf>
    requires(std::is_invocable_v<decltype(Pmf), StatsIterBase const &>)
class StatsIter : public StatsIterBase {
  public:
    using StatsIterBase::operator==;
    using StatsIterBase::StatsIterBase;
    auto operator*() const -> std::invoke_result_t<decltype(Pmf), StatsIterBase const &> {
        return (static_cast<StatsIterBase const &>(*this).*Pmf)();
    }
    auto operator->() const -> std::invoke_result_t<decltype(Pmf), StatsIterBase const &> {
        return (static_cast<StatsIterBase const &>(*this).*Pmf)();
    }
    auto operator++() -> StatsIter & {
        this->next();
        return *this;
    }
    auto operator++(int) -> StatsIter {
        StatsIter t(*this);
        this->next();
        return t;
    }
};
template <auto Fun> auto make_py_iter(clingo_stats_t *stats, uint64_t key, std::size_t len) {
    return py::make_iterator(StatsIter<Fun>(stats, key, len), std::default_sentinel);
}
auto get_type(py::handle value, std::optional<ConstStats> old_value) -> std::pair<StatsType, py::object> {
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

auto ConstStatsArray::len() const -> size_t {
    size_t size = 0;
    handle_error(clingo_stats_array_size(stats_, key_, &size));
    return size;
}

auto ConstStatsArray::get(size_t index) const -> ConstStats {
    if (index < len()) {
        uint64_t subkey = 0;
        handle_error(clingo_stats_array_at(stats_, key_, index, &subkey));
        return {stats_, subkey};
    }
    throw py::index_error{"array index out of bounds"};
}

auto ConstStatsArray::items() const -> py::iterator {
    return make_py_iter<&StatsIterBase::array_item>(stats_, key_, len());
}

ConstStatsArray::operator StatsArray() const {
    return StatsArray{stats_, key_};
}

auto StatsArray::get(size_t index) -> Stats {
    return static_cast<Stats>(ConstStatsArray::get(index));
}

void StatsArray::set(size_t index, py::handle value) {
    get(index).update_(value, false);
}

void StatsArray::append(py::handle value) {
    uint64_t subkey = 0;
    auto [subtype, subval] = get_type(value, std::nullopt);
    handle_error(clingo_stats_array_push(stats_, key_, static_cast<clingo_stats_type_t>(subtype), &subkey));
    Stats{stats_, subkey}.update_(std::move(subval), true);
}

auto ConstStatsMap::len() const -> size_t {
    size_t size = 0;
    handle_error(clingo_stats_map_size(stats_, key_, &size));
    return size;
}

auto ConstStatsMap::contains(std::string_view name) const -> bool {
    auto res = false;
    handle_error(clingo_stats_map_has_subkey(stats_, key_, name.data(), name.size(), &res));
    return res;
}

auto ConstStatsMap::get(std::string_view name) const -> ConstStats {
    if (auto stats = try_get(name); stats) {
        return *stats;
    }
    throw py::index_error{"invalid key"};
}

auto ConstStatsMap::try_get(std::string_view name) const -> std::optional<ConstStats> {
    uint64_t subkey = 0;
    handle_error(clingo_stats_map_try_at(stats_, key_, name.data(), name.size(), key_, &subkey));
    std::optional<ConstStats> res;
    if (subkey != key_) {
        res.emplace(stats_, subkey);
    }
    return res;
}

auto ConstStatsMap::keys() const -> py::iterator {
    return make_py_iter<&StatsIterBase::map_key>(stats_, key_, len());
}

auto ConstStatsMap::values() const -> py::iterator {
    return make_py_iter<&StatsIterBase::map_value>(stats_, key_, len());
}

auto ConstStatsMap::items() const -> py::iterator {
    return make_py_iter<&StatsIterBase::map_item>(stats_, key_, len());
}

ConstStatsMap::operator StatsMap() const {
    return StatsMap{stats_, key_};
}

auto StatsMap::get(std::string_view name) -> Stats {
    return static_cast<Stats>(ConstStatsMap::get(name));
}

void StatsMap::set(std::string_view name, py::handle value) {
    auto old = try_get(name);
    auto [subtype, subval] = get_type(value, old);
    uint64_t subkey = 0;
    handle_error(clingo_stats_map_add_subkey(stats_, key_, name.data(), name.size(),
                                             static_cast<clingo_stats_type_t>(subtype), &subkey));
    Stats{stats_, subkey}.update_(std::move(subval), !old.has_value());
}

auto ConstStats::type() const -> StatsType {
    clingo_stats_type_t type = 0;
    handle_error(clingo_stats_type(stats_, key_, &type));
    return static_cast<StatsType>(type);
}

auto ConstStats::array() const -> ConstStatsArray {
    if (type() == StatsType::array) {
        return {stats_, key_};
    }
    throw py::type_error{"not an array"};
}

auto ConstStats::map() const -> ConstStatsMap {
    if (type() == StatsType::map) {
        return {stats_, key_};
    }
    throw py::type_error{"not a map"};
}

auto ConstStats::get_value() const -> double {
    if (type() == StatsType::value) {
        double value = 0;
        handle_error(clingo_stats_value_get(stats_, key_, &value));
        return value;
    }
    throw py::type_error{"not a value"};
}

auto ConstStats::str() const -> std::string_view {
    auto *bld = string_builder();
    handle_error(clingo_stats_to_string(stats_, key_, bld));
    clingo_string_t res;
    handle_error(clingo_string_builder_string(bld, &res));
    return {res.data, res.size};
}

auto ConstStats::get(std::size_t key) const -> ConstStats {
    return array().get(key);
}

auto ConstStats::at(std::string_view key) const -> ConstStats {
    return map().get(key);
}

auto ConstStats::len() const -> size_t {
    switch (type()) {
        case StatsType::array:
            return ConstStatsArray{stats_, key_}.len();
        case StatsType::map:
            return ConstStatsMap{stats_, key_}.len();
        default:
            return 0;
    }
}

auto ConstStats::contains(std::string_view key) const -> bool {
    return map().contains(key);
}

ConstStats::operator Stats() const {
    return Stats{stats_, key_};
}

auto ConstStats::nestify() const -> py::object {
    switch (type()) {
        case StatsType::value: {
            return py::float_{get_value()};
        }
        case StatsType::array: {
            auto x = ConstStatsArray{stats_, key_};
            auto n = x.len();
            auto res = py::list{n};
            for (size_t i = 0; i < n; ++i) {
                res[i] = x.get(i).nestify();
            }
            return res;
        }
        case StatsType::map: {
            auto map = ConstStatsMap{stats_, key_};
            auto res = py::dict{};
            for (auto it = StatsIter<&StatsIterBase::map_item>{stats_, key_, map.len()}; it != std::default_sentinel;
                 ++it) {
                auto [name, object] = *it;
                res[py::str{name}] = object.nestify();
            }
            return res;
        }
    }
    unreachable();
}

auto ConstStats::iter() const -> py::iterator {
    switch (type()) {
        case StatsType::map:
            return map().keys();
        case StatsType::array:
            return array().items();
        case StatsType::value:
            throw py::type_error{"'StatsType::value' is not iterable"};
    }
    unreachable();
}

void Stats::set_value(double value) {
    if (type() == StatsType::value) {
        handle_error(clingo_stats_value_set(c_ptr(), key(), value));
    } else {
        throw py::type_error{"not a value"};
    }
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

auto Stats::array() -> StatsArray {
    return static_cast<StatsArray>(ConstStats::array());
}

auto Stats::map() -> StatsMap {
    return static_cast<StatsMap>(ConstStats::map());
}

auto Stats::get(std::size_t key) -> Stats {
    return static_cast<Stats>(ConstStats::get(key));
}

auto Stats::at(std::string_view key) -> Stats {
    return static_cast<Stats>(ConstStats::at(key));
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

    py::native_enum<StatsType>{module, "StatsType", "enum.IntEnum", "The type of a stats object."}
        .value("Map", StatsType::map, R"(Indicate a map of stats.)")
        .value("Array", StatsType::array, R"(Indicate an array of stats.)")
        .value("Value", StatsType::value, R"(Indicate a value of stats.)")
        .finalize();

    auto stats_view = py::class_<ConstStats>{module, "StatsView", R"(Class representing read-only solver stats.)"};
    auto stats = py::class_<Stats, ConstStats>{module, "Stats", R"(Class representing solver stats.)"};
    auto stats_array_view = py::class_<ConstStatsArray>{module, "StatsArrayView", R"(
Class representing a read-only array of stats.

This class partially implements the mutable sequence protocol.
)"_d};
    auto stats_array = py::class_<StatsArray, ConstStatsArray>{module, "StatsArray", R"(
Class representing an array of stats.

This class partially implements the mutable sequence protocol - elements of
arrays can be modified but they cannot be deleted. Modifications are
implemented via `Stats.update`.

Most use cases should be implementable just using the update function of the
top-level statistics object.
)"_d};
    auto stats_map_view = py::class_<ConstStatsMap>{module, "StatsMapView", R"(
Class representing a read-only map of stats.

This class partially implements the mutable mapping protocol.
)"_d};
    auto stats_map = py::class_<StatsMap, ConstStatsMap>{module, "StatsMap", R"(
Class representing a map of stats.

This class partially implements the mutable mapping protocol - value of keys
can be modified but they cannot be deleted. Modifications are implemented via
`Stats.update`.

Most use cases should be implementable just using the update function of the
top-level statistics object.
)"_d};

    stats_view //
        .def("__str__", &ConstStats::str, R"(A readable representation to inspect the statistics.)")
        .def("__getitem__", &ConstStats::get, "Get the element at the given index.")
        .def("__getitem__", &ConstStats::at, py::arg("key"), "Lookup the value with the given key.")
        .def("__len__", &ConstStats::len, "Get the length of this element.")
        .def("__contains__", &ConstStats::contains,
             "Checks whether the given key is in the element, which must be a map.")
        .def("__iter__", &ConstStats::iter, "Get an iterator over the keys of the map.")
        .def("__float__", &ConstStats::get_value, "Get the value of the element.")
        .def("__eq__",
             [](const ConstStats &lhs, const py::object &rhs) {
                 if (!py::isinstance<ConstStats>(rhs)) {
                     return lhs.nestify().equal(rhs);
                 }
                 return lhs == rhs.cast<ConstStats>();
             })
        .def("nestify", &ConstStats::nestify, R"(
Convert the statistics object into a nested structure consisting of sequencens,
mappings with string keys, and floats.
)"_d)
        .def_property_readonly("type", &ConstStats::type, R"(Get the type of the stats object.)")
        .def_property_readonly("array", &ConstStats::array, R"(Get an array of stats objects.)")
        .def_property_readonly("map", &ConstStats::map, R"(Get a map of stats objects.)")
        .def_property_readonly("value", &ConstStats::get_value, R"(Get the value of the stats object.)");

    stats_array_view //
        .def("__len__", &ConstStatsArray::len, "Get the length of the array.")
        .def("__iter__", &ConstStatsArray::items, "Get an iterator over the elements of the array.")
        .def("__getitem__", &ConstStatsArray::get, "Get the element at the given index.");

    stats_array //
        .def("__setitem__", &StatsArray::set, "Set the element at the given index to the given value.")
        .def("__getitem__", &StatsArray::get, "Get the element at the given index.")
        .def("append", &StatsArray::append, py::arg("value"), R"(
Append the given value to the array.

Args:
	value: The value to append.
)"_d);

    stats_map_view //
        .def("__len__", &ConstStatsMap::len, "Get the length of the map.")
        .def("__iter__", &ConstStatsMap::keys, "Get an iterator over the keys of the map.")
        .def("keys", &ConstStatsMap::keys, "Get an iterator over the keys of the map.")
        .def("values", &ConstStatsMap::values, "Get an iterator over the values of the map.")
        .def("items", &ConstStatsMap::items, "Get an iterator over the items of the map.")
        .def("__getitem__", &ConstStatsMap::get, py::arg("key"), "Lookup the value with the given key.");

    stats_map //
        .def("__setitem__", &StatsMap::set, py::arg("key"), py::arg("value"), "Set the value at the given key.")
        .def("__getitem__", &StatsMap::get, py::arg("key"), "Lookup the value with the given key.");

    stats //
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
        .def("__getitem__", &Stats::get, "Get the element at the given index.")
        .def("__getitem__", &Stats::at, py::arg("key"), "Lookup the value with the given key.")
        .def_property_readonly("array", &Stats::array, R"(Get an array of stats objects.)")
        .def_property_readonly("map", &Stats::map, R"(Get a map of stats objects.)")
        .def_property("value", &Stats::get_value, &Stats::set_value, R"(Get/set the value of the stats object.)");
}

} // namespace PyClingo
