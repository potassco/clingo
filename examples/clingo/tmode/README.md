# Modeling Transition Systems

This example is similar to clingo's incremental mode but additionally takes
care of attaching a time parameter to atoms.

Note that with clingo it is not possible to obtain an AST from logic programs
passed via the command line. Hence, the example embedding Python code simply
reads a program from the file `example.lp`. Furthermore, this example is
incompatible with clingo's CFFI based module.

When using the python module, files can parsed into ASTs. This is shown in the
file `tmode-cffi.py`. Note that this file is incompatible with clingo's CPython
based module.

# Example Calls

    clingo -q2 tmode.lp
    python tmode-cffi.py example.lp
