# Solving the Towers of Hanoi Problem

This example solves Towers of Hanoi problems.  There are two variants.  First,
there is an incremental encoding.  Second, there is an optimizing version that
tries to find a shortest plan within a given planning horizon.

# Example Calls

Using the incremental version:

    $ clingo tohE.lp tohI.lp

Using the bounded version:

    $ clingo opt.lp tohB.lp tohI.lp -c n=30
