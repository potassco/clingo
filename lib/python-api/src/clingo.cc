#include <pybind11/pybind11.h>

#include "core.hh"

PYBIND11_MODULE(clingo, m) {
    m.doc() = "the clingo python module";
    Clingo::Core::register_module(m);
}
