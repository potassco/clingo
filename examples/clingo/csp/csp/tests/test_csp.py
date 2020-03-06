"""
Basic tests checking the whole system.
"""

import unittest
from csp.tests import Solver, solve


# pylint: disable=missing-docstring

class TestMain(unittest.TestCase):
    def test_simple(self):
        self.assertEqual(
            solve("""\
            &sum {   1 *y + (-5)*x } <= 0.
            &sum { (-1)*y +   5 *x } <= 0.
            &sum { 15*x } <= 15.
            &sum { 10*x } <= 7.
            """),
            [[('x', -4), ('y', -20)],
             [('x', -3), ('y', -15)],
             [('x', -2), ('y', -10)],
             [('x', -1), ('y', -5)],
             [('x', 0), ('y', 0)]])

        self.assertEqual(
            solve("""\
            &sum {   1 *even + (-2)*_i } <= 0.
            &sum { (-1)*even +   2 *_i } <= 0.
            &sum {   1 *odd + (-2)*_i } <=  1.
            &sum { (-1)*odd +   2 *_i } <= -1.""", -2, 2),
            [[('even', -2), ('odd', -1)],
             [('even', 0), ('odd', 1)]])

        self.assertEqual(
            solve("""\
            a :- &sum{-1*x} <= 0.
            b :- &sum{1*x} <= 5.
            :- not a.
            :- not b."""),
            [['a', 'b', ('x', 0)],
             ['a', 'b', ('x', 1)],
             ['a', 'b', ('x', 2)],
             ['a', 'b', ('x', 3)],
             ['a', 'b', ('x', 4)],
             ['a', 'b', ('x', 5)]])

        self.assertEqual(
            solve("""\
            &sum { 1 * x + (-1) * y } <= -1.
            &sum { 1 * y + (-1) * x } <= -1.
            """), [])

        self.assertEqual(
            solve("""\
            &sum { 1 } <= 2.
            """), [[]])

        self.assertEqual(
            solve("""\
            &sum { 2 } <= 1.
            """), [])

        self.assertEqual(
            solve("""\
            {a}.
            &sum {   1 *x } <= -5 :- a.
            &sum { (-1)*x } <= -5 :- not a.
            """, -6, 6),
            [[('x', 5)],
             [('x', 6)],
             ['a', ('x', -6)],
             ['a', ('x', -5)]])

    def test_parse(self):
        self.assertEqual(solve("&sum { x(f(1+2)) } <= 0.", 0, 0), [[('x(f(3))', 0)]])
        self.assertEqual(solve("&sum { x(f(1-2)) } <= 0.", 0, 0), [[('x(f(-1))', 0)]])
        self.assertEqual(solve("&sum { x(f(-2)) } <= 0.", 0, 0), [[('x(f(-2))', 0)]])
        self.assertEqual(solve("&sum { x(f(2*2)) } <= 0.", 0, 0), [[('x(f(4))', 0)]])
        self.assertEqual(solve("&sum { x(f(4/2)) } <= 0.", 0, 0), [[('x(f(2))', 0)]])
        self.assertEqual(solve("&sum { x(f(9\\2)) } <= 0.", 0, 0), [[('x(f(1))', 0)]])
        self.assertEqual(solve("&sum { (a,b) } <= 0.", 0, 0), [[('(a,b)', 0)]])
        self.assertEqual(solve("&sum { x } = 5."), [[('x', 5)]])
        self.assertEqual(solve("&sum { x } != 0.", -3, 3), [[('x', -3)], [('x', -2)], [('x', -1)], [('x', 1)], [('x', 2)], [('x', 3)]])
        self.assertEqual(solve("&sum { x } < 2.", -3, 3), [[('x', -3)], [('x', -2)], [('x', -1)], [('x', 0)], [('x', 1)]])
        self.assertEqual(solve("&sum { x } > 1.", -3, 3), [[('x', 2)], [('x', 3)]])
        self.assertEqual(solve("&sum { x } >= 1.", -3, 3), [[('x', 1)], [('x', 2)], [('x', 3)]])
        self.assertEqual(solve("a :- &sum { x } >= 1.", -3, 3), [
            [('x', -3)], [('x', -2)], [('x', -1)],
            [('x', 0)],
            ['a', ('x', 1)], ['a', ('x', 2)], ['a', ('x', 3)]])
        self.assertEqual(solve("a :- &sum { x } = 1.", -3, 3), [
            [('x', -3)], [('x', -2)], [('x', -1)],
            [('x', 0)],
            [('x', 2)], [('x', 3)], ['a', ('x', 1)]])
        self.assertEqual(solve("&sum { 5*x + 10*y } = 20.", -3, 3), [[('x', -2), ('y', 3)], [('x', 0), ('y', 2)], [('x', 2), ('y', 1)]])
        self.assertEqual(solve("&sum { -5*x + 10*y } = 20.", -3, 3), [[('x', -2), ('y', 1)], [('x', 0), ('y', 2)], [('x', 2), ('y', 3)]])

    def test_singleton(self):
        self.assertEqual(solve("&sum { x } <= 1.", 0, 2), [[('x', 0)], [('x', 1)]])
        self.assertEqual(solve("&sum { x } >= 1.", 0, 2), [[('x', 1)], [('x', 2)]])
        self.assertEqual(solve("a :- &sum { x } <= 1.", 0, 2), [[('x', 2)], ['a', ('x', 0)], ['a', ('x', 1)]])
        self.assertEqual(solve(":- &sum { x } <= 1.", 0, 2), [[('x', 2)]])
        self.assertEqual(solve(":- not &sum { x } <= 1.", 0, 2), [[('x', 0)], [('x', 1)]])
        self.assertEqual(solve("a :- &sum { x } <= 1. b :- not &sum { x } > 1.", 0, 2), [[('x', 2)], ['a', 'b', ('x', 0)], ['a', 'b', ('x', 1)]])
        self.assertEqual(solve(" :- &sum { x } <= 1. :- not &sum { x } > 1.", 0, 2), [[('x', 2)]])

    def test_distinct(self):
        self.assertEqual(solve("&distinct { x; y }.", 0, 1), [[('x', 0), ('y', 1)], [('x', 1), ('y', 0)]])
        self.assertEqual(solve("&distinct { 2*x; 3*y }.", 2, 3), [[('x', 2), ('y', 2)], [('x', 2), ('y', 3)], [('x', 3), ('y', 3)]])
        self.assertEqual(solve("&distinct { 0*x; 0*y }.", 0, 1), [])
        # TODO: gringo uses sets/maybe I should change that to also support such ugly semantics.
        self.assertEqual(solve("&distinct { 0 }.", 0, 1), [[]])
        self.assertEqual(solve("&distinct { 0; 0+0 }.", 0, 1), [])
        self.assertEqual(solve("&distinct { 0; 1 }.", 0, 1), [[]])
        self.assertEqual(solve("&distinct { 2*x; (1+1)*x }.", 0, 1), [])
        self.assertEqual(solve("&distinct { y-x; x-y }.", 0, 1), [[('x', 0), ('y', 1)], [('x', 1), ('y', 0)]])
        self.assertEqual(solve("&distinct { x; y } :- c. &sum { x } = y :- not c. {c}.", 0, 1), [
            [('x', 0), ('y', 0)],
            [('x', 1), ('y', 1)],
            ['c', ('x', 0), ('y', 1)],
            ['c', ('x', 1), ('y', 0)]])
        self.assertEqual(solve("&dom{1..1}=x. &dom{1..2}=y. &dom{1..3}=z. &distinct{x;y;z}."), [[('x', 1), ('y', 2), ('z', 3)]])
        self.assertEqual(solve("&dom{1..3}=x. &dom{2..3}=y. &dom{3..3}=z. &distinct{x;y;z}."), [[('x', 1), ('y', 2), ('z', 3)]])
        self.assertEqual(
            solve("""\
            &dom { 1..2 } = x.
            &dom { 1..2 } = y.
            &dom { 1..2 } = z.
            &distinct { 2*x+3*y+5*z; 5*x+2*y+3*z; 3*x+5*y+2*z }.
            """),
            [[('x', 1), ('y', 1), ('z', 2)],
             [('x', 1), ('y', 2), ('z', 1)],
             [('x', 1), ('y', 2), ('z', 2)],
             [('x', 2), ('y', 1), ('z', 1)],
             [('x', 2), ('y', 1), ('z', 2)],
             [('x', 2), ('y', 2), ('z', 1)]])
        self.assertEqual(
            solve("""\
            &dom { 1..2 } = x.
            &dom { 1..2 } = y.
            &dom { 1..2 } = z.
            &distinct { 2*x+3*y+5*z+1; 5*x+2*y+3*z; 3*x+5*y+2*z-1 }.
            """),
            [[('x', 1), ('y', 2), ('z', 2)],
             [('x', 2), ('y', 1), ('z', 1)],
             [('x', 2), ('y', 2), ('z', 1)]])

    def test_dom(self):
        self.assertEqual(solve("&dom { 0;1..2;2..3;5 } = x.", -10, 10), [[('x', 0)], [('x', 1)], [('x', 2)], [('x', 3)], [('x', 5)]])
        self.assertEqual(solve("1 {a; b} 1. &dom { 0;2;4 } = x :- a. &dom { 1;3;5 } = x :- b.", -10, 10), [
            ['a', ('x', 0)], ['a', ('x', 2)], ['a', ('x', 4)],
            ['b', ('x', 1)], ['b', ('x', 3)], ['b', ('x', 5)]])
        self.assertEqual(solve("&dom { 0-1..0+1 } = x.", -10, 10), [[('x', -1)], [('x', 0)], [('x', 1)]])

    def test_multishot(self):
        s = Solver(0, 3)
        self.assertEqual(s.solve("&sum { x } <= 2."), [[('x', 0)], [('x', 1)], [('x', 2)]])
        self.assertEqual(s.solve(""), [[('x', 0)], [('x', 1)], [('x', 2)]])
        self.assertEqual(s.solve("&sum { x } <= 1."), [[('x', 0)], [('x', 1)]])
        self.assertEqual(s.solve("&sum { x } <= 0."), [[('x', 0)]])
        self.assertEqual(s.solve("&sum { x } <= 1."), [[('x', 0)]])
        self.assertEqual(s.solve("&sum { x } <= 2."), [[('x', 0)]])

    def test_optimize(self):
        self.assertEqual(solve("&minimize { x }.", -3, 3), [[('x', -3)]])
        self.assertEqual(solve("&minimize { x+6 }.", -3, 3), [[('x', -3)]])
        self.assertEqual(solve("&maximize { 2*x }.", -3, 3), [[('x', 3)]])
        self.assertEqual(solve("&maximize { x + y }. ", -3, 3), [[('x', 3), ('y', 3)]])
        self.assertEqual(solve("&maximize { x + y }. &sum{ x + y} <= 5. ", -3, 3), [[('x', 2), ('y', 3)], [('x', 3), ('y', 2)]])
        self.assertEqual(solve("&maximize { x }. &sum{ x } <= 0 :- a. {a}. ", -3, 3), [[('x', 3)]])
        self.assertEqual(solve("&minimize { x }. &sum{ x } <= 0 :- a. {a}. ", -3, 3), [[('x', -3)], [('a'), ('x', -3)]])
        self.assertEqual(solve("&minimize { x }. a :- &sum{ x } <= 0. ", -3, 3), [[('a'), ('x', -3)]])

    def test_shift(self):
        self.assertEqual(solve("{a}. :- a, &sum { x } < 3. :- not a, &sum { x } > 0.", 0, 3), [[('x', 0)], ['a', ('x', 3)]])

    def test_optimize_bound(self):
        sol = [[('x', 0), ('y', 2), ('z', 0)],
               [('x', 1), ('y', 1), ('z', 1)],
               [('x', 2), ('y', 0), ('z', 2)]]
        for translate_minimize in (True, False):
            s = Solver(0, 3)
            s.prp.config.translate_minimize.value = translate_minimize
            s.solve("&minimize { x + 2 * y + z + 5 }. &sum{ x + y } >= 2. &sum { y + z } >= 2.")
            self.assertEqual(s.bound, 9)
            self.assertEqual(s.solve("", optimize=False, bound=9), sol)
            self.assertEqual(s.solve("", optimize=False, bound=9), sol)
            self.assertEqual(s.solve("&minimize { 6 }.", optimize=False, bound=9), [])
            self.assertEqual(s.solve("", optimize=False, bound=15), sol)
