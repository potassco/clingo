# A Propagator for Linear Constraints

This example extends ASP programs with linear constraints in rules:

    &sum{u-v} <= d :- [rule body].
    [rule head] :- &sum{u-v} <= d, [rule body].

# Usage and Example Calls

Finding all solutions for the flow shop problem.
The problem can be solved using clingo's python module, which also gives
options to configure the minimum and maximum values for variables.

    $ python -m csp 0 -c bound=17 --min-int=1 --max-int=17 examples/fsE.lp examples/fsI.lp

Furthemore, there is an encoding to minimize a set of CSP variables:

    $ python -m csp 0 --min-int=0 --max-int=30 examples/fsO.lp examples/fsI.lp

To have some more instances to play with, clingo-dl's `diff` atoms are also
accepted:

    $ python -m csp 0 --min-int=0 --max-int=30 -c bound=16 examples/fsD.lp examples/fsI.lp

# Running the Unit Tests

You can run the tests with

    $ python -m unittest
