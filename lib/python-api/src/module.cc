#include "clingo.hh"

// NOLINTNEXTLINE
PyMODINIT_FUNC PyInit_clingo() {
    return PyClingo::init_clingo();
}
