#pragma once

#include <pybind11/pybind11.h>

#include <concepts>
#include <iterator>
#include <stdexcept>

namespace Clingo::Python {

template <typename T> class ArrowProxy {
  public:
    constexpr ArrowProxy(T value) : value_(std::move(value)) {}
    constexpr auto operator->() -> T * { return &value_; }

  private:
    T value_;
};

template <typename View>
concept IsView = requires(View c, size_t i) {
    typename View::value_type;
    { c.at(i) } -> std::same_as<typename View::value_type>;
};

template <IsView View> class RandomAccessIterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename View::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = ArrowProxy<value_type>;
    using reference = value_type;

    // NOTE: Added to fullfil the sentinel_for concept; should not be used.
    constexpr RandomAccessIterator() : view_{throw std::logic_error("invalid iterator")}, index_{0} {}
    constexpr RandomAccessIterator(View container, size_t index) noexcept
        : view_{std::move(container)}, index_{index} {}
    constexpr auto operator*() const -> reference { return view_.at(index_); }
    constexpr auto operator->() const -> pointer { return view_.at(index_); }
    constexpr auto operator++() -> RandomAccessIterator & {
        ++index_;
        return *this;
    }
    constexpr auto operator++(int) -> RandomAccessIterator {
        auto tmp = *this;
        ++index_;
        return tmp;
    }
    constexpr auto operator--() -> RandomAccessIterator & {
        --index_;
        return *this;
    }
    constexpr auto operator--(int) -> RandomAccessIterator {
        auto tmp = *this;
        --index_;
        return tmp;
    }
    constexpr auto operator-(const RandomAccessIterator &other) const -> difference_type {
        return static_cast<difference_type>(index_) - static_cast<difference_type>(other.index_);
    }
    constexpr auto operator+(difference_type n) const -> RandomAccessIterator {
        return RandomAccessIterator(view_, index_ + n);
    }
    constexpr auto operator-(difference_type n) const -> RandomAccessIterator {
        return RandomAccessIterator(view_, index_ - n);
    }
    constexpr auto operator+=(difference_type n) -> RandomAccessIterator & {
        index_ += n;
        return *this;
    }
    constexpr auto operator-=(difference_type n) -> RandomAccessIterator & {
        index_ -= n;
        return *this;
    }
    constexpr auto operator==(const RandomAccessIterator &other) const -> bool { return index_ == other.index_; }
    constexpr auto operator<=>(const RandomAccessIterator &other) const { return index_ <=> other.index_; }
    constexpr auto operator[](difference_type n) const -> reference { return view_.at(index_ + n); }

  private:
    View view_;
    size_t index_;
};

namespace MakeSequence {
struct no_contains {};
struct no_slice {};
template <typename T, typename... Args> constexpr auto contains_tag() -> bool {
    return (std::is_same_v<T, Args> || ...);
}
} // namespace MakeSequence

template <IsView T, typename... O, typename... Args>
auto make_sequence(pybind11::class_<T, O...> cls, [[maybe_unused]] Args const &...args) -> pybind11::class_<T, O...> {
    cls.def("__len__", &T::size, R"(Get the size of the sequence.)")
        .def("__getitem__", &T::at, pybind11::arg("index"), R"(Get the value at the given index.)")
        .def(
            "__iter__", [](T const &x) { return pybind11::make_iterator(x.begin(), x.end()); },
            "Get an iterator for the sequence.")
        .def(
            "__reversed__",
            [](T const &x) {
                return pybind11::make_iterator(std::make_reverse_iterator(x.end()),
                                               std::make_reverse_iterator(x.begin()));
            },
            "Get a reverse iterator for the sequence.")
        .def(
            "index",
            [](T const &x, T::value_type lit) {
                auto it = std::ranges::find(x, lit);
                return it != x.end() ? std::distance(x.begin(), it) : throw pybind11::value_error("Value not found");
            },
            pybind11::arg("value"), "Get the index of the given value in the sequence.")
        .def(
            "count", [](T const &x, T::value_type lit) { return std::count(x.begin(), x.end(), lit); },
            pybind11::arg("value"), "Count how often the given value occurs in the sequence.");
    if constexpr (!MakeSequence::contains_tag<MakeSequence::no_contains, Args...>()) {
        cls.def(
            "__contains__",
            [](T const &x, T::value_type lit) { return std::ranges::find(x.begin(), x.end(), lit) != x.end(); },
            pybind11::arg("value"), "Get a reverse iterator for the sequence.");
    }
    if constexpr (!MakeSequence::contains_tag<MakeSequence::no_slice, Args...>()) {
        cls.def("__getitem__", &T::slice, pybind11::arg("slice"), "Slice the sequence.");
    }
    return cls;
}

template <IsView T, typename... O> auto make_mapping(pybind11::class_<T, O...> cls) -> pybind11::class_<T, O...> {
    cls.def("__len__", &T::size, "Get the number elements in the map.")
        .def("__contains__", &T::contains, pybind11::arg("key"), R"(Check if the map contains the given key.)")
        .def(
            "__getitem__",
            [](T const &map, T::key_type const &key) {
                auto ret = map.get(key, std::nullopt);
                return ret ? *std::move(ret) : throw pybind11::key_error{"key not found"};
            },
            pybind11::arg("key"), R"(Get the value for the given key.)")
        .def(
            "__iter__", [](T const &map) { return pybind11::make_key_iterator(map.begin(), map.end()); },
            "Get an iterator over the keys in the map.")
        .def(
            "items", [](T const &map) { return pybind11::make_iterator(map.begin(), map.end()); },
            R"(Get an iterator over the items in the map.)")
        .def("get", &T::get, pybind11::arg("key"), pybind11::arg("default") = std::nullopt,
             R"(Get the value for the given key or the default if absent.)")
        .def(
            "values", [](T const &map) { return pybind11::make_value_iterator(map.begin(), map.end()); },
            R"(Get an iterator over the values in the map.)")
        .def(
            "keys", [](T const &map) { return pybind11::make_key_iterator(map.begin(), map.end()); },
            R"(Get get an iterator over the keys in the map.)");
    return cls;
}

} // namespace Clingo::Python
