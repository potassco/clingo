#include "clingo.hh"
#include "core.hh" // IWYU pragma: keep
#include "script.hh"

#include <pybind11/embed.h>

extern "C" auto clingo_register_python(clingo_lib_t *lib) -> bool {
    return PyClingo::register_python(lib);
}

PYBIND11_EMBEDDED_MODULE(clingo, m) {
    PyClingo::register_clingo(m);
}
