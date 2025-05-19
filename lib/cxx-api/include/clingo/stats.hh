#pragma once

#include <clingo/core.hh>

#include <clingo/stats.h>

namespace Clingo {

enum class StatsType : clingo_stats_type_t {
    value = clingo_stats_type_value,
    array = clingo_stats_type_array,
    map = clingo_stats_type_map,
};

class ConstStatsArray;
class StatsArray;
class ConstStatsMap;
class StatsMap;

class ConstStats {
  public:
    explicit ConstStats(clingo_stats_t const *stats, uint64_t key) : stats_{stats}, key_{key} {}
    friend auto c_cast(ConstStats const &stats) -> clingo_stats_t const * { return stats.stats_; }

    [[nodiscard]] auto type() const -> StatsType {
        return static_cast<StatsType>(Detail::call<clingo_stats_type>(stats_, key_));
    }

    [[nodiscard]] auto array() const -> ConstStatsArray;
    [[nodiscard]] auto at(size_t index) const -> ConstStats;
    [[nodiscard]] auto operator[](size_t index) const -> ConstStats { return at(index); }
    [[nodiscard]] auto map() const -> ConstStatsMap;
    [[nodiscard]] auto get(std::string_view name) const -> ConstStats;
    [[nodiscard]] auto operator[](std::string_view name) const -> ConstStats { return get(name); }
    [[nodiscard]] auto value() const -> double {
        if (type() == StatsType::value) {
            return Detail::call<clingo_stats_value_get>(stats_, key_);
        }
        throw std::bad_variant_access();
    }

    [[nodiscard]] auto to_string() -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_stats_to_string(stats_, key_, c_cast(bld)));
        return std::string{bld.str()};
    }

  private:
    friend class Stats;

    clingo_stats_t const *stats_;
    uint64_t key_;
};

class Stats : public ConstStats {
  public:
    explicit Stats(clingo_stats_t *stats, uint64_t key) : ConstStats{stats, key} {}
    friend auto c_cast(Stats const &stats) -> clingo_stats_t * { return stats.stats_(); }

    [[nodiscard]] auto array() const -> StatsArray;
    [[nodiscard]] auto at(size_t index) const -> Stats;
    [[nodiscard]] auto operator[](size_t index) const -> Stats { return at(index); }
    [[nodiscard]] auto map() const -> StatsMap;
    [[nodiscard]] auto get(std::string_view name) const -> Stats;
    [[nodiscard]] auto operator[](std::string_view name) const -> Stats { return get(name); }
    void value(double value) const {
        if (type() == StatsType::value) {
            Detail::handle_error(clingo_stats_value_set(stats_(), key_, value));
        } else {
            throw std::bad_variant_access{};
        }
    }

  private:
    [[nodiscard]] auto stats_() const -> clingo_stats_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_stats_t *>(ConstStats::stats_);
    }
};

class ConstStatsArray {
  public:
    using value_type = ConstStats;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<ConstStatsArray>;

    explicit ConstStatsArray(clingo_stats_t const *stats, uint64_t key) : stats_{stats}, key_{key} {}

    [[nodiscard]] auto at(size_t index) const -> ConstStats { return ConstStats{stats_, at_(index)}; }
    [[nodiscard]] auto operator[](size_t index) const -> ConstStats { return at(index); }
    [[nodiscard]] auto size() const -> size_t { return Detail::call<clingo_stats_array_size>(stats_, key_); }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
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
    throw std::bad_variant_access{};
}

inline auto ConstStats::at(size_t index) const -> ConstStats {
    return array().at(index);
}

class StatsArray : public ConstStatsArray {
  public:
    using value_type = Stats;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<StatsArray>;

    explicit StatsArray(clingo_stats_t *stats, uint64_t key) : ConstStatsArray{stats, key} {}

    [[nodiscard]] auto at(size_t index) const -> Stats { return Stats{stats_(), at_(index)}; }
    [[nodiscard]] auto operator[](size_t index) const -> Stats { return at(index); }
    [[nodiscard]] auto push(StatsType type) const -> Stats {
        return Stats{stats_(),
                     Detail::call<clingo_stats_array_push>(stats_(), key_, static_cast<clingo_stats_type_t>(type))};
    }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
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
    throw std::bad_variant_access{};
}
inline auto Stats::at(size_t index) const -> Stats {
    return array().at(index);
}

class ConstStatsMap {
  public:
    using key_type = std::string_view;
    using mapped_type = ConstStats;
    using value_type = std::pair<key_type, mapped_type>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<ConstStatsMap>;

    explicit ConstStatsMap(clingo_stats_t const *stats, uint64_t key) : stats_{stats}, key_{key} {}

    [[nodiscard]] auto size() const -> size_t { return Detail::call<clingo_stats_map_size>(stats_, key_); }
    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto [name, subkey] = at_(index);
        return {name, ConstStats{stats_, subkey}};
    }
    [[nodiscard]] auto operator[](std::string_view name) const -> ConstStats { return get(name); }
    [[nodiscard]] auto get(std::string_view name) const -> ConstStats { return ConstStats{stats_, at_(name)}; }
    [[nodiscard]] auto contains(std::string_view name) const -> bool {
        return Detail::call<clingo_stats_map_has_subkey>(stats_, key_, name.data(), name.size());
    }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
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
    throw std::bad_variant_access{};
}

inline auto ConstStats::get(std::string_view name) const -> ConstStats {
    return map().get(name);
}

class StatsMap : public ConstStatsMap {
  public:
    using key_type = std::string_view;
    using mapped_type = Stats;
    using value_type = std::pair<key_type, mapped_type>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<StatsMap>;

    explicit StatsMap(clingo_stats_t *stats, uint64_t key) : ConstStatsMap{stats, key} {}

    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto [name, subkey] = at_(index);
        return {name, Stats{stats_(), subkey}};
    }
    [[nodiscard]] auto operator[](std::string_view name) const -> Stats { return get(name); }
    [[nodiscard]] auto get(std::string_view name) const -> Stats { return Stats{stats_(), at_(name)}; }
    [[nodiscard]] auto insert(std::string_view name, StatsType type) const -> Stats {
        return Stats{stats_(), Detail::call<clingo_stats_map_add_subkey>(stats_(), key_, name.data(), name.size(),
                                                                         static_cast<clingo_stats_type_t>(type))};
    }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
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
    throw std::bad_variant_access{};
}

inline auto Stats::get(std::string_view name) const -> Stats {
    return map().get(name);
}

} // namespace Clingo
