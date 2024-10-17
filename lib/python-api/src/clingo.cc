#include "clingo.hh"
#include "ast.hh"
#include "control.hh"
#include "core.hh"
#include "script.hh"
#include "symbol.hh"

#include <pybind11/pybind11.h>

namespace Clingo::Python {

void register_clingo(pybind11::module &m) {
    m.doc() = "the clingo python module";
    Clingo::Python::register_core(m);
    Clingo::Python::register_symbol(m);
    Clingo::Python::register_control(m);
    Clingo::Python::register_script(m);
    Clingo::Python::register_ast(m);
}

} // namespace Clingo::Python
