"""
Tests checking utility functions.
"""

import unittest
import csp.util as util


# pylint: disable=missing-docstring

class TestMain(unittest.TestCase):
    def test_intervals(self):
        x = util.IntervalSet()
        x.add(1, 2)
        self.assertEqual(list(x.items()), [(1, 2)])
        x.add(3, 4)
        self.assertEqual(list(x.items()), [(1, 2), (3, 4)])
        x.add(4, 5)
        self.assertEqual(list(x.items()), [(1, 2), (3, 5)])
        x.add(0, 1)
        self.assertEqual(list(x.items()), [(0, 2), (3, 5)])
        x.add(2, 3)
        self.assertEqual(list(x.items()), [(0, 5)])
        x.add(-1, 6)
        self.assertEqual(list(x.items()), [(-1, 6)])
