#include <pybind11/pybind11.h>

#include "core.hh"
#include "symbol.hh"

PYBIND11_MODULE(clingo, m) {
    m.doc() = "the clingo python module";
    Clingo::register_module(m);
    Clingo::Symbol::register_module(m);
}
