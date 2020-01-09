# A Propagator for Linear Constraints

This example extends ASP programs with linear constraints in rules:

    &sum{u-v} <= d :- [rule body].
    [rule head] :- &sum{u-v} <= d, [rule body].

# Usage and Example Calls

Finding all solutions for the flow shop problem.
The problem can be solved using clingo's python module, which also gives
options to configure the minimum and maximum values for variables.

    $ python csp.py 0 -c bound=17 --min-int=1 --max-int=17 tests/fsE.lp tests/fsI.lp

Furthemore, there is an option to minimize a CSP variable:

    $ python csp.py 0 --min-int=0 --max-int=30 --minimize-variable=bound tests/fsO.lp tests/fsI.lp

To have some more instances to play with, clingo-dl's `diff` atoms are also
accepted:

    $ python csp.py 0 --min-int=0 --max-int=30 --minimize-variable=bound tests/fsD.lp tests/fsI.lp
