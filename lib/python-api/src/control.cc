#include "control.hh"
#include "util.hh"

namespace Clingo::Python {

void register_control(pybind11::module &m) {
    auto control = m.def_submodule("control", doc(R"(
Module containing the Control class responsible for grounding and solving.
)"));
}

} // namespace Clingo::Python
