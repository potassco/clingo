'''
The clingo module.

This module provides functions and classes to control the grounding and solving
process.

If the clingo application is build with Python support, clingo will also be
able to execute Python code embedded in logic programs.  Functions defined in a
Python script block are callable during the instantiation process using
`@`-syntax.  The default grounding/solving process can be customized if a main
function is provided.

Note that gringo's precomputed terms (terms without variables and interpreted
functions), called symbols in the following, are wrapped in the Symbol class.
Furthermore, strings, numbers, and tuples can be passed wherever a symbol is
expected - they are automatically converted into a Symbol object.  Functions
called during the grounding process from the logic program must either return a
symbol or a sequence of symbols.  If a sequence is returned, the corresponding
`@`-term is successively substituted by the values in the sequence.

## Examples

The first example shows how to use the clingo module from Python.

    >>> import clingo
    >>> class Context:
    ...     def id(self, x):
    ...         return x
    ...     def seq(self, x, y):
    ...         return [x, y]
    ...
    >>> def on_model(m):
    ...     print (m)
    ...
    >>> ctl = clingo.Control()
    >>> ctl.add("base", [], """\
    ... p(@id(10)).
    ... q(@seq(1,2)).
    ... """)
    >>> ctl.ground([("base", [])], context=Context())
    >>> ctl.solve(on_model=on_model)
    p(10) q(1) q(2)
    SAT

The second example shows how to use Python code from clingo.

    #script (python)

    import clingo

    class Context:
        def id(x):
            return x

        def seq(x, y):
            return [x, y]

    def main(prg):
        prg.ground([("base", [])], context=Context())
        prg.solve()

    #end.

    p(@id(10)).
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
