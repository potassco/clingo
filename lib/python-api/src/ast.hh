#pragma once

#include "core.hh"
#include "util.hh"

#include <clingo/ast.h>

#include <pybind11/pybind11.h>

namespace PyClingo::AST {

namespace py = pybind11;

class Program {
  public:
    explicit Program(Library &lib);
    // NOLINTNEXTLINE
    operator clingo_program_t *() { return prg_.get(); }

  private:
    static void free(clingo_program_t *prg) noexcept;

    owner_ptr<clingo_program_t, free> prg_;
};

//! Get the managed clingo_ast_t pointer.
//!
//! The returned pointer is owned by the given handle.
auto convert_stm(py::handle hnd) -> clingo_ast_t *;
//! Convert the given ast into a python object.
//!
//! The returned object holds a copy of the given ast.
auto convert_stm(clingo_ast_t *ast) -> py::object;

//! Get the clingo program of a `clingo.ast.Program` object.
//!
//! Returns nullptr with a Python error set if the object is not a program.
inline auto py_ast_program_get(PyObject *obj) -> clingo_program_t * {
    try {
        return py::cast<Program &>(py::handle{obj});
    } catch (py::error_already_set &e) {
        e.restore();
    } catch (py::cast_error const &) {
        PyErr_SetString(PyExc_TypeError, "expected clingo.ast.Program");
    } catch (std::exception const &e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
    }
    return nullptr;
}

void register_ast(pybind11::module &m);

} // namespace PyClingo::AST
