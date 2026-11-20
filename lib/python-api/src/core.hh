#pragma once

#include "util.hh" // IWYU pragma: keep

#include <clingo/core.h>

#include <pybind11/pybind11.h>

#include <string_view>

namespace PyClingo {

//! Store the current Python error as a clingo error and return false.
//!
//! The message includes the traceback. If no Python error is set, a runtime
//! error with the fallback message is stored.
auto store_python_error(std::string_view fallback) -> bool;

//! Raise the current clingo error as a Python error.
void raise_python_error();

//! Raise the current clingo error as a Python error and return nullptr.
auto py_handle_error() -> std::nullptr_t;

//! Report an error to stderr.
//!
//! An error is reported if res is false or the code indicates an error and a
//! clingo error is set.
auto report_main_error(bool res, int code, clingo_string_t name) -> bool;

void register_core(pybind11::module &m);

// C API to access `clingo.core.Library` objects.
//
// NOTE: Library objects release their clingo library in tp_dealloc and not in
// tp_clear so that objects in reference cycles with the library (e.g., scripts
// holding symbols) are released before the clingo library is freed.

//! Get the `clingo.core.Library` type (new reference).
auto py_library_type() -> PyObject *;

//! Get the clingo library of a `clingo.core.Library` object.
//!
//! Returns nullptr with a Python error set if the object is not a library or
//! has been invalidated.
auto py_library_get(PyObject *type, PyObject *obj) -> clingo_lib_t *;

//! Get the `clingo.core.Library` object for a `clingo_lib_t` object.
//!
//! Each clingo library is represented by at most one library object, which is
//! registered as its owner. If there is one, it is returned. Otherwise, Python
//! attaches itself to the library: a non-owning library object is created and
//! registered. The clingo library keeps it alive and invalidates it when it is
//! released.
//!
//! Returns a new reference or nullptr with a Python error set.
auto py_library_attach(PyObject *type, clingo_lib_t *lib) -> PyObject *;

//! Keep the given object alive as long as the library object.
//!
//! The reference is visible to the garbage collector. Returns false with a
//! Python error set on failure.
auto py_library_keep(PyObject *type, PyObject *library, PyObject *obj) -> bool;

// C API to access `clingo.core.Location` objects.

//! Get the `clingo.core.Location` type (new reference).
auto py_location_type() -> PyObject *;

//! Create a location object holding a copy of the given location.
//!
//! Returns a new reference or nullptr with a Python error set.
auto py_location_new(PyObject *type, clingo_location_t const *loc) -> PyObject *;

//! Get the clingo location of a `clingo.core.Location` object.
//!
//! The location is owned by the object. Returns nullptr with a Python error
//! set if the object is not a location.
auto py_location_get(PyObject *type, PyObject *obj) -> clingo_location_t const *;

} // namespace PyClingo
