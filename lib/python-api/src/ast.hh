#pragma once

#include "core.hh"
#include "util.hh"

#include <clingo/ast.h>

#include <pybind11/pybind11.h>

namespace PyClingo {

namespace py = pybind11;

class Program {
  public:
    Program(Library &lib);
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

void register_ast(pybind11::module &m);

} // namespace PyClingo
