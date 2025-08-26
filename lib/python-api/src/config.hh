#pragma once

#include <clingo/config.h>

#include <pybind11/pybind11.h>

#include "iterable.hh"

namespace PyClingo {

class Control;

class Config {
  public:
    using Setter = TypeHint<"Callable[[str], None] | Callable[[str, Optional[int]], None] | None">;
    using Getter = TypeHint<"Callable[[], Optional[str]] | Callable[Optional[int], Optional[str]] | None>">;
    using Size = TypeHint<"Optional[Callable[[], int]]">;

    Config(Control &ctl, clingo_config_t *config, clingo_id_t key) : ctl_{&ctl}, config_{config}, key_{key} {}

    // value interface
    auto is_value() -> bool;
    auto get_value() -> std::optional<std::string_view>;
    void set_value(pybind11::handle value);

    // sequence interface
    auto is_sequence() -> bool;
    auto at_sequence(size_t index) -> Config;
    auto len_sequence() -> size_t;

    // attribute access
    auto get(std::string_view name) -> Config;
    void set(std::string_view name, pybind11::handle value);
    auto attrs() -> TypeHint<"Sequence[str]">;

    // inspection
    auto str() -> std::string_view;
    auto desc() -> std::string_view;

    // extension
    void add(std::string_view name, std::string_view description, Getter const &get, Setter const &set,
             Size const &size);

  private:
    auto type_() -> clingo_config_type_bitset_t;
    auto is_map_() -> bool;
    auto has_subkey_(std::string_view name) -> bool;
    void str_(std::ostringstream &out, size_t first_indent, size_t indent);

    Control *ctl_;
    clingo_config_t *config_;
    clingo_id_t key_;
};

void register_config(pybind11::module &m);

} // namespace PyClingo
