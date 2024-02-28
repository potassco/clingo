#pragma once

#include <pybind11/operators.h>

#define CLINGO_PY_TOTAL_ORDER                                                                                          \
    .def(py::self == py::self)                                                                                         \
        .def(py::self != py::self)                                                                                     \
        .def(py::self < py::self)                                                                                      \
        .def(py::self <= py::self)                                                                                     \
        .def(py::self > py::self)                                                                                      \
        .def(py::self >= py::self)

#define CLINGO_CPP_TOTAL_ORDER(type, T)                                                                                \
    [[maybe_unused]] type auto operator!=(T const &a, T const &b) -> bool { return !(a == b); }                        \
    [[maybe_unused]] type auto operator<=(T const &a, T const &b) -> bool { return !(b < a); }                         \
    [[maybe_unused]] type auto operator>(T const &a, T const &b) -> bool { return b < a; }                             \
    [[maybe_unused]] type auto operator>=(T const &a, T const &b) -> bool { return !(a < b); }
