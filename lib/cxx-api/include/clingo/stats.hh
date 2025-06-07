#pragma once

#include <clingo/core.hh>

#include <clingo/stats.h>

namespace Clingo {

//! @addtogroup cpp_stats
//! Inspect search and problem stats.
//! @{

//! Enumeration of statistics types.
enum class StatsType : clingo_stats_type_t {
    //! The statistics entry holds a value.
    value = clingo_stats_type_value,
    //! The statistics entry holds an array.
    array = clingo_stats_type_array,
    //! The statistics entry holds map.
    map = clingo_stats_type_map,
};

class ConstStatsArray;
class StatsArray;
class ConstStatsMap;
class StatsMap;

//! Class modeling an immutable view on a statistics entry.
class ConstStats {
  public:
    //! Construct a statistics entry from a pointer to the C API and a key.
    explicit ConstStats(clingo_stats_t const *stats, uint64_t key) : stats_{stats}, key_{key} {}

    //! Cast to the underlying C API type.
    //!
    //! @param stats the statistics entry to cast
    //! @return the underlying C API type
    friend auto c_cast(ConstStats const &stats) -> clingo_stats_t const * { return stats.stats_; }

    //! Get the type of the statistics entry.
    //!
    //! @return the type of the statistics entry
    [[nodiscard]] auto type() const -> StatsType {
        return static_cast<StatsType>(Detail::call<clingo_stats_type>(stats_, key_));
    }

    //! Get a view on the array of statistics entries if the entry is an array.
    //!
    //! @return a view on the array of statistics entries
    [[nodiscard]] auto array() const -> ConstStatsArray;

    //! Get a statistics entry at the given index if the entry is an array.
    //!
    //! @param index the index of the statistics entry to get
    //! @return a statistics entry at the given index
    [[nodiscard]] auto at(size_t index) const -> ConstStats;

    //! @copydoc Clingo::ConstStats::at()
    [[nodiscard]] auto operator[](size_t index) const -> ConstStats { return at(index); }

    //! Get a view on the map of statistics entries if the entry is a map.
    //!
    //! @return a view on the map of statistics entries
    [[nodiscard]] auto map() const -> ConstStatsMap;

    //! Get a statistics entry with the given name if the entry is a map.
    //! @param name the name of the statistics entry to get
    //! @return a statistics entry with the given name
    [[nodiscard]] auto get(std::string_view name) const -> ConstStats;

    //! @copydoc Clingo::ConstStats::get()
    [[nodiscard]] auto operator[](std::string_view name) const -> ConstStats { return get(name); }

    //! Get the value of the statistics entry if it is a value.
    //!
    //! @return the value of the statistics entry
    [[nodiscard]] auto value() const -> double {
        if (type() == StatsType::value) {
            return Detail::call<clingo_stats_value_get>(stats_, key_);
        }
        throw std::logic_error{"not a value"};
    }

    //! @copydoc Clingo::ConstStats::value()
    [[nodiscard]] auto operator*() const -> double { return value(); }

    //! Get a string representation of the statistics entry.
    //!
    //! The represention is YAML-like and can be used for debugging purposes.
    //!
    //! @return a string representation of the statistics entry
    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_stats_to_string(stats_, key_, c_cast(bld)));
        return std::string{bld.str()};
    }

  private:
    friend class Stats;

    clingo_stats_t const *stats_;
    uint64_t key_;
};

//! Class modeling a mutable view on a statistics entry.
class Stats : public ConstStats {
  public:
    //! @copydoc Clingo::ConstStats::ConstStats()
    explicit Stats(clingo_stats_t *stats, uint64_t key) : ConstStats{stats, key} {}

    //! @copydoc Clingo::ConstStats::c_cast()
    friend auto c_cast(Stats const &stats) -> clingo_stats_t * { return stats.stats_(); }

    //! @copydoc Clingo::ConstStats::array()
    [[nodiscard]] auto array() const -> StatsArray;

    //! @copydoc Clingo::ConstStats::at()
    [[nodiscard]] auto at(size_t index) const -> Stats;

