"""
Unit tests for the aspif parser.
"""

import os
import tempfile
from contextlib import contextmanager

from clingo.control import Control
from clingo.core import Library
from util import MCB


@contextmanager
def make_file(content: str | None = None):
    """
    Context manager that creates a temporary file with initial content.

    Ensures proper cleanup even if exceptions occur during file operations.
    """
    temp_name = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w", delete=False, encoding="utf-8"
        ) as temp_file:
            if content is not None:
                temp_file.write(content)
            temp_name = temp_file.name
        yield temp_name

    finally:
        try:
            if temp_name is not None and os.path.exists(temp_name):
                os.remove(temp_name)
        except (OSError, PermissionError):
            pass


class TestWriteAspif:
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

    @contextmanager
    def parse(self, content: str, symbols: bool = False):
        """
        Parse the given program and return a file with its aspif grounding.
        """
        ctl = Control(self.lib)
        ctl.parse_string(content)
        ctl.ground()
        with make_file() as path:
            ctl.write_aspif(path, symbols=symbols)
            yield path

    def parse_string(self, content: str, symbols: bool = False):
        """
        Parse the given program and return a string with its aspif grounding.
        """
        ctl = Control(self.lib)
        ctl.parse_string(content)
        ctl.ground()
        ctl.write_aspif(symbols=symbols)
        return ctl.buffer

    def test_buffer(self):
        """
        Test writing aspif to a buffer and parsing it again.
        """
        prg = self.parse_string("a. {b}. c :- b.")
        self.ctl.parse_string(prg)
        mcb = MCB()
        self.ctl.solve(on_model=mcb)
        assert mcb.symbols == [["a"], ["a", "b", "c"]]

    def test_rule(self):
        """
        Test rules.
        """
        with self.parse("a. {b}. c :- b.") as path:
            self.ctl.parse_files([path])
            mcb = MCB()
            self.ctl.solve(on_model=mcb)
            assert mcb.symbols == [["a"], ["a", "b", "c"]]

    def test_aggregate(self):
        """
        Test aggregates.
        """
        self._ctl = Control(self.lib, ["0", "--trans-ext", "no"])
        with self.parse("{a;b;c}. :- 2 {a;b;c} 2.") as path:
            self.ctl.parse_files([path])
            mcb = MCB()
            self.ctl.solve(on_model=mcb)
            assert mcb.symbols == [[], ["a"], ["a", "b", "c"], ["b"], ["c"]]

    def test_disjunction(self):
        """
        Test disjunction.
        """
        with self.parse("a | b | c. a :- b. b :- a.") as path:
            self.ctl.parse_files([path])
            mcb = MCB()
            self.ctl.solve(on_model=mcb)
            assert mcb.symbols == [["a", "b"], ["c"]]

    def test_minimize(self):
        """
        Test minimize.
        """
        with self.parse(
            "#minimize { 1:a; 2:b; 3:c }. 1 {a; b; c}. :- a, not b, not c. :- b, not a, not c."
        ) as path:
            self.ctl.parse_files([path])
            mcb = MCB()
            self.ctl.solve(on_model=mcb)
            assert mcb.symbols == [["a", "b"], ["c"]]

    def test_project(self):
        """
        Test minimize.
        """
        self._ctl = Control(self.lib, ["0", "--project"])
        with self.parse("1 {a; b; c}. #show a/0. #project a/0. #project b/0.") as path:
            self.ctl.parse_files([path])
            mcb = MCB()
            self.ctl.solve(on_model=mcb)
            assert mcb.symbols == [[], [], ["a"], ["a"]]

    def test_output(self):
        """
        Test output.
        """
        for symbols in (True, False):
            with self.parse(
                "1 {a; x}. #show a/0. #show b : x. #show c : a, x.", symbols=symbols
            ) as path:
                self._ctl = Control(self.lib, ["0", "--opt-mode=optN"])
                self.ctl.parse_files([path])
                mcb = MCB()
                self.ctl.solve(on_model=mcb)
                assert mcb.symbols == [["a"], ["a", "b", "c"], ["b"]]

    def test_external(self):
        """
        Test external.
        """
        self._ctl = Control(self.lib, ["0"])
        with self.parse(
            "#external a. [true] #external b. [false] #external c. [free]"
        ) as path:
            self.ctl.parse_files([path])
            mcb = MCB()
            self.ctl.solve(on_model=mcb)
            assert mcb.symbols == [["a"], ["a", "c"]]

    def test_heuristic(self):
        """
        Test heuristic statements.
        """
        self._ctl = Control(self.lib, ["--heuristic", "domain"])
        with self.parse("""\
            {a; b}.
            #heuristic a. [1,true]
            #heuristic b. [0,false]
            """) as path:
            self.ctl.parse_files([path])
            mcb = MCB()
            self.ctl.solve(on_model=mcb)
            assert mcb.symbols == [["a"]]

    def test_edge(self):
        """
        Test edge statements.
        """
        with self.parse("""\
            {a; b}.
            #edge (a,b) : a.
            #edge (b,a) : b.
            """) as path:
            self.ctl.parse_files([path])
            mcb = MCB()
            self.ctl.solve(on_model=mcb)
            assert mcb.symbols == [[], ["a"], ["b"]]

    def test_theory(self):
        """
        Test theory statements.
        """
        with self.parse("""\
            #theory p {
            p { + : 0, binary, left };
            &p/0: p, {<}, p, any
            }.
            &p{ 1,f(1+2),[1],{2},(3,),(4) }.
            &p{} < 2.
            """) as path:
            self.ctl.parse_files([path])
            assert sorted(str(atom) for atom in self.ctl.base.theory) == [
                "&p { 1,f((1+2)),[1],{2},(3),4 }",
                "&p { } < 2",
            ]
