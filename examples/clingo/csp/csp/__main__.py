"""
Main module providing the application logic.
"""

import sys
from textwrap import dedent
from collections import OrderedDict
import clingo
from csp import transform, THEORY, Propagator


_FALSE = ["0", "no", "false"]
_TRUE = ["1", "yes", "true"]


class AppConfig(object):
    """
    Class for application specific options.
    """
    def __init__(self):
        self.shift_constraints = clingo.Flag(True)


class Application(object):
    """
    Application class that can be used with `clingo.clingo_main` to solve CSP
    problems.
    """
    def __init__(self):
        self.program_name = "csp"
        self.version = "1.0"
        self._propagator = Propagator()
        self.config = AppConfig()
        self.occurrences = OrderedDict()
        self.todo = []

    def print_model(self, model, default_printer):
        """
        Print the current model and the assignment to integer variables.
        """
        default_printer()

        sys.stdout.write("Valid assignment for constraints found:\n")
        sep = ""
        for n, v in self._propagator.get_assignment(model.thread_id):
            sys.stdout.write("{}{}={}".format(sep, n, v))
            sep = " "
        sys.stdout.write("\n")
        if self._propagator.has_minimize:
            sys.stdout.write("Cost: {}\n".format(self._propagator.get_minimize_value(model.thread_id)))

        sys.stdout.flush()

    def _parse_int(self, config, attr, min_value=None, max_value=None):
        """
        Parse and set value for integer variable [sup..inf].
        """
        def __parse(value):
            num = int(value)
            if min_value is not None and num < min_value:
                return False
            if max_value is not None and num > max_value:
                return False
            setattr(config, attr, num)
            return True
        return __parse

    def _parse_bool_thread(self, attr):
        """
        Parse boolean variable to store in getattr(config,attr) and also getattr(config,attr+"threads").
        """
        def __parse(value):
            config = self._propagator.config
            l = value.lower().split(",")
            if len(l) > 2:
                return False
            enable = l[0] in _TRUE
            if not enable and l[0] not in _FALSE:
                return False
            if len(l) == 1:
                self.occurrences.setdefault(attr, 0)
                self.occurrences[attr] += 1
                setattr(config.default_state_config, attr, enable)
            else:
                i = int(l[1])
                if not 0 <= i < 64:
                    return False
                self.occurrences.setdefault((attr, i), 0)
                self.occurrences[(attr, i)] += 1
                self.todo.append(lambda: setattr(config.state_config(i), attr, enable))
            return True
        return __parse

    def _flag_str(self, flag):
        return "yes" if flag else "no"

    def register_options(self, options):
        """
        Register CSP related options.
        """
        conf = self._propagator.config
        group = "CSP Options"

        # translation
        options.add_flag(
            group, "shift-constraints",
            "Shift constraints into head of integrity constraints [{}]".format(self._flag_str(self.config.shift_constraints.value)),
            self.config.shift_constraints)
        options.add_flag(
            group, "sort-constraints",
            "Sort constraint elements [{}]".format(self._flag_str(conf.sort_constraints)),
            conf.sort_constraints)
        options.add(
            group, "translate-clauses",
            "Restrict translation to <n> clauses per constraint [{}]".format(conf.clause_limit),
            self._parse_int(conf, "clause_limit", min_value=0), argument="<n>")
        options.add_flag(
            group, "literals-only",
            "Only create literals during translation but no clauses [{}]".format(self._flag_str(conf.literals_only)),
            conf.literals_only)
        options.add(
            group, "translate-pb",
            "Restrict translation to <n> literals per pb constraint [{}]".format(conf.weight_constraint_limit),
            self._parse_int(conf, "weight_constraint_limit", min_value=0), argument="<n>")
        options.add(
            group, "translate-distinct",
            "Restrict translation of distinct constraint to <n> pb constraints [{}]".format(conf.distinct_limit),
            self._parse_int(conf, "distinct_limit", min_value=0), argument="<n>")
        options.add_flag(
            group, "translate-opt",
            "Translate minimize constraint into clasp's minimize constraint [{}]\n".format(self._flag_str(conf.translate_minimize)),
            conf.translate_minimize)

        # propagation
        options.add(
            group, "refine-reasons",
            dedent("""\
            Refine reasons during propagation [{}]
                  <arg>: {{yes|no}}[,<i>]
                  <i>: Only enable for thread <i>\
            """.format(self._flag_str(conf.default_state_config.refine_reasons))),
            self._parse_bool_thread("refine_reasons"), True)
        options.add(
            group, "refine-introduce",
            dedent("""\
            Introduce order literals when generating reasons [{}]
                  <arg>: {{yes|no}}[,<i>]
                  <i>: Only enable for thread <i>\
            """.format(self._flag_str(conf.default_state_config.refine_introduce))),
            self._parse_bool_thread("refine_introduce"), True)
        options.add(
            group, "propagate-chain",
            dedent("""\
            Use closest order literal as reason [{}]
                  <arg>: {{yes|no}}[,<i>]
                  <i>: Only enable for thread <i>\
            """.format(self._flag_str(conf.default_state_config.propagate_chain))),
            self._parse_bool_thread("propagate_chain"), True)

        # hidden/debug
        options.add(
            group, "min-int,@2",
            "Minimum integer [{}]".format(conf.min_int),
            self._parse_int(conf, "min_int"), argument="<i>")
        options.add(
            group, "max-int,@2",
            "Maximum integer [{}]".format(conf.max_int),
            self._parse_int(conf, "max_int"), argument="<i>")
        options.add_flag(
            group, "check-solution,@2",
            "Verify solutions [{}]".format(self._flag_str(conf.check_solution)),
            conf.check_solution)
        options.add_flag(
            group, "check-state,@2",
            "Verify state consistency [{}]".format(self._flag_str(conf.check_state)),
            conf.check_state)

    def validate_options(self):
        """
        Validate options.
        """
        # validate min-/max-int
        if self._propagator.config.min_int > self._propagator.config.max_int:
            raise RuntimeError("min-int must not be larger than max-int")

        # validate thread-specific options
        for opt, n in self.occurrences.items():
            if n > 1:
                raise RuntimeError("multiple occurrences of {}".format(opt))

        # apply thread-specific options
        for f in self.todo:
            f()

    def _on_statistics(self, step, akku):
        self._propagator.on_statistics(step, akku)

    def _read(self, path):
        if path == "-":
            return sys.stdin.read()
        with open(path) as file_:
            return file_.read()

    def main(self, prg, files):
        """
        Entry point of the application registering the propagator and
        implementing the standard ground and solve functionality.
        """
        prg.register_propagator(self._propagator)
        prg.add("base", [], THEORY)

        if not files:
            files = ["-"]

        with prg.builder() as b:
            for path in files:
                transform(b, self._read(path), self.config.shift_constraints)

        prg.ground([("base", [])])

        for model in prg.solve(on_statistics=self._on_statistics, yield_=True):
            if self._propagator.has_minimize:
                bound = self._propagator.get_minimize_value(model.thread_id)
                self._propagator.update_minimize(bound-1)


if __name__ == "__main__":
    sys.exit(int(clingo.clingo_main(Application(), sys.argv[1:])))
