#include "clingo.hh"
#include "core.hh" // IWYU pragma: keep
#include "script.hh"

#include <pybind11/embed.h>

extern "C" auto clingo_register_python(clingo_lib_t *lib) -> clingo_result_t {
    return Clingo::Python::register_python(lib);
}

PYBIND11_EMBEDDED_MODULE(clingo, m) {
    Clingo::Python::register_clingo(m);
}
