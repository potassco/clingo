#include "clingo.hh"

PYBIND11_MODULE(clingo, m) {
    Clingo::Python::register_clingo(m);
}
