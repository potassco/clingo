"""
Basic tests checking the whole system.
"""

import unittest
import clingo
import csp


def parse_model(m, prp):
    """
    Combine model and assignment in one list.
    """
    ret = []
    for sym in m.symbols(shown=True):
        ret.append(str(sym))
    ret.extend(prp.get_assignment(m.thread_id))
    return list(sorted(ret))


def solve(s):
    """
    Return the models/assignments of the program in the given string.
    """

    csp.MIN_INT = -20
    csp.MAX_INT = 20

    prp = csp.Propagator()
    prg = clingo.Control(['0'], message_limit=0)
    prg.register_propagator(prp)

    prg.add("base", [], csp.THEORY)
    with prg.builder() as b:
        csp.transform(b, s)

    prg.ground([("base", [])])

    ret = []
    prg.solve(on_model=lambda m: ret.append(parse_model(m, prp)))

    return list(sorted(ret))


# pylint: disable=missing-docstring

class TestMain(unittest.TestCase):
    def test_simple(self):
        self.assertEqual(solve("""\
            &sum {   1 *y + (-5)*x } <= 0.
            &sum { (-1)*y +   5 *x } <= 0.
            &sum { 15*x } <= 15.
            &sum { 10*x } <= 7.
            """), [[('x', -4), ('y', -20)],
                   [('x', -3), ('y', -15)],
                   [('x', -2), ('y', -10)],
                   [('x', -1), ('y', -5)],
                   [('x', 0), ('y', 0)]])
