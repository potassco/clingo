#pragma once

#include <clingo/config.h>

#include <pybind11/pybind11.h>

namespace Clingo::Python {

class Config {
  public:
    Config(clingo_config_t *config, clingo_id_t key) : config_{config}, key_{key} {}

    // value interface
    auto is_value() -> bool;
    auto has_value() -> bool;
    auto get_value() -> char const *;
    void set_value(pybind11::handle value);

    // array interface
    auto is_array() -> bool;
    auto array_at(size_t index) -> Config;
    auto len_array() -> size_t;

    // attribute access
    auto get(char const *name) -> Config;
    void set(char const *name, pybind11::handle value);
    auto attrs() -> std::vector<char const *>;

    // inspection
    auto str() -> std::string;
    auto desc() -> char const *;

  private:
    auto type_() -> clingo_config_type_bitset_t;
    auto is_map_() -> bool;
    auto has_subkey_(char const *name) -> bool;
    void str_(std::ostringstream &out, size_t first_indent, size_t indent);

    clingo_config_t *config_;
    clingo_id_t key_;
};

void register_config(pybind11::module &m);

} // namespace Clingo::Python
