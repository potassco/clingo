#include "ast.hh"
#include "ast2.hh"
#include "core.hh"
#include "symbol.hh"

#include <pybind11/pybind11.h>

PYBIND11_MODULE(clingo, m) {
    m.doc() = "the clingo python module";
    Clingo::Core::register_module(m);
    Clingo::Symbol::register_module(m);
    Clingo::AST::register_module(m);
    Clingo::AST2::register_module(m);
}
