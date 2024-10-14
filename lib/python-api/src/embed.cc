#include "clingo.hh"

#include <pybind11/embed.h>

PYBIND11_EMBEDDED_MODULE(clingo, m) { Clingo::Python::register_clingo(m); }
