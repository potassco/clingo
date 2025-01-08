#pragma once

#include "ast.hh"
#include "symbol.hh"

#include <clingo/control.h>

#include <pybind11/pybind11.h>

namespace Clingo::Python {

class Control {
  public:
    Control(Library &lib, std::vector<std::string> const &args);
    Control(clingo_control_t *ctl) : ctl_{ctl} {}

    void parse_string(char const *str);
    void join(Program &prg);
    void ground(std::optional<std::vector<std::pair<std::string, SymbolVec>>> const &parts, py::handle ctx);
    void solve();
    void main();
    auto buffer() -> char const *;

  private:
    static auto ctx_(clingo_lib_t *lib, clingo_location_t const *location, char const *name,
                     clingo_symbol_t const *arguments, size_t arguments_size, void *data,
                     clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) -> clingo_result_t;
    static void free_(clingo_control_t *ctl) noexcept { clingo_control_free(ctl); }
    owner_ptr<clingo_control_t, free_> ctl_;
};

void register_control(pybind11::module &m);

} // namespace Clingo::Python
