#pragma once

#include <pybind11/pybind11.h>

#include <concepts>
#include <iterator>
#include <stdexcept>

namespace PyClingo {

namespace Detail {

template <typename T>
concept HasContains = requires(T const &t, T::value_type const &val) {
    { t.contains(val) };
};

template <typename T>
concept HasSlice = requires(T const &t, pybind11::slice const &slc) {
    { t.slice(slc) };
};

template <typename Seq>
concept IsSequence = requires(Seq c, size_t i) {
    typename Seq::value_type;
    { c.at(i) } -> std::same_as<typename Seq::value_type>;
    { c.size() } -> std::integral;
};

template <typename Map>
concept IsMapping =
    IsSequence<Map> && requires(Map c, Map::key_type key, std::optional<typename Map::mapped_type> val) {
        typename Map::key_type;
        typename Map::mapped_type;
        { c.contains(key) } -> std::same_as<bool>;
        { c.get(key, val) } -> std::same_as<std::optional<typename Map::mapped_type>>;
    };

template <typename T> class ArrowProxy {
  public:
    constexpr ArrowProxy(T value) : value_(std::move(value)) {}
    constexpr auto operator->() -> T * { return &value_; }

  private:
    T value_;
};

} // namespace Detail

template <Detail::IsSequence Seq> class RandomAccessIterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename Seq::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = Detail::ArrowProxy<value_type>;
    using reference = value_type;

    // NOTE: Added to fullfil the sentinel_for concept; should not be used.
    constexpr RandomAccessIterator() : view_{throw std::logic_error("invalid iterator")}, index_{0} {}
    constexpr RandomAccessIterator(Seq container, size_t index) noexcept : view_{std::move(container)}, index_{index} {}
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
    friend constexpr auto operator+(difference_type n, RandomAccessIterator it) -> RandomAccessIterator {
        return RandomAccessIterator(it.view_, it.index_ + n);
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
    Seq view_;
    size_t index_;
};

namespace Detail {

template <IsSequence T> auto begin(T x) {
    return RandomAccessIterator{std::move(x), 0};
}

template <IsSequence T> auto end(T x) {
    auto n = x.size();
    return RandomAccessIterator{std::move(x), n};
}

} // namespace Detail

template <Detail::IsSequence T, typename... O>
auto make_sequence(pybind11::class_<T, O...> cls) -> pybind11::class_<T, O...> {
    cls.def("__len__", &T::size, R"(Get the size of the sequence.)")
        .def("__getitem__", &T::at, pybind11::arg("index"), R"(Get the value at the given index.)")
        .def(
            "__iter__", [](T const &seq) { return pybind11::make_iterator(Detail::begin(seq), Detail::end(seq)); },
            "Get an iterator for the sequence.")
        .def(
            "__reversed__",
            [](T const &seq) {
                return pybind11::make_iterator(std::make_reverse_iterator(Detail::end(seq)),
                                               std::make_reverse_iterator(Detail::begin(seq)));
            },
            "Get a reverse iterator for the sequence.")
        .def(
            "index",
            [](T const &seq, T::value_type val) {
                auto it = std::ranges::find(Detail::begin(seq), Detail::end(seq), val);
                return it != Detail::end(seq) ? std::distance(Detail::begin(seq), it)
                                              : throw pybind11::value_error("Value not found");
            },
            pybind11::arg("value"), "Get the index of the given value in the sequence.")
        .def(
            "count",
            [](T const &seq, T::value_type val) { return std::count(Detail::begin(seq), Detail::end(seq), val); },
            pybind11::arg("value"), "Count how often the given value occurs in the sequence.");
    if constexpr (Detail::HasContains<T>) {
        cls.def("__contains__", &T::contains, pybind11::arg("value"), "Get a reverse iterator for the sequence.");
    } else {
        cls.def(
            "__contains__",
            [](T const &seq, T::value_type val) {
                return std::ranges::find(Detail::begin(seq), Detail::end(seq), val) != Detail::end(seq);
            },
            pybind11::arg("value"), "Get a reverse iterator for the sequence.");
    }
    if constexpr (Detail::HasSlice<T>) {
        cls.def("__getitem__", &T::slice, pybind11::arg("slice"), "Slice the sequence.");
    }
    return cls;
}

template <Detail::IsMapping T, typename... O>
auto make_mapping(pybind11::class_<T, O...> cls) -> pybind11::class_<T, O...> {
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
            "__iter__", [](T const &map) { return pybind11::make_key_iterator(Detail::begin(map), Detail::end(map)); },
            "Get an iterator over the keys in the map.")
        .def(
            "items", [](T const &map) { return pybind11::make_iterator(Detail::begin(map), Detail::end(map)); },
            R"(Get an iterator over the items in the map.)")
        .def("get", &T::get, pybind11::arg("key"), pybind11::arg("default") = std::nullopt,
             R"(Get the value for the given key or the default if absent.)")
        .def(
            "values", [](T const &map) { return pybind11::make_value_iterator(Detail::begin(map), Detail::end(map)); },
            R"(Get an iterator over the values in the map.)")
        .def(
            "keys", [](T const &map) { return pybind11::make_key_iterator(Detail::begin(map), Detail::end(map)); },
            R"(Get an iterator over the keys in the map.)");
    return cls;
}

} // namespace PyClingo
