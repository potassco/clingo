#pragma once

#include "ast.hh"
#include "base.hh"
#include "config.hh"
#include "solve.hh"
#include "symbol.hh"

#include <clingo/control.h>

namespace Clingo::Python {

class Control {
  public:
    using AssumptionVec = std::vector<std::variant<std::pair<Symbol, bool>, Lit_t>>;

    Control(Library &lib, std::vector<std::string> const &args);
    Control(clingo_control_t *ctl) : ctl_{ctl} {}

    void parse_string(char const *str);
    void join(Program &prg);
    void ground(std::optional<std::vector<std::pair<std::string, SymbolVec>>> const &parts, py::handle ctx);
    auto solve(AssumptionVec const &assumptions, std::optional<ModelCallback> on_model, bool yield, bool async)
        -> SSolveHandle;
    auto base() -> Base;
    auto config() -> Config;
    auto stats() -> py::dict;
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
