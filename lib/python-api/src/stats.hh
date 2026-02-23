#pragma once

#include "clingo/stats.h"

#include "iterable.hh"

#include <pybind11/pybind11.h>

namespace PyClingo {

namespace py = pybind11;

enum class StatsType : uint8_t {
    value = clingo_stats_type_value,
    array = clingo_stats_type_array,
    map = clingo_stats_type_map,
};

class Stats;
class ConstStats;
class StatsArray;

class ConstStatsArray {
  public:
    ConstStatsArray(clingo_stats_t *stats, uint64_t key) : stats_{stats}, key_{key} {}
    [[nodiscard]] auto get(size_t index) const -> ConstStats;
    [[nodiscard]] auto len() const -> size_t;
    [[nodiscard]] auto items() const -> TypeHint<"Iterator[StatsView]">;
    explicit operator StatsArray() const;

  private:
    friend class StatsArray;
    clingo_stats_t *stats_;
    uint64_t key_;
};

class StatsArray : public ConstStatsArray {
  public:
    using ConstStatsArray::ConstStatsArray;
    void set(size_t index, py::handle value);
    auto get(size_t index) -> Stats;
    void append(py::handle value);
};

class StatsMap;
class ConstStatsMap {
  public:
    ConstStatsMap(clingo_stats_t *stats, uint64_t key) : stats_{stats}, key_{key} {}
    [[nodiscard]] auto get(std::string_view name) const -> ConstStats;
    [[nodiscard]] auto len() const -> size_t;
    [[nodiscard]] auto contains(std::string_view name) const -> bool;
    [[nodiscard]] auto keys() const -> TypeHint<"Iterator[str]">;
    [[nodiscard]] auto values() const -> TypeHint<"Iterator[StatsView]">;
    [[nodiscard]] auto items() const -> TypeHint<"Iterator[tuple[str,StatsView]]">;

    explicit operator StatsMap() const;

  private:
    [[nodiscard]] auto try_get(std::string_view name) const -> std::optional<ConstStats>;
    friend class StatsMap;
    clingo_stats_t *stats_;
    uint64_t key_;
};

class StatsMap : public ConstStatsMap {
  public:
    using ConstStatsMap::ConstStatsMap;
    auto get(std::string_view name) -> Stats;
    void set(std::string_view name, py::handle value);
};

class ConstStats {
  public:
    ConstStats(clingo_stats_t *stats, uint64_t key) : stats_{stats}, key_{key} {}

    [[nodiscard]] auto type() const -> StatsType;
    [[nodiscard]] auto array() const -> ConstStatsArray;
    [[nodiscard]] auto map() const -> ConstStatsMap;
    [[nodiscard]] auto get_value() const -> double;
    [[nodiscard]] auto str() const -> std::string_view;
    [[nodiscard]] auto c_ptr() const -> clingo_stats_t * { return stats_; }
    [[nodiscard]] auto key() const -> uint64_t { return key_; }
    [[nodiscard]] auto nestify() const -> py::object;
    [[nodiscard]] auto iter() const -> TypeHint<"Iterator[str|StatsView]">;

    [[nodiscard]] auto get(std::size_t key) const -> ConstStats;
    [[nodiscard]] auto at(std::string_view key) const -> ConstStats;
    [[nodiscard]] auto len() const -> size_t;
    [[nodiscard]] auto contains(std::string_view key) const -> bool;

    explicit operator Stats() const;
    [[nodiscard]] auto operator==(const ConstStats &) const -> bool = default;
    [[nodiscard]] auto operator!=(const ConstStats &) const -> bool = default;

  private:
    clingo_stats_t *stats_;
    uint64_t key_;
};
class Stats : public ConstStats {
  public:
    using ConstStats::ConstStats;
    auto array() -> StatsArray;
    auto map() -> StatsMap;
    auto get(std::size_t key) -> Stats;
    auto at(std::string_view key) -> Stats;
    void set_value(double value);
    void update(py::handle value) { update_(value, false); }
    void update_(py::handle value, bool init);
};

void register_stats(pybind11::module &m);

} // namespace PyClingo
