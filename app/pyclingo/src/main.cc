// {{{ GPL License 

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Benjamin Kaufmann

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#include "Python.h"
#include "gringo/python.hh"
#include "clingo/clingocontrol.hh"

namespace {

DefaultGringoModule g_module;

} // namespace

#if PY_MAJOR_VERSION >= 3
#define INITRETURN return (PyObject *)
#define INITFAIL return nullptr;
PyMODINIT_FUNC PyInit_gringo() {
#else
#define INITRETURN
#define INITFAIL return;
PyMODINIT_FUNC initgringo() {
#endif
    try                             { INITRETURN Gringo::Python::initlib(g_module); }
    catch (std::bad_alloc const &e) { PyErr_SetString(PyExc_MemoryError, e.what()); INITFAIL }
    catch (std::exception const &e) { PyErr_SetString(PyExc_RuntimeError, e.what()); INITFAIL }
    catch (...)                     { PyErr_SetString(PyExc_RuntimeError, "unknown error"); INITFAIL }
}

