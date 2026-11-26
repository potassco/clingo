"""
Unit tests for clingo.core module.
"""

import pytest
from clingo.core import Library, LogLevel, MessageType, version


class TestCore:
    """
    Unit tests for clingo.core module.
    """

    def test_version(self):
        """
        Test the version function.
        """
        assert version() >= (6, 0, 0)

    def test_library(self):
        """
        Test library creation.
        """
        assert Library() is not None

    def test_report(self):
        """
        Test reporting messages via the logger of a library.
        """
        messages: list[tuple[MessageType, str]] = []
        lib = Library(
            log_level=LogLevel.Warn,
            logger=lambda code, msg: messages.append((code, msg)),
        )
        lib.report(MessageType.Warn, "test")
        lib.report(MessageType.Info, "test")
        assert messages == [(MessageType.Warn, "warning: test")]
        with pytest.raises(ValueError):
            lib.report(42, "test")  # type: ignore
