#pragma once

#include <Python.h>

#include "symbol.hh"
#include <clingo/base.h>

namespace PyClingo {

//! Create the clingo.base module with its own types and functions (first registration phase).
auto create_base(PyObject *parent) -> PyObject *;
//! Store the types of other modules in the state of the clingo.base module (second registration phase).
auto link_base(PyObject *mod) -> bool;

auto py_base_type() -> PyObject *;
//! Create a `clingo.base.Base` object (new reference).
//!
//! The owner is kept alive by the base object. If valid is not nullptr, it is
//! the address of a pointer of the owner that is null once the owner is no
//! longer valid; accessing the base then raises an error. Otherwise, the base
//! is valid until py_base_invalidate() is called.
auto py_base_new(PyObject *owner, void const *valid, PyObject *type, clingo_base_t const *base) -> PyObject *;
//! Invalidate a `clingo.base.Base` object; accessing it or objects obtained from it then raises an error.
void py_base_invalidate(PyObject *obj) noexcept;

//! Creates a `clingo.base.Base` object for a scope and invalidates it at the end.
class BaseGuard {
  public:
    BaseGuard(PyObject *owner, PyObject *type, clingo_base_t const *base)
        : obj_{py_base_new(owner, nullptr, type, base)} {}
    BaseGuard(BaseGuard const &) = delete;
    BaseGuard(BaseGuard &&) = delete;
    auto operator=(BaseGuard const &) -> BaseGuard & = delete;
    auto operator=(BaseGuard &&) -> BaseGuard & = delete;
    ~BaseGuard() {
        if (obj_ != nullptr) {
            py_base_invalidate(obj_);
            Py_DECREF(obj_);
        }
    }

    //! The base object (nullptr with a Python error set if creating it failed).
    [[nodiscard]] auto get() const -> PyObject * { return obj_; }

  private:
    PyObject *obj_;
};
auto py_base_get(PyObject *type, PyObject *obj) -> clingo_base_t const *;

auto convert(PyObject *base_type, PyObject *base, PyObject *lits, bool flip, bool disjunctive,
             PyBuffer<clingo_literal_t> &out) -> bool;

} // namespace PyClingo
