#pragma once

#include "symbol.hh"

#include <pybind11/pybind11.h>

namespace Clingo::Python {

class Control {
  public:
    Control(Library &lib, std::vector<std::string> const &args);
    Control(clingo_control_t *ctl) : ctl_{ctl}, own_{false} {}
    Control(Control const &) = delete;
    Control(Control &&) = delete;

    void parse_string(char const *str);
    void ground(std::vector<std::pair<std::string, SymbolVec>> const &parts);
    ~Control() noexcept;

  private:
    clingo_control_t *ctl_ = nullptr;
    bool own_ = true;
};

void register_control(pybind11::module &m);

} // namespace Clingo::Python
