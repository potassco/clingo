#include "clingo.hh"

PYBIND11_MODULE(clingo, m) {
    PyClingo::register_clingo(m);
}