    //! @copydoc Clingo::ConstStats::at()
    [[nodiscard]] auto operator[](size_t index) const -> Stats { return at(index); }

    //! @copydoc Clingo::ConstStats::map()
    [[nodiscard]] auto map() const -> StatsMap;

    //! @copydoc Clingo::ConstStats::get()
    [[nodiscard]] auto get(std::string_view name) const -> Stats;

    //! @copydoc Clingo::ConstStats::get()
    [[nodiscard]] auto operator[](std::string_view name) const -> Stats { return get(name); }

    using ConstStats::value;

    //! Set the value of the statistics entry if it is a value.
    //!
    //! @param value the value to set
    void value(double value) const {
        if (type() == StatsType::value) {
            Detail::handle_error(clingo_stats_value_set(stats_(), key_, value));
        } else {
            throw std::logic_error{"not a value"};
        }
    }

    //! @copydoc Clingo::Stats::value()
    //! @return a reference to the statistics entry
    auto operator=(double value) const -> Stats { // NOLINT
        this->value(value);
        return *this;
    }

  private:
    [[nodiscard]] auto stats_() const -> clingo_stats_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_stats_t *>(ConstStats::stats_);
    }
};

//! Class modeling an immutable view on an array of statistics entries.
class ConstStatsArray {
  public:
    //! The value type of the array, which are stats entries.
    using value_type = ConstStats;
    //! The size type of the array.
    using size_type = std::size_t;
    //! The difference type of the array.
    using difference_type = std::ptrdiff_t;
    //! The reference type of the array, which are stats entries.
    using reference = value_type;
    //! The pointer type of the array, which is a proxy to stats entries.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type, which is a random access iterator over stats entries.
    using iterator = Detail::RandomAccessIterator<ConstStatsArray>;

    //! Construct a statistics array from a pointer to the C API and a key.
    //!
    //! @param stats the statistics entry to construct the array from
    //! @param key the key of the statistics entry
    explicit ConstStatsArray(clingo_stats_t const *stats, uint64_t key) : stats_{stats}, key_{key} {}

    //! @copydoc Clingo::ConstStats::at()
    [[nodiscard]] auto at(size_t index) const -> ConstStats { return ConstStats{stats_, at_(index)}; }

    //! @copydoc Clingo::ConstStats::at()
    [[nodiscard]] auto operator[](size_t index) const -> ConstStats { return at(index); }

    //! Get the size of the array.
    //!
    //! @return the size of the array
    [[nodiscard]] auto size() const -> size_t { return Detail::call<clingo_stats_array_size>(stats_, key_); }

    //! Get an iterator to the beginning of the array.
    //!
    //! @return an iterator to the beginning of the array
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }

    //! Get an iterator to the end of the array.
    //!
    //! @return an iterator to the end of the array
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    friend class StatsArray;
    [[nodiscard]] auto at_(size_t index) const -> uint64_t {
        return Detail::call<clingo_stats_array_at>(stats_, key_, index);
    }

    clingo_stats_t const *stats_;
    uint64_t key_;
};

inline auto ConstStats::array() const -> ConstStatsArray {
    if (type() == StatsType::array) {
        return ConstStatsArray{stats_, key_};
    }
    throw std::logic_error{"not an array"};
}

inline auto ConstStats::at(size_t index) const -> ConstStats {
    return array().at(index);
}

//! Class modeling a mutable view on an array of statistics entries.
class StatsArray : public ConstStatsArray {
  public:
    //! The value type of the array, which are stats entries.
    using value_type = Stats;
    //! The size type of the array.
    using size_type = std::size_t;
    //! The difference type of the array.
    using difference_type = std::ptrdiff_t;
    //! The reference type of the array, which are stats entries.
    using reference = value_type;
    //! The pointer type of the array, which is a proxy to stats entries.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type, which is a random access iterator over stats entries.
    using iterator = Detail::RandomAccessIterator<StatsArray>;

    //! @copydoc Clingo::ConstStatsArray::ConstStatsArray()
    explicit StatsArray(clingo_stats_t *stats, uint64_t key) : ConstStatsArray{stats, key} {}

