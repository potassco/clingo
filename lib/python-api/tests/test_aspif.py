"""
Unit tests for the aspif parser.
"""

import os
import tempfile
from contextlib import contextmanager
from textwrap import dedent

from clingo.control import Control
from clingo.core import Library
from clingo.symbol import Function
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
            temp_file.write(content)
            temp_name = temp_file.name
        yield temp_name

    finally:
        try:
            if temp_name is not None and os.path.exists(temp_name):
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
        self._ctl = Control(self._lib, ["0", "--opt-mode=optN"])

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
        Test adding normal rules.
        """
        self.parse("""\
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
            """)
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

        self.ctl.parse_string("r(X) :- q(X-1), p(X), q(X+1).")
        self.ctl.ground()
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [
            ["p(1)", "p(2)", "p(3)"],
            ["p(1)", "p(2)", "q(3)"],
            ["p(1)", "p(3)", "q(2)"],
            ["p(1)", "q(2)", "q(3)"],
            ["p(2)", "p(3)", "q(1)"],
            ["p(2)", "q(1)", "q(3)", "r(2)"],
            ["p(3)", "q(1)", "q(2)"],
            ["q(1)", "q(2)", "q(3)"],
        ]

    def test_choice(self):
        """
        Test adding choice rules.
        """
        self.parse("""\
            asp 1 0 0
            1 1 3 1 2 3 0 0
            1 0 0 1 2 3 1 1 2 1 3 1
            4 1 a 1 1
            4 1 b 1 2
            4 1 c 1 3
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [[], ["a"], ["b"], ["c"]]

    def test_disjunction(self):
        """
        Test adding disjunctive rules.
        """
        self.parse("""\
            asp 1 0 0
            1 0 1 4 0 1 3
            1 0 1 3 0 1 4
            1 0 1 1 0 1 2
            1 0 1 2 0 1 1
            1 0 2 1 2 0 2 -3 -4
            1 0 2 3 4 0 2 -1 -2
            4 1 a 1 1
            4 1 b 1 2
            4 1 c 1 3
            4 1 d 1 4
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [["a", "b"], ["c", "d"]]

    def test_minimize(self):
        """
        Test adding minimize constraints.
        """
        self.parse("""\
            asp 1 0 0
            1 0 1 4 0 0
            1 0 0 0 1 -5
            1 1 3 1 2 3 0 0
            1 0 0 0 2 1 -3
            1 0 1 5 0 2 1 2
            1 0 1 5 0 2 1 3
            1 0 1 5 0 2 2 3
            2 0 3 3 3 2 2 1 1
            4 1 a 1 1
            4 1 b 1 2
            4 1 c 1 3
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [["a", "c"]]

    def test_project(self):
        """
        Test adding project directives.
        """
        self._ctl = Control(self.lib, ["--project", "0"])
        self.parse("""\
            asp 1 0 0
            1 0 1 1 0 0
            1 0 0 0 1 -4
            1 0 1 6 0 1 4
            1 1 2 2 3 0 0
            1 0 1 4 0 1 3
            1 0 1 4 0 1 2
            1 0 0 0 2 3 2
            4 1 a 1 3
            3 2 2 3
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [[], ["a"]]

    def test_output_v1_fact(self):
        """
        Test output fact.
        """
        self.parse("""\
            asp 1 0 0
            1 0 1 1 0 0
            4 1 a 0
            0
            """)
        sym = Function(self.lib, "a", [])
        assert sym not in self.ctl.base.terms
        assert sym in self.ctl.base
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [["a"]]

    def test_output_v1_cond(self):
        """
        Test output statements.
        """
        self.parse("""\
            asp 1 0 0
            1 1 2 1 2 0 0
            4 1 a 1 1
            4 1 b 1 2
            4 1 c 2 1 2
            4 1 d 0
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [["a", "b", "c", "d"], ["a", "d"], ["b", "d"], ["d"]]
        sym = Function(self.lib, "d", [])
        assert sym in self.ctl.base.terms
        assert sym not in self.ctl.base

    def test_output_v2_fact(self):
        """
        Test output fact.
        """
        self.parse("""\
            asp 2 0 0
            1 0 1 1 0 0
            4 0 1 1 a
            4 0 1 1 b
            0
            """)
        sym = Function(self.lib, "a", [])
        assert sym not in self.ctl.base.terms
        assert sym in self.ctl.base
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [["a", "b"]]

    def test_output_v2_cond(self):
        """
        Test output fact.
        """
        self.parse("""\
            asp 2 0 0
            1 1 2 1 2 0 0
            4 1 0 1 a
            4 2 0 0
            4 1 1 1 b
            4 2 1 1 1
            4 1 2 1 c
            4 2 2 2 1 2
            0
            """)
        a = Function(self.lib, "a", [])
        b = Function(self.lib, "b", [])
        c = Function(self.lib, "c", [])
        assert a in self.ctl.base.terms
        assert b in self.ctl.base.terms
        assert c in self.ctl.base.terms
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [["a"], ["a"], ["a", "b"], ["a", "b", "c"]]

    def test_output_symbols(self):
        """
        Test output statements.
        """
        self.parse("""\
            asp 2 0 0 symbols
            1 0 1 1 0 0
            1 0 1 2 0 0
            1 0 1 3 0 0
            4 5 0 1 p
            4 4 1 1
            4 7 2 0 0 1 1
            4 0 2 1
            4 4 3 2
            4 7 4 0 0 1 3
            4 0 4 2
            4 4 5 3
            4 7 6 0 0 1 5
            4 0 6 3
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [["p(1)", "p(2)", "p(3)"]]

        self._ctl = Control(self.lib, ["0", "--opt-mode=optN"])
        self.parse("""\
            asp 2 0 0 symbols
            1 0 1 1 0 0
            4 5 0 1 p
            4 3 1 0
            4 3 2 1
            4 5 3 5 a"b
            c
            4 5 4 1 f
            4 4 5 1
            4 7 6 0 4 1 5
            4 6 7 1 5
            4 6 8 0
            4 7 9 0 0 6 1 2 3 6 7 8
            4 0 9 1
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [['p(#inf,#sup,"a\\"b\\nc",f(1),(1,),())']]

    def test_external(self):
        """
        Test external statements.
        """
        self.parse("""\
            asp 1 0 0
            5 1 1
            5 2 2
            5 3 0
            4 1 a 1 1
            4 1 b 1 2
            4 1 c 1 3
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [["a"], ["a", "c"]]

    def test_assume(self):
        """
        Test assumption statements.
        """
        self.parse("""\
            asp 1 0 0
            1 1 3 1 2 3 0 0
            6 2 1 -2
            4 1 a 1 1
            4 1 b 1 2
            4 1 c 1 3
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [["a"], ["a", "c"]]

    def test_heuristic(self):
        """
        Test heuristic statements.
        """
        self._ctl = Control(self.lib, ["--heuristic", "domain"])
        self.parse("""\
            asp 1 0 0
            1 1 2 1 2 0 0
            4 1 a 1 1
            4 1 b 1 2
            7 4 1 1 0 0
            7 5 2 0 0 0
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [["a"]]

    def test_edge(self):
        """
        Test edge statements.
        """
        self.parse("""\
            asp 1 0 0
            1 1 2 1 2 0 0
            4 1 a 1 1
            4 1 b 1 2
            8 0 1 1 2
            8 1 0 1 1
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [[], ["a"], ["b"]]

    def test_theory(self):
        """
        Test adding theory statements.
        """
        # NOTE: the parser currently expects theory atoms to be read in in
        # depth-first order.
        self.parse("""\
            asp 1 0 0
            1 0 1 2 0 0
            1 0 1 3 0 0
            1 0 1 5 0 0
            1 0 1 6 0 0
            1 0 1 7 0 1 1
            1 1 1 1 0 0
            4 1 a 1 1
            9 1 9 1 p
            9 1 1 1 f
            9 0 2 1
            9 0 3 2
            9 1 4 1 +
            9 2 5 4 2 2 3
            9 1 6 1 g
            9 2 7 6 1 5
            9 4 8 2 1 7 1 1
            9 1 10 1 <
            9 0 0 5
            9 6 2 9 1 8 10 0
            9 0 11 3
            9 2 12 -1 3 2 3 11
            9 4 13 1 12 0
            9 5 3 9 1 13
            9 2 14 -3 3 2 3 11
            9 4 15 1 14 0
            9 5 5 9 1 15
            9 2 16 -2 3 2 3 11
            9 4 17 1 16 0
            9 5 6 9 1 17
            9 4 18 1 2 0
            9 5 7 9 1 18
            0
            """)
        assert sorted(str(atom) for atom in self.ctl.base.theory) == [
            "&p { (1,2,3) }",
            "&p { 1 }",
            "&p { [1,2,3] }",
            "&p { f,g((1+2)): <literal: 1> } < 5",
            "&p { {1,2,3} }",
        ]

    def test_comment(self):
        """
        Test aspif comments.
        """
        self.parse("""\
            asp 1 0 0
            10 123
            1 1 1 1 0 0
            10 abc
            4 1 a 1 1
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [[], ["a"]]

    def test_incremental(self):
        """
        Test incremental parsing.
        """
        self.parse("""\
            asp 1 0 0 incremental
            1 1 1 1 0 0
            4 1 a 1 1
            0
            1 1 1 2 0 0
            4 1 b 1 2
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [[], ["a"], ["a", "b"], ["b"]]

    def test_incremental_theory(self):
        """
        Test incremental theory parsing.
        """
        # NOTE: Since the parser does not support interleaved parsing and
        # solving, it simply merges theories.
        self.parse("""\
            asp 1 0 0 incremental
            1 0 1 1 0 0
            9 1 2 1 p
            9 1 0 1 a
            9 4 1 1 0 0
            9 5 1 2 1 1
            0
            1 0 1 3 0 0
            9 1 2 1 p
            9 1 0 1 b
            9 4 1 1 0 0
            9 5 3 2 1 1
            0
            """)
        assert sorted(str(atom) for atom in self.ctl.base.theory) == [
            "&p { a }",
            "&p { b }",
        ]

    def test_incremental_assume(self):
        """
        Test incremental assumptions.
        """
        self.parse("""\
            asp 1 0 0 incremental
            1 1 1 1 0 0
            6 1 1
            4 1 a 1 1
            0
            1 1 1 2 0 0
            6 1 2
            4 1 b 1 2
            0
            """)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [["a", "b"], ["b"]]
