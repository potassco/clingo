#pragma once

#include <Python.h>

#include <clingo/ast.h>

namespace PyClingo {

//! Create the clingo.ast module with its own types and functions (first registration phase).
auto create_ast(PyObject *parent) -> PyObject *;
//! Store the types of other modules in the state of the clingo.ast module (second registration phase).
auto link_ast(PyObject *mod) -> bool;

//! Get the `clingo.ast.AST` type (new reference).
auto py_ast_type() -> PyObject *;

//! Create an AST object for the given clingo AST.
//!
//! The reference to the clingo AST is taken over (also on failure). Returns a
//! new reference or nullptr with a Python error set.
auto py_ast_new(PyObject *type, clingo_ast_t *ast) -> PyObject *;

//! Get the clingo AST of a `clingo.ast.AST` object.
//!
//! The AST is owned by the object. Returns nullptr with a Python error set if
//! the object is not an AST.
auto py_ast_get(PyObject *type, PyObject *obj) -> clingo_ast_t *;

//! Get the `clingo.ast.Program` type (new reference).
auto py_ast_program_type() -> PyObject *;

//! Get the clingo program of a `clingo.ast.Program` object.
//!
//! Returns nullptr with a Python error set if the object is not a program.
auto py_ast_program_get(PyObject *type, PyObject *obj) -> clingo_program_t *;

} // namespace PyClingo