    //! @copydoc Clingo::ConstStatsArray::at()
    [[nodiscard]] auto at(size_t index) const -> Stats { return Stats{stats_(), at_(index)}; }

    //! @copydoc Clingo::ConstStatsArray::at()
    [[nodiscard]] auto operator[](size_t index) const -> Stats { return at(index); }

    //! Append a new statistics entry of the given type to the array.
    //!
    //! Appended entries have their default values; empty for arrays and lists,
    //! zero for numbers.
    //!
    //! @param type the type of the new statistics entry
    //! @return a statistics entry at the end of the array
    [[nodiscard]] auto push(StatsType type) const -> Stats {
        return Stats{stats_(),
                     Detail::call<clingo_stats_array_push>(stats_(), key_, static_cast<clingo_stats_type_t>(type))};
    }

    //! Ensure that the array has an entry at the given index.
    //!
    //! @param index the index of the statistics entry to ensure
    //! @param type the type of the statistics entry to ensure
    //! @return a statistics entry at the given index
    [[nodiscard]] auto ensure(size_t index, StatsType type) const -> Stats {
        size_t n = size();
        if (index < n) {
            return at(index);
        }
        for (; n < index; ++n) {
            std::ignore = push(type);
        }
        return push(type);
    }

    //! @copydoc Clingo::ConstStatsArray::begin()
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }

    //! @copydoc Clingo::ConstStatsArray::end()
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    [[nodiscard]] auto stats_() const -> clingo_stats_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_stats_t *>(ConstStatsArray::stats_);
    }
};

inline auto Stats::array() const -> StatsArray {
    if (type() == StatsType::array) {
        return StatsArray{stats_(), key_};
    }
    throw std::logic_error{"not an array"};
}
inline auto Stats::at(size_t index) const -> Stats {
    return array().at(index);
}

//! Class modeling an immutable view on a map of statistics entries.
class ConstStatsMap {
  public:
    //! The key type of the map, which is a string view.
    using key_type = std::string_view;
    //! The mapped type of the map, which is an immutable stats entry.
    using mapped_type = ConstStats;
    //! The value type of the map, which is a pair of key and mapped type.
    using value_type = std::pair<key_type, mapped_type>;
    //! The size type of the map.
    using size_type = std::size_t;
    //! The difference type of the map.
    using difference_type = std::ptrdiff_t;
    //! The reference type of the map, which is a pair of key and mapped type.
    using reference = value_type;
    //! The pointer type of the map, which is a proxy to the value type.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type, which is a random access iterator over value types.
    using iterator = Detail::RandomAccessIterator<ConstStatsMap>;

    //! Construct a statistics map from a pointer to the C API and a key.
    //!
    //! @param stats the statistics entry to construct the map from
    //! @param key the key of the statistics entry
    explicit ConstStatsMap(clingo_stats_t const *stats, uint64_t key) : stats_{stats}, key_{key} {}

    //! Get the size of the map.
    //!
    //! @return the size of the map
    [[nodiscard]] auto size() const -> size_t { return Detail::call<clingo_stats_map_size>(stats_, key_); }

    //! Get the name entry pair at the given index.
    //!
    //! @param index the index of the entry to get
    //! @return the name entry pair at the given index
    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto [name, subkey] = at_(index);
        return {name, ConstStats{stats_, subkey}};
    }

    //! @copydoc Clingo::ConstStats::get()
    [[nodiscard]] auto operator[](std::string_view name) const -> ConstStats { return get(name); }

    //! @copydoc Clingo::ConstStats::get()
    [[nodiscard]] auto get(std::string_view name) const -> ConstStats { return ConstStats{stats_, at_(name)}; }

    //! Check if the map contains a subkey with the given name.
    //!
    //! @param name the name of the subkey to check
    //! @return whether the map contains a subkey with the given name
    [[nodiscard]] auto contains(std::string_view name) const -> bool {
        return Detail::call<clingo_stats_map_has_subkey>(stats_, key_, name.data(), name.size());
    }

    //! Get an iterator to the beginning of the map.
    //!
    //! @return an iterator to the beginning of the map
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }

    //! Get an iterator to the end of the map.
    //!
    //! @return an iterator to the end of the map
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    friend class StatsMap;

    [[nodiscard]] auto at_(std::string_view name) const -> uint64_t {
        return Detail::call<clingo_stats_map_at>(stats_, key_, name.data(), name.size());
    }

    [[nodiscard]] auto at_(size_t index) const -> std::pair<std::string_view, uint64_t> {
        auto [data, size] = Detail::call<clingo_stats_map_subkey_name>(stats_, key_, index);
        auto str = std::string_view{data, size};
        return {str, at_(str)};
    }

    clingo_stats_t const *stats_;
    uint64_t key_;
};

