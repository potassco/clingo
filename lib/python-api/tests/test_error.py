"""
Unit tests for error propagation across various modules.
"""

import gc

import pytest
from clingo.control import Control
from clingo.core import Library


class TestError:
    # pylint: disable=attribute-defined-outside-init
    """
    Unit tests for error propagation.
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
        # Note: ensure that the held context object is collected
        gc.collect()
        self._lib = None

    @property
    def lib(self) -> Library:
        """
        Get the library object.
        """
        assert self._lib is not None
        return self._lib

    def fun(self, num):
        """
        Test function that throws an error.
        """
        raise RuntimeError(f"fun called with {num}")

    def test_error_context(self):
        """
        Test errors in grounding callbacks.
        """

        ctl = Control(self.lib)
        ctl.parse_string("p(@fun(1)). q.")
        with pytest.raises(RuntimeError) as exc_info:
            ctl.ground(context=self)
        assert str(exc_info.value) == "fun called with 1"
