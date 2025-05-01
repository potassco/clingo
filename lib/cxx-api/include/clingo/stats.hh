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
        clingo_stats_type_t type = 0;
        Detail::handle_error(clingo_stats_type(stats_, key_, &type));
        return static_cast<StatsType>(type);
    }

    [[nodiscard]] auto array() const -> ConstStatsArray;
    [[nodiscard]] auto map() const -> ConstStatsMap;
    [[nodiscard]] auto get() const -> double {
        if (type() == StatsType::value) {
            double value = 0;
            Detail::handle_error(clingo_stats_value_get(stats_, key_, &value));
            return value;
        }
        throw std::bad_variant_access();
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
    [[nodiscard]] auto map() const -> StatsMap;
    void set(double value) const {
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
    [[nodiscard]] auto size() const -> size_t {
        size_t size = 0;
        Detail::handle_error(clingo_stats_array_size(stats_, key_, &size));
        return size;
    }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    friend class StatsArray;
    [[nodiscard]] auto at_(size_t index) const -> uint64_t {
        uint64_t subkey = 0;
        Detail::handle_error(clingo_stats_array_at(stats_, key_, index, &subkey));
        return subkey;
    }

    clingo_stats_t const *stats_;
    uint64_t key_;
};

[[nodiscard]] inline auto ConstStats::array() const -> ConstStatsArray {
    if (type() == StatsType::array) {
        return ConstStatsArray{stats_, key_};
    }
    throw std::bad_variant_access{};
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
        uint64_t subkey = 0;
        Detail::handle_error(clingo_stats_array_push(stats_(), key_, static_cast<clingo_stats_type_t>(type), &subkey));
        return Stats{stats_(), subkey};
    }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    [[nodiscard]] auto stats_() const -> clingo_stats_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_stats_t *>(ConstStatsArray::stats_);
    }
};

[[nodiscard]] inline auto Stats::array() const -> StatsArray {
    if (type() == StatsType::array) {
        return StatsArray{stats_(), key_};
    }
    throw std::bad_variant_access{};
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

    explicit ConstStatsMap(clingo_stats_t *stats, uint64_t key) : stats_{stats}, key_{key} {}

    [[nodiscard]] auto size() const -> size_t {
        size_t size = 0;
        Detail::handle_error(clingo_stats_map_size(stats_, key_, &size));
        return size;
    }
    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto [name, subkey] = at_(index);
        return {name, ConstStats{stats_, subkey}};
    }
    [[nodiscard]] auto operator[](std::string_view name) const -> ConstStats { return ConstStats{stats_, at_(name)}; }
    [[nodiscard]] auto contains(std::string_view name) const -> bool {
        auto res = false;
        // TODO: needs sized interface
        Detail::handle_error(clingo_stats_map_has_subkey(stats_, key_, Detail::StringBuffer{name}.c_str(), &res));
        return res;
    }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    friend class StatsMap;

    [[nodiscard]] auto at_(std::string_view name) const -> uint64_t {
        uint64_t subkey = 0;
        // TODO: needs sized interface
        Detail::handle_error(clingo_stats_map_at(stats_, key_, Detail::StringBuffer{name}.c_str(), &subkey));
        return subkey;
    }

    [[nodiscard]] auto at_(size_t index) const -> std::pair<std::string_view, uint64_t> {
        // TODO: needs sized interface
        char const *name = nullptr;
        Detail::handle_error(clingo_stats_map_subkey_name(stats_, key_, index, &name));
        return {name, at_(name)};
    }

    clingo_stats_t const *stats_;
    uint64_t key_;
};

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
    [[nodiscard]] auto operator[](std::string_view name) const -> Stats { return Stats{stats_(), at_(name)}; }
    [[nodiscard]] auto insert(std::string_view name, StatsType type) const -> Stats {
        uint64_t subkey = 0;
        Detail::handle_error(clingo_stats_map_add_subkey(stats_(), key_, Detail::StringBuffer{name}.c_str(),
                                                         static_cast<clingo_stats_type_t>(type), &subkey));
        return Stats{stats_(), subkey};
    }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    [[nodiscard]] auto stats_() const -> clingo_stats_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_stats_t *>(ConstStatsMap::stats_);
    }
};

} // namespace Clingo
