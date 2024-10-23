#pragma once

#include "ast.hh"
#include "symbol.hh"

#include <pybind11/pybind11.h>

namespace Clingo::Python {

class Control {
  public:
    Control(Library &lib, std::vector<std::string> const &args);
    Control(clingo_control_t *ctl) : ctl_{ctl} {}

    void parse_string(char const *str);
    void join(Program &prg);
    void ground(std::vector<std::pair<std::string, SymbolVec>> const &parts);

  private:
    static void free_(clingo_control_t *ctl) noexcept { clingo_control_free(ctl); }
    owner_ptr<clingo_control_t, free_> ctl_;
};

void register_control(pybind11::module &m);

} // namespace Clingo::Python
