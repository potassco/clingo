'''
Module providing functions and classes to control the grounding and solving
process.

Next follow some concepts used throughout the modules of the clingo package.

Terms
-----
Terms without variables and interpreted functions are called symbols in the
following. They are wrapped in the `clingo.symbol.Symbol` class.

Atoms and Literals
------------------
Atoms are also captured using the `clingo.symbol.Symbol` class. They must be of
type `clingo.symbol.SymbolType.Function`. Some functions accept literals in
form of pairs of symbols and Booleans. The Boolean stands for the sign of the
literal (`True` for positive and `False` for negative).

Program Literals
----------------
*Program literals* are non-zero integers associated with symbolic atoms, theory
atoms, and also without any association if they are used to translate complex
language constructs. The sign of a program literal is used to represent default
negation. Symbolic atoms can be mapped to program literals using the
`clingo.symbolic_atoms` module, theory atoms using the `clingo.theory_atoms`
module, and the `clingo.backend` can also be used to introduce fresh program
literals.

Solver Literals
---------------
Furthermore, there are non-zero integer *solver literals*. There is a
surjective mapping from program atoms to solver literals. The
`clingo.propagator.PropagateInit.solver_literal` function can be used to map
program literals to solver literals.

Embedded Python Code
--------------------
If the clingo application is build with Python support, clingo will also be
able to execute Python code embedded in logic programs.  Functions defined in a
Python script block are callable during the instantiation process using
`@`-syntax.  The default grounding/solving process can be customized if a main
function is provided.

## Examples

The first example shows how to use the clingo module from Python.

    >>> from clingo.symbol import Number
    >>> from clingo.control import Control
    >>>
    >>> class Context:
    ...     def inc(self, x):
    ...         return Number(x.number + 1)
    ...     def seq(self, x, y):
    ...         return [x, y]
    ...
    >>> def on_model(m):
    ...     print (m)
    ...
    >>> ctl = Control()
    >>> ctl.add("base", [], """\\
    ... p(@inc(10)).
    ... q(@seq(1,2)).
    ... """)
    >>> ctl.ground([("base", [])], context=Context())
    >>> print(ctl.solve(on_model=on_model))
    p(11) q(1) q(2)
    SAT

The second example shows how to use Python code from clingo.

    #script (python)

    from clingo.symbol import Number

    class Context:
        def inc(x):
            return Number(x.number)

        def seq(x, y):
            return [x, y]

    def main(prg):
        prg.ground([("base", [])], context=Context())
        prg.solve()

    #end.

    p(@inc(10)).
    q(@seq(1,2)).
'''

from .core import *
from .symbol import *
from .symbolic_atoms import *
from .theory_atoms import *
from .solving import *
from .propagator import *
from .backend import *
from .configuration import *
from .statistics import *
from .control import *
from .application import *

__version__ = '.'.join(map(str, version()))
