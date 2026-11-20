#pragma once

#include <Python.h>

namespace PyClingo {

//! Initialize the clingo module (multi-phase initialization).
//!
//! The function returns the module definition; the submodules are created when
//! the import system executes the module.
auto init_clingo() -> PyObject *;

} // namespace PyClingo
