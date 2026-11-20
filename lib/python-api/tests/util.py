"""
Utilities for testing.
"""

import sys

import pytest
from clingo.solve import Model
from clingo.symbol import Symbol

# WebAssembly platforms like Pyodide support neither threads nor processes.
WASM = sys.platform in ("emscripten", "wasi")

needs_threads = pytest.mark.skipif(WASM, reason="threads are not supported")
needs_processes = pytest.mark.skipif(WASM, reason="processes are not supported")


class MCB:
    """
    Helper to intercept symbols while solving.
    """

    _syms: list[list[Symbol]]

    def __init__(self) -> None:
        self._syms = []
        self.cleared = False

    def __call__(self, mdl: Model):
        if not self.cleared and mdl.optimality_proven:
            self.cleared = True
            self._syms.clear()
        self._syms.append(sorted(mdl.symbols(shown=True)))

    @property
    def symbols(self) -> list[list[str]]:
        """
        Get the collected symbols.
        """
        return [[str(sym) for sym in syms] for syms in sorted(self._syms)]
