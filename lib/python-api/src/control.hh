#pragma once

#include <clingo/control.h>

#include <pybind11/pybind11.h>

#include <cstdint>

namespace PyClingo {

//! Get the `clingo.control.Control` type (new reference).
auto py_control_type() -> PyObject *;

//! Get the clingo control object of a `clingo.control.Control` object.
//!
//! Returns nullptr with a Python error set if the object is not a control
//! object of the given type or if it is no longer valid.
auto py_control_get(PyObject *type, PyObject *obj) -> clingo_control_t *;

//! Get the clingo control object of a `clingo.control.Control` object without its type.
//!
//! The type is identified via the module it belongs to. This function is meant
//! for modules registered before the control module, which cannot import its
//! types. Returns nullptr with a Python error set if the object is not a
//! control object or if it is no longer valid.
auto py_control_get_any(PyObject *obj) -> clingo_control_t *;

//! How a clingo control object is passed to a callback (see py_control_begin()).
enum class ControlRef : uint8_t { share, claim };

//! Begin using a `clingo_control_t` object from Python.
//!
//! Returns a new reference to a `clingo.control.Control` object that must be
//! passed to `py_control_end` with the same reference type.
//!
//! If the reference type is claim, the control must not have a Python owner;
//! otherwise, an error is raised. The returned object owns the control until
//! the matching end call, which invalidates it.
//!
//! If the reference type is share and the control already has a Python owner,
//! that owner is returned. Otherwise, a non-owning object is returned and
//! registered as the control's owner. The `clingo_control_t` object keeps it
//! alive and invalidates it when it is destroyed.
auto py_control_begin(PyObject *type, PyObject *library, clingo_control_t *ctl, ControlRef ref_type) -> PyObject *;

//! End using a `clingo_control_t` object from Python.
//!
//! Steals the reference to the object returned by `py_control_begin`.
//!
//! See `py_control_begin` for the reference type semantics.
void py_control_end(PyObject *obj, ControlRef ref_type) noexcept;

//! Calls py_control_begin() and py_control_end() for a scope.
class ControlGuard {
  public:
    ControlGuard(PyObject *type, PyObject *library, clingo_control_t *ctl, ControlRef ref_type)
        : obj_{py_control_begin(type, library, ctl, ref_type)}, ref_type_{ref_type} {}
    ControlGuard(ControlGuard const &) = delete;
    ControlGuard(ControlGuard &&) = delete;
    auto operator=(ControlGuard const &) -> ControlGuard & = delete;
    auto operator=(ControlGuard &&) -> ControlGuard & = delete;
    ~ControlGuard() {
        if (obj_ != nullptr) {
            py_control_end(obj_, ref_type_);
        }
    }

    //! The control object (nullptr with a Python error set if begin failed).
    [[nodiscard]] auto get() const -> PyObject * { return obj_; }

  private:
    PyObject *obj_;
    ControlRef ref_type_;
};

void register_control(pybind11::module &m);

} // namespace PyClingo
