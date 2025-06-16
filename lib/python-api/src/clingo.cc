#include "clingo.hh"
#include "app.hh"
#include "ast.hh"
#include "backend.hh"
#include "base.hh"
#include "config.hh"
#include "control.hh"
#include "core.hh"
#include "propagate.hh"
#include "script.hh"
#include "solve.hh"
#include "stats.hh"
#include "symbol.hh"
#include "theory.hh"

#include <pybind11/pybind11.h>

namespace PyClingo {

void register_clingo(pybind11::module &m) {
    m.doc() = R"doc(
Module providing functions and classes to control the grounding and solving
process of Answer Set Programming (ASP).

# Introduction

This module offers a comprehensive set of tools for interacting with the clingo
ASP solver, allowing users to control grounding and solving processes
programmatically.

# Key Concepts

## Terms

Terms without variables and interpreted functions are called symbols in the
following. They are wrapped in the `clingo.symbol.Symbol` class.

## Symbolic Atoms and Literals

*Symbolic atoms* without variables and interpreted functions, which appear in
ground logic programs, are captured using the `clingo.symbol.Symbol` class.
They must be of type `clingo.symbol.SymbolType.Function`. Furthermore, some
functions accept *symbolic literals*, which are represented as pairs of symbols
and Booleans. The Boolean stands for the sign of the literal (`True` for
positive and `False` for negative).

## Program Literals

When passing a ground logic program to a solver, clingo does not use a human
readable textual representation but the aspif format. The literals in this
format are called *program literals*. They are non-zero integers associated
with symbolic atoms, theory atoms, and also without any association if they are
used to translate complex language constructs not directly representable in
aspif format. The sign of a program literal is used to represent default
negation. Symbolic and theory atoms can be mapped to program literals using the
`clingo.base` module. Note that symbolic and theory atoms can share the same
program literals. Finally, the `clingo.backend` module can also be used to
introduce fresh symbolic atoms and program literals.

## Solver Literals

Before solving, programs in aspif format are translated to an internal solver
representation, where program literals are again mapped to non-zero integers,
so called *solver literals*. The `clingo.propagate.PropagateInit.solver_literal`
function can be used to map program literals to solver literals. Note that
different program literals can share the same solver literal.

# Embedded Python Code

If the clingo application is built with Python support, clingo will also be
able to execute Python code embedded in logic programs. Functions defined in a
Python script block are callable during the instantiation process using
`@`-syntax. The default grounding/solving process can be customized if a main
function is provided.

# Examples

The first example shows how to use Python code from clingo:

```python
#script (python)

from typing import Sequence
from clingo.core import Library
from clingo.control import Control
from clingo.symbol import Number, Symbol

Parts = Sequence[Sequence[tuple[str, Sequence[Symbol]]]]

def f(lib: Library, x: Symbol):
	return Number(lib, x.number)

def main(lib: Library, ctl: Control, parts: Parts):
    for part in parts:
        ctl.ground(part)
        with ctl.solve() as hnd:
            hnd.get()

#end.

1 {p(1..2)} 1.
q(@f(X)) :- p(X).
```

The second example shows how to use the `clingo` module from Python. Note the
use of a context object here. In fact, it is not possible (by default) to call
functions from the main scope. See the `clingo.script` module for more
information.

```python
>>> from clingo.core import Library
>>> from clingo.control import Control
>>> from clingo.symbol import Number, Symbol
...
>>> class Context:
...     def __init__(self, lib: Library) -> None:
...         self.lib = lib
...     def f(self, x: Symbol):
...         return Number(self.lib, x.number + 1)
...
>>> with Library() as lib:
...     ctl = Control(lib, ["0"])
...     ctl.parse_string("1 {p(1..2)} 1. q(@f(X)) :- p(X).")
...     ctl.ground(context=Context(lib))
...     with ctl.solve(on_model=print) as hnd:
...         print(hnd.get())
```
)doc"_d;
    PyClingo::register_core(m);
    PyClingo::register_symbol(m);
    PyClingo::AST::register_ast(m);
    PyClingo::register_base(m);
    PyClingo::register_backend(m);
    PyClingo::register_solve(m);
    PyClingo::register_config(m);
    PyClingo::register_stats(m);
    PyClingo::register_propagate(m);
    PyClingo::register_control(m);
    PyClingo::register_script(m);
    PyClingo::register_app(m);
    PyClingo::register_theory(m);
}

} // namespace PyClingo
