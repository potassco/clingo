# A Propagator for Linear Constraints

This example extends ASP programs with linear constraints in rule heads:

    &sum{u-v} <= d :- [rule body].

# Usage

To check if a program with difference constraints is satisfiable, simply pass
`csp.lp` as argument to clingo:

    $ clingo [clingo options] csp.lp [files]

# Example Calls

Finding all solutions for the flow shop problem:

    $ clingo 0 -c bound=16 csp.lp fsE.lp fsI.lp

The problem can also be solved using clingo's python module, which also gives
options to configure the minimum and maximum values for variables.

    $ python csp.py 0 -c bound=17 --min-int=1 --max-int=17 fsE.lp fsI.lp
