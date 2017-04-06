# A Propagator for Difference Constraints

This example extends ASP programs with difference constraints in rule heads:

    &diff{u-v} <= d :- [rule body].

# Usage

To check if a program with difference constraints is satisfiable, simply pass
`dl.lp` as argument to clingo:

    $ clingo [clingo options] dl.lp [files]

To additionally optimize variable `bound`, pass `dlO.lp` to clingo instead:

    $ clingo [clingo options] dlO.lp [files]

# Example Calls

Finding all solutions for the flow shop problem:

    $ clingo 0 dl.lp fsE.lp fsI.lp

Finding an optimal solution for the flow shop problem:

    $ clingo dlO.lp fsE.lp fsI.lp
