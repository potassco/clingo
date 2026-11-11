"""
Unit tests for the clingo.theory module.
"""

import ctypes
import os

import pytest
from clingo.core import Library
from clingo.symbol import Number, Symbol
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

        idx = assignment.lookup(Number(self.lib, 10))
        assert idx == 0
        assert assignment.lookup(Number(self.lib, 999)) is None

        symbol, value = assignment.at(0)
        assert symbol == Number(self.lib, 10)
        assert value == 7

        symbol, value = assignment.at(1)
        assert symbol == Number(self.lib, 20)
        assert value == pytest.approx(3.5)

        symbol, value = assignment.at(2)
        assert symbol == Number(self.lib, 30)
        assert isinstance(value, Symbol)
        assert value == Number(self.lib, 10)

        with pytest.raises(IndexError):
            assignment.at(3)

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
