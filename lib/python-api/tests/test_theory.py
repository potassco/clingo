"""
Unit tests for the clingo.theory module.
"""

import ctypes
import os
from collections.abc import Callable
from typing import cast

import pytest
from clingo.app import AppOptions
from clingo.ast import Statement, parse_string
from clingo.control import Control
from clingo.core import Library
from clingo.symbol import Number
from clingo.theory import Theory


def _load_fake_theory():
    """
    Load the fake theory plugin built by CMake.
    """
    path = os.environ.get(
        "PYCLINGO_FAKE_THEORY", "./build/debug/bin/python/fake_theory.so"
    )
    try:
        return ctypes.CDLL(path)
    except OSError:
        pass
    pytest.skip(
        "fake_theory plugin not found (build tests with CLINGO_BUILD_TESTS=ON) "
    )


def _capsule_new(ptr: int, name: bytes):
    """
    Create a Python capsule from a function pointer.
    """
    cap_new = ctypes.pythonapi.PyCapsule_New
    cap_new.restype = ctypes.py_object
    cap_new.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p]
    return cap_new(ctypes.c_void_p(ptr), name, None)


def _create_capsule(lib: ctypes.CDLL, name: bytes = b"clingo_theory_create"):
    """
    Build a theory-create capsule from fake_theory.
    """
    fn = lib.fake_theory_create
    ptr = ctypes.cast(fn, ctypes.c_void_p).value
    assert ptr is not None
    return _capsule_new(ptr, name)


class TestTheory:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for the theory module.
    """

    def setup_method(self, method):
        """
        Create lib.
        """
        assert method is not None
        self._lib = Library()

    def teardown_method(self, method):
        """
        Destroy lib.
        """
        assert method is not None
        self._lib = None

    @property
    def lib(self) -> Library:
        """
        Get the library object.
        """
        assert self._lib is not None
        return self._lib

    def test_construct_invalid_capsule_name(self):
        """
        Theory constructor validates capsule name.
        """
        plugin = _load_fake_theory()
        cap = _create_capsule(plugin, b"wrong_name")
        with pytest.raises(ValueError):
            Theory(self.lib, cap)

    def test_info_and_assignment(self):
        """
        Test info access and assignment API.
        """
        plugin = _load_fake_theory()
        cap = _create_capsule(plugin)
        theory = Theory(self.lib, cap)

        assert theory.version == (1, 2, 3)
        assert theory.name == "fake-theory"
        assert theory.commit_hash == "deadbeef"
        assert theory.has_assignment

        assignment = theory.assignment(0)

        symbol = Number(self.lib, 10)
        assert assignment.get(symbol) == 0
        assert assignment.get(Number(self.lib, 999)) is None

        assert assignment[0] == assignment[symbol] == 7
        assert assignment[1] == pytest.approx(3.5)
        assert assignment[2] == Number(self.lib, 10)

        with pytest.raises(IndexError):
            assignment[3]

        with pytest.raises(IndexError):
            assignment[Number(self.lib, 999)]

        values = list(assignment)
        assert len(values) == 3
        assert values[0] == (Number(self.lib, 10), 7)
        assert values[1][0] == Number(self.lib, 20)
        assert values[1][1] == pytest.approx(3.5)
        assert values[2][0] == Number(self.lib, 30)
        assert values[2][1] == Number(self.lib, 10)

    def test_value(self):
        """
        Test value(thread_id, symbol).
        """
        plugin = _load_fake_theory()
        cap = _create_capsule(plugin)
        theory = Theory(self.lib, cap)

        assert theory.value(0, Number(self.lib, 10)) == 7
        assert theory.value(0, Number(self.lib, 20)) == pytest.approx(3.5)
        assert theory.value(0, Number(self.lib, 30)) == Number(self.lib, 10)
        assert theory.value(0, Number(self.lib, 999)) is None

    def test_api(self, tmp_path):
        """
        Exercise all callable methods.
        """
        plugin = _load_fake_theory()
        cap = _create_capsule(plugin)
        theory = Theory(self.lib, cap)

        control = Control(self.lib)

        assert theory.register(control) is None
        with pytest.raises(TypeError):
            theory.register_options(cast(AppOptions, None))

        assert theory.validate_options() is None
        assert theory.prepare(control) is None

        statements = []
        parse_string(self.lib, "a.", statements.append)
        assert statements

        rewritten = []
        assert theory.rewrite(statements[0], rewritten.append) is None
        assert rewritten

        with pytest.raises(TypeError):
            theory.rewrite(statements[0], cast(Callable[[Statement], None], None))

        assert theory.rewrite_string(self.lib, control, "b.") is None

        path = tmp_path / "rewrite.lp"
        path.write_text("c.\n", encoding="utf-8")
        assert theory.rewrite_files(self.lib, control, [str(path)]) is None

        control.parse_string("d.")
        control.ground([("base", [])])

        seen_model = False
        seen_stats = False

        def _on_model(model):
            nonlocal seen_model
            seen_model = True
            assert theory.on_model(model) is None

        def _on_stats(step, accu):
            nonlocal seen_stats
            seen_stats = True
            assert theory.on_stats(step, accu) is None

        control.solve(on_model=_on_model, on_stats=_on_stats)
        assert seen_model
        assert seen_stats
