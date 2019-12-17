# A Propagator for Linear Constraints

This example extends ASP programs with linear constraints in rule heads:

    &sum{u-v} <= d :- [rule body].

# Usage

To check if a program with difference constraints is satisfiable, simply pass
`csp.lp` as argument to clingo:

    $ clingo [clingo options] csp.lp [files]

# Example Calls

Finding all solutions for the flow shop problem:

    $ clingo 0 -c bound=14 csp.lp fsE.lp fsI.lp

