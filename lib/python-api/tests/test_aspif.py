"""
Unit tests for the aspif parser.
"""

import os
import tempfile
from contextlib import contextmanager
from textwrap import dedent

from clingo.control import Control
from clingo.core import Library
from util import MCB


@contextmanager
def make_file(content: str):
    """
    Context manager that creates a temporary file with initial content.

    Ensures proper cleanup even if exceptions occur during file operations.
    """
    temp_name = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w", delete=False, encoding="utf-8"
        ) as temp_file:
            # Write content immediately
            temp_file.write(content)
            temp_name = temp_file.name
        yield temp_name

    finally:
        if temp_name is not None and os.path.exists(temp_name):
            try:
                os.remove(temp_name)
            except (OSError, PermissionError):
                pass


class TestAspif:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for the aspif parser module.
    """

    def setup_method(self, method):
        """
        Create lib.
        """
        assert method is not None
        self._lib = Library()
        self._ctl = Control(self._lib, ["0"])

    def teardown_method(self, method):
        """
        Destroy lib.
        """
        assert method is not None
        self._ctl = None
        self._lib = None

    @property
    def lib(self) -> Library:
        """
        Get the library object.
        """
        assert self._lib is not None
        return self._lib

    @property
    def ctl(self) -> Control:
        """
        Get the control object.
        """
        assert self._ctl is not None
        return self._ctl

    def parse(self, content: str):
        """
        Parse the given string in aspif format.
        """
        with make_file(dedent(content)) as path:
            self.ctl.parse_files([path])

    def test_rule(self):
        """
        Test adding aspif rules.
        """
        self.parse(
            """\
            asp 1 0 0
            1 0 1 1 0 0
            1 0 1 2 0 0
            1 0 1 3 0 0
            1 0 1 4 0 1 -5
            1 0 1 6 0 1 -7
            1 0 1 8 0 1 -9
            1 0 1 5 0 1 -4
            1 0 1 7 0 1 -6
            1 0 1 9 0 1 -8
            4 4 q(1) 1 4
            4 4 p(1) 1 5
            4 4 q(2) 1 6
            4 4 p(2) 1 7
            4 4 q(3) 1 8
            4 4 p(3) 1 9
            0
            """
        )
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [
            ["p(1)", "p(2)", "p(3)"],
            ["p(1)", "p(2)", "q(3)"],
            ["p(1)", "p(3)", "q(2)"],
            ["p(1)", "q(2)", "q(3)"],
            ["p(2)", "p(3)", "q(1)"],
            ["p(2)", "q(1)", "q(3)"],
            ["p(3)", "q(1)", "q(2)"],
            ["q(1)", "q(2)", "q(3)"],
        ]
