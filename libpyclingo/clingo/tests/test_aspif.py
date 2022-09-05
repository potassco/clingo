'''
Tests for aspif parsing.
'''
from unittest import TestCase
from textwrap import dedent

from ..control import Control
from .util import solve


def solve_aspif(prg):
    ctl = Control()
    ctl.add(dedent(prg))
    return solve(ctl)


class TestASPIF(TestCase):
    '''
    Test ASPIF parsing.
    '''

    def test_preamble(self):
        '''
        Test preamble parsing.
        '''
        ctl = Control()
        ctl.add(dedent("""\
            asp 1 0 0
            0
            """))
        ctl = Control([], lambda msg, code: None)
        self.assertRaises(RuntimeError, ctl.add, dedent("""\
            asp 1 0 0 unknown
            0
            """))
        ctl = Control([], lambda msg, code: None)
        ctl.add(dedent("""\
            asp 1 0 0 incremental
            0
            """))
        self.assertRaises(RuntimeError, ctl.add, dedent("""\
            asp 1 0 0 incremental
            0
            """))
        ctl = Control([], lambda msg, code: None)
        self.assertRaises(RuntimeError, ctl.add, dedent("""\
            asp 1 0 0 incremental
            0
            0
            """))

    def test_rule(self):
        '''
        Test rule parsing.
        '''
        # a fact
        self.assertEqual(solve_aspif("""\
            asp 1 0 0
            1 0 1 1 0 0
            4 1 a 1 1
            0
            """), [["a"]])
        # a choice
        self.assertEqual(solve_aspif("""\
            asp 1 0 0
            1 1 1 1 0 0
            4 1 a 1 1
            0
            """), [[], ["a"]])
        # a rule with normal body
        self.assertEqual(solve_aspif("""\
            asp 1 0 0
            1 1 1 1 0 0
            1 0 1 2 0 1 1
            1 0 1 3 0 1 -2
            4 1 a 1 1
            4 1 b 1 2
            4 1 c 1 3
            0
            """), [["a", "b"], ["c"]])
        # a rule with weight body
        self.assertEqual(solve_aspif("""\
            asp 1 0 0
            1 1 2 1 2 0 0
            1 0 1 3 1 2 2 1 1 2 1
            1 0 0 0 1 -3
            4 1 a 1 1
            4 1 b 1 2
            0
            """), [["a", "b"]])

    def test_minimize(self):
        '''
        Test minimize parsing.
        '''
        self.assertEqual(solve_aspif("""\
            asp 1 0 0
            1 1 2 1 2 0 0
            1 0 0 0 1 -2
            1 0 0 0 1 -1
            2 1 1 1 1
            2 2 1 2 2
            4 1 a 1 1
            4 1 b 1 2
            0
            """), [["a", "b"]])

    def test_external(self):
        '''
        Test external parsing.
        '''
        self.assertEqual(solve_aspif("""\
            asp 1 0 0
            5 1 0
            4 1 a 1 1
            0
            """), [[], ["a"]])
        self.assertEqual(solve_aspif("""\
            asp 1 0 0
            5 1 1
            4 1 a 1 1
            0
            """), [["a"]])
        self.assertEqual(solve_aspif("""\
            asp 1 0 0
            5 1 2
            4 1 a 1 1
            0
            """), [[]])

        self.assertEqual(solve_aspif("""\
            asp 1 0 0
            5 1 3
            4 1 a 1 1
            0
            """), [[]])

    def test_assume(self):
        '''
        Test assumption parsing.
        '''
        self.assertEqual(solve_aspif("""\
            asp 1 0 0
            1 1 2 1 2 0 0
            6 2 1 -2
            4 1 a 1 1
            4 1 b 1 2
            0
            """), [["a"]])

    def test_heuristic(self):
        '''
        Test heuristic parsing.
        '''
        self.assertEqual(solve_aspif("""\
            asp 1 0 0
            1 1 1 1 0 0
            7 4 1 1 2 0
            4 1 a 1 1
            0
            """), [[], ["a"]])

    def test_edge(self):
        '''
        Test edge parsing.
        '''
        self.assertEqual(solve_aspif("""\
            asp 1 0 0
            1 1 1 1 0 0
            8 0 1 1 1
            8 1 0 0
            4 1 a 1 1
            0
            """), [[]])

    def test_theory(self):
        '''
        Test theory parsing.
        '''
