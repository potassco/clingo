#include "clingo.hh"
#include "ast.hh"
#include "base.hh"
#include "config.hh"
#include "control.hh"
#include "core.hh"
#include "script.hh"
#include "solving.hh"
#include "statistics.hh"
#include "symbol.hh"

#include <pybind11/pybind11.h>

namespace Clingo::Python {

void register_clingo(pybind11::module &m) {
    m.doc() = R"doc(The clingo python module.

Module providing functions and classes to control the grounding and solving
process.

Next follow some concepts used throughout the modules of the clingo package.

Terms
-----
Terms without variables and interpreted functions are called symbols in the
following. They are wrapped in the `clingo.symbol.Symbol` class.

Symbolic Atoms and Literals
---------------------------
*Symbolic atoms* without variables and interpreted functions, which appear in
ground logic programs, are captured using the `clingo.symbol.Symbol` class.
They must be of type `clingo.symbol.SymbolType.Function`. Furthermore, some
functions accept *symbolic literals*, which are represented as pairs of symbols
and Booleans. The Boolean stands for the sign of the literal (`True` for
positive and `False` for negative).

Program Literals
----------------
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

Solver Literals
---------------
Before solving, programs in aspif format are translated to an internal solver
representation, where program literals are again mapped to non-zero integers,
so called *solver literals*. The `clingo.propagator.PropagateInit.solver_literal`
function can be used to map program literals to solver literals. Note that
different program literals can share the same solver literal.

Embedded Python Code
--------------------
If the clingo application is build with Python support, clingo will also be
able to execute Python code embedded in logic programs. Functions defined in a
Python script block are callable during the instantiation process using
`@`-syntax. The default grounding/solving process can be customized if a main
function is provided.

## Examples

The first example shows how to use the clingo module from Python.

```python
>>> from clingo.core import Library
>>> from clingo.control import Control
>>>
>>> with Library() as lib:
...     ctl = Control(lib, ["0"])
...     ctl.parse_string("1 {a; b} 1.")
...     ctl.ground()
...     with ctl.solve(on_model=print) as hnd:
...         print(hnd.get())
a
b
SAT
```

The second example shows how to use Python code from clingo.
```python
#script (python)

from clingo.core import Library
from clingo.control import Control

def main(lib: Library, ctl: Control):
    ctl.ground(context=Context(lib))
    with ctl.solve() as hnd:
        hnd.get()

#end.

1 {a; b} 1.
```
)doc";
    Clingo::Python::register_core(m);
    Clingo::Python::register_symbol(m);
    Clingo::Python::register_ast(m);
    Clingo::Python::register_base(m);
    Clingo::Python::register_solving(m);
    Clingo::Python::register_config(m);
    Clingo::Python::register_statistics(m);
    Clingo::Python::register_control(m);
    Clingo::Python::register_script(m);
}

} // namespace Clingo::Python
