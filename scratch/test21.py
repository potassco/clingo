"""
Example program adding single parsed statement to program.
"""

import clingo


def main(lib):
    """
    Run the main function.
    """
    ctl = clingo.control.Control(lib, [])
    ctl.parse_string("a.")

    prg = clingo.ast.Program(lib)
    prg.add(clingo.ast.parse_statement(lib, "b :- a."))
    ctl.join(prg)

    ctl.ground([("base", [])])
    ctl.solve()


def run():
    """
    Setup library object and run main.
    """
    lib = clingo.core.Library()
    main(lib)


if __name__ == "__main__":
    run()
