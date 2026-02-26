#pragma once

#include "clingo/stats.h"

#include "iterable.hh"

#include <pybind11/pybind11.h>

#include <optional>

namespace PyClingo {

namespace py = pybind11;

enum class StatsType : uint8_t {
    value = clingo_stats_type_value,
    array = clingo_stats_type_array,
    map = clingo_stats_type_map,
};

class StatsBase {
  public:
    constexpr StatsBase(clingo_stats_t *stats, uint64_t key) : stats_(stats), key_(key) {}
    [[nodiscard]] auto c_ptr() const -> clingo_stats_t * { return stats_; }
    [[nodiscard]] auto key() const -> uint64_t { return key_; }
    [[nodiscard]] auto str() const -> std::string_view;
    template <typename T> [[nodiscard]] auto as() const -> T { return T{c_ptr(), key()}; }
    constexpr auto operator==(StatsBase const &) const -> bool = default;
    constexpr auto operator!=(StatsBase const &) const -> bool = default;

  private:
    clingo_stats_t *stats_;
    uint64_t key_;
};
class Stats;
class StatsView;

class StatsArrayView : public StatsBase {
  public:
    using StatsBase::StatsBase;
    [[nodiscard]] auto get(size_t index) const -> StatsView;
    [[nodiscard]] auto len() const -> size_t;
    [[nodiscard]] auto items() const -> TypeHint<"Iterator[StatsView]">;
};

class StatsArray : public StatsArrayView {
  public:
    using StatsArrayView::StatsArrayView;
    void set(size_t index, py::handle value);
    auto get(size_t index) -> Stats;
    auto items() -> TypeHint<"Iterator[Stats]">;
    void append(py::handle value);
};

class StatsMapView : public StatsBase {
  public:
    using StatsBase::StatsBase;
    [[nodiscard]] auto get(std::string_view name) const -> StatsView;
    [[nodiscard]] auto len() const -> size_t;
    [[nodiscard]] auto contains(std::string_view name) const -> bool;
    [[nodiscard]] auto keys() const -> TypeHint<"Iterator[str]">;
    [[nodiscard]] auto values() const -> TypeHint<"Iterator[StatsView]">;
    [[nodiscard]] auto items() const -> TypeHint<"Iterator[tuple[str,StatsView]]">;
    [[nodiscard]] auto try_get(std::string_view name) const -> std::optional<StatsView>;
    [[nodiscard]] auto get_key(size_t index) const -> std::string_view;
};

class StatsMap : public StatsMapView {
  public:
    using StatsMapView::StatsMapView;
    auto get(std::string_view name) -> Stats;
    void set(std::string_view name, py::handle value);
    auto values() -> TypeHint<"Iterator[Stats]">;
    auto items() -> TypeHint<"Iterator[tuple[str,Stats]]">;
};

class StatsView : public StatsBase {
  public:
    using StatsBase::StatsBase;

    [[nodiscard]] auto type() const -> StatsType;
    [[nodiscard]] auto array() const -> StatsArrayView;
    [[nodiscard]] auto map() const -> StatsMapView;
    [[nodiscard]] auto get_value() const -> double;
    [[nodiscard]] auto len() const -> size_t;
    [[nodiscard]] auto get(std::size_t key) const -> StatsView;
    [[nodiscard]] auto at(std::string_view key) const -> StatsView;
    [[nodiscard]] auto contains(std::string_view key) const -> bool;
    [[nodiscard]] auto iter() const -> TypeHint<"Iterator[str|StatsView]">;

    [[nodiscard]] auto nestify() const -> py::object;
};

class Stats : public StatsView {
  public:
    using StatsView::StatsView;
    auto array() -> StatsArray;
    auto map() -> StatsMap;
    auto get(std::size_t key) -> Stats;
    auto at(std::string_view key) -> Stats;
    auto iter() -> TypeHint<"Iterator[str|Stats]">;
    void set_value(double value);
    void update(py::handle value) { update_(value, false); }
    void update_(py::handle value, bool init);
};

void register_stats(pybind11::module &m);

} // namespace PyClingo
