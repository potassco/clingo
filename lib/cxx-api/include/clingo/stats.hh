#pragma once

#include <clingo/core.hh>

#include <clingo/stats.h>

namespace Clingo {

enum class StatsType : clingo_stats_type_t {
    value = clingo_stats_type_value,
    array = clingo_stats_type_array,
    map = clingo_stats_type_map,
};

class Stats;

class StatsArray {
  public:
    StatsArray(clingo_stats_t *stats, uint64_t key) : stats_{stats}, key_{key} {}
    [[nodiscard]] auto at(size_t index) const -> Stats;
    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto push(StatsType type) const -> Stats;

    // Needs: begin end to iterator over values

  private:
    clingo_stats_t *stats_;
    uint64_t key_;
};

class StatsMap {
  public:
    StatsMap(clingo_stats_t *stats, uint64_t key) : stats_{stats}, key_{key} {}
    [[nodiscard]] auto at(char const *name) const -> std::optional<Stats>;
    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto contains(char const *name) const -> bool;
    [[nodiscard]] auto insert(char const *name, StatsType type) const -> Stats;

    // Needs: begin end to iterator over keys

  private:
    clingo_stats_t *stats_;
    uint64_t key_;
};

class Stats {
  public:
    Stats(clingo_stats_t *stats, uint64_t key) : stats_{stats}, key_{key} {}

    [[nodiscard]] auto type() const -> StatsType;
    [[nodiscard]] auto array() const -> StatsArray;
    [[nodiscard]] auto map() const -> StatsMap;
    [[nodiscard]] auto get() const -> double;
    void set(double value) const;
    friend auto c_cast(Stats const &stats) -> clingo_stats_t * { return stats.stats_; }

  private:
    clingo_stats_t *stats_;
    uint64_t key_;
};

} // namespace Clingo