inline auto ConstStats::map() const -> ConstStatsMap {
    if (type() == StatsType::map) {
        return ConstStatsMap{stats_, key_};
    }
    throw std::logic_error{"not a map"};
}

inline auto ConstStats::get(std::string_view name) const -> ConstStats {
    return map().get(name);
}

//! Class modeling a mutable view on a map of statistics entries.
class StatsMap : public ConstStatsMap {
  public:
    //! The key type of the map, which is a string view.
    using key_type = std::string_view;
    //! The mapped type of the map, which is a mutable stats entry.
    using mapped_type = Stats;
    //! The value type of the map, which is a pair of key and mapped type.
    using value_type = std::pair<key_type, mapped_type>;
    //! The size type of the map.
    using size_type = std::size_t;
    //! The difference type of the map.
    using difference_type = std::ptrdiff_t;
    //! The reference type of the map, which corresponds to the value type.
    using reference = value_type;
    //! The pointer type of the map, which is a proxy to the value type.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type, which is a random access iterator over value types.
    using iterator = Detail::RandomAccessIterator<StatsMap>;

    //! Construct a statistics map from a pointer to the C API and a key.
    //!
    //! @param stats the statistics entry to construct the map from
    //! @param key the key of the statistics entry
    explicit StatsMap(clingo_stats_t *stats, uint64_t key) : ConstStatsMap{stats, key} {}

    //! @copydoc Clingo::ConstStatsMap::at()
    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto [name, subkey] = at_(index);
        return {name, Stats{stats_(), subkey}};
    }

    //! @copydoc Clingo::ConstStatsMap::get()
    [[nodiscard]] auto operator[](std::string_view name) const -> Stats { return get(name); }

    //! @copydoc Clingo::ConstStatsMap::get()
    [[nodiscard]] auto get(std::string_view name) const -> Stats { return Stats{stats_(), at_(name)}; }

    //! Insert a statistics entry with the given name and type into the map.
    //!
    //! If the map already contains an entry with the given name, the existing
    //! entry is returned.
    //!
    //! New entries have their default values; empty for arrays and lists, zero
    //! for numbers.
    //!
    //! @param name the name of the statistics entry to insert
    //! @param type the type of the statistics entry to insert
    //! @return a statistics entry with the given name and type
    [[nodiscard]] auto insert(std::string_view name, StatsType type) const -> Stats {
        return Stats{stats_(), Detail::call<clingo_stats_map_add_subkey>(stats_(), key_, name.data(), name.size(),
                                                                         static_cast<clingo_stats_type_t>(type))};
    }

    //! Get an iterator to the beginning of the map.
    //!
    //! @return an iterator to the beginning of the map
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }

    //! Get an iterator to the end of the map.
    //!
    //! @return an iterator to the end of the map
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    [[nodiscard]] auto stats_() const -> clingo_stats_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_stats_t *>(ConstStatsMap::stats_);
    }
};

inline auto Stats::map() const -> StatsMap {
    if (type() == StatsType::map) {
        return StatsMap{stats_(), key_};
    }
    throw std::logic_error{"not a map"};
}

inline auto Stats::get(std::string_view name) const -> Stats {
    return map().get(name);
}

//! @}

} // namespace Clingo
