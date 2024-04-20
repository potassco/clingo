"""
Unit tests for clingo.core module.
"""

from unittest import TestCase

from clingo.core import Library, version


class TestCore(TestCase):
    """
    Unit tests for clingo.core module.
    """

    def test_version(self):
        """
        Test the version function.
        """
        self.assertGreaterEqual(version(), (6, 0, 0))

    def test_library(self):
        """
        Test library creation.
        """
        with Library() as lib:
            self.assertIsNotNone(lib)
