# Solving the Towers of Hanoi Problem

This example solves Towers of Hanoi problems.  There are two variants.  First,
there is an incremental encoding.  Second, there branch-and-bound-based
optimizing version with respect to a given planning horizon.

# Examples

Using the incremental version:

    $ clingo tohE.lp tohI.lp

Using the bounded version:

    $ clingo opt.lp tohB.lp tohI.lp -c n=30
