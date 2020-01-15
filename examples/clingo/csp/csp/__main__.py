"""
Main module providing the application logic.
"""

import sys
from textwrap import dedent
import clingo
import csp
from .parsing import transform


class Application(object):
    """
    Application class that can be used with `clingo.clingo_main` to solve CSP
    problems.
    """
    def __init__(self):
        self.program_name = "csp"
        self.version = "1.0"
        self._propagator = csp.Propagator()
        self._bound_symbol = None  # CSP variable to minimize
        self._bound_value = None   # current/initial value of CSP variable

    def print_model(self, model, default_printer):
        """
        Print the current model and the assignment to integer variables.
        """
        ass = self._propagator.get_assignment(model.thread_id)

        default_printer()

        print("Valid assignment for constraints found:")
        print(" ".join("{}={}".format(n, v) for n, v in ass))

        if self._bound_symbol is not None:
            print("CSP Optimization: {}".format(self._get_bound(model)))

        sys.stdout.flush()

    def _parse_min(self, value):
        """
        Parse and set minimum value for integer variables.
        """
        # Note: just done like this because otherwise the data-structure for
        #       literals in `VarState` objects would have become a little more
        #       difficult. This should not be done in a real application.
        # pylint: disable=global-statement
        csp.MIN_INT = int(value)
        return True

    def _parse_max(self, value):
        """
        Parse and set maximum value for integer variables.
        """
        # Note: just done like this because otherwise the data-structure for
        #       literals in `VarState` objects would have become a little more
        #       difficult. This should not be done in a real application.
        # pylint: disable=global-statement
        csp.MAX_INT = int(value)
        return True

    def _parse_minimize(self, value):
        """
        Parse and set variable to minimize and an optional initial value.
        """
        term = clingo.parse_term("({},)".format(value))
        args = term.arguments
        size = len(args)
        if size == 0 or size > 2 or (size > 1 and args[1].type != clingo.SymbolType.Number):
            return False
        self._bound_symbol = args[0]
        if size > 1:
            self._bound_value = args[1].number
        return True

    def register_options(self, options):
        """
        Register CSP related options.
        """
        group = "CSP Options"
        options.add(group, "min-int", "Minimum integer [-20]", self._parse_min, argument="<i>")
        options.add(group, "max-int", "Maximum integer [20]", self._parse_max, argument="<i>")
        options.add(group, "minimize-variable", dedent("""\
            Minimize the given variable
                  <arg>   : <variable>[,<initial>]
                  <variable>: the variable to minimize
                  <initial> : upper bound for the variable,
            """), self._parse_minimize)

    def validate_options(self):
        """
        Validate options making sure that the minimum is larger than the
        maximum integer.
        """
        if csp.MIN_INT > csp.MAX_INT:
            raise RuntimeError("min-int must not be larger than max-int")

    def _get_bound(self, model):
        """
        Get the current value of the variable being minimized.
        """
        return self._propagator.get_value(self._bound_symbol, model.thread_id)

    def main(self, prg, files):
        """
        Entry point of the application registering the propagator and
        implementing the standard ground and solve functionality.
        """
        prg.register_propagator(self._propagator)
        prg.add("base", [], csp.THEORY)

        with prg.builder() as b:
            for f in files:
                transform(b, open(f).read())

        prg.ground([("base", [])])
        if self._bound_symbol is None:
            prg.solve()
        else:
            # Note: This is to mirror the implementation in clingo-dl. In
            # principle this could be dealt with differently with order
            # variables.
            with prg.builder() as b:
                transform(b, "#program __bound(s,b). &sum { 1*s } <= b.")

            found = True
            while found:
                if self._bound_value is not None:
                    prg.ground((("__bound", (
                        self._bound_symbol,
                        clingo.Number(self._bound_value - 1))),))

                found = False
                for model in prg.solve(yield_=True):
                    self._bound_value = self._get_bound(model)
                    found = True
                    break


if __name__ == "__main__":
    sys.exit(int(clingo.clingo_main(Application(), sys.argv[1:])))
