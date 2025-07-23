"""
Print programs in a more predictable format.

The idea is to be able to apply diffs to programs with small differences.
"""

import sys
from functools import singledispatchmethod

from clingo import ast
from clingo.core import Library


class Order:
    """
    Class print programs in a more predictable format.

    TODO:
    - Elements of aggregates and minimize constraints shoud be ordered as well.
    """

    def __init__(self):
        self._lib = Library()

    @singledispatchmethod
    def _dispatch(self, expr):
        """
        Order bodies of statements.
        """
        return expr.transform(self._lib, self._dispatch)

    @_dispatch.register
    def _(self, stm: ast.StatementRule) -> ast.StatementRule:
        """
        Order bodies of rules.
        """
        return stm.update(self._lib, body=sorted(stm.body))

    def order(self, files: list[str]) -> None:
        """
        Parse the statements in the given files and output them in an orderly fashion.
        """
        stms = []
        ast.parse_files(
            self._lib, files, lambda stm: stms.append(self._dispatch(stm) or stm)
        )
        for stm in sorted(stms):
            print(str(stm))


if __name__ == "__main__":
    Order().order(sys.argv[1:])
