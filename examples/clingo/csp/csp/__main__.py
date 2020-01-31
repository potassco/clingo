"""
Main module providing the application logic.
"""

import sys
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

    def print_model(self, model, default_printer):
        """
        Print the current model and the assignment to integer variables.
        """
        ass = self._propagator.get_assignment(model.thread_id)

        default_printer()

        print("Valid assignment for constraints found:")
        print(" ".join("{}={}".format(n, v) for n, v in ass))

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

    def register_options(self, options):
        """
        Register CSP related options.
        """
        group = "CSP Options"
        options.add(group, "min-int", "Minimum integer [-(2**32)]", self._parse_min, argument="<i>")
        options.add(group, "max-int", "Maximum integer [2*32]", self._parse_max, argument="<i>")

    def validate_options(self):
        """
        Validate options making sure that the minimum is larger than the
        maximum integer.
        """
        if csp.MIN_INT > csp.MAX_INT:
            raise RuntimeError("min-int must not be larger than max-int")

    def _on_statistics(self, step, akku):
        self._propagator.on_statistics(step, akku)

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

        for model in prg.solve(on_statistics=self._on_statistics, yield_=True):
            if self._propagator.has_minimize:
                bound = self._propagator.get_minimize_value(model.thread_id)
                self._propagator.update_minimize(bound-1)


if __name__ == "__main__":
    sys.exit(int(clingo.clingo_main(Application(), sys.argv[1:])))
