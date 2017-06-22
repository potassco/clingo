# Modeling Transition Systems

This example is similar to clingo's incremental mode but additionally takes
care of attaching a time parameter to atoms.

Note that with clingo it is not possible to obtain an AST from logic programs
passed via the command line. Hence, the example simply reads a program from the
file `example.lp`.

# Example Calls

    clingo -q2 visitor.lp

