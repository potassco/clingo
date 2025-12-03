"""
Temporal Mode Application for clingo.

This module provides a temporal mode transformation for logic programs using
clingo's Python API. It rewrites temporal logic programs by adding a time
parameter to all symbolic atoms and program directives, enabling incremental
solving. The transformation also adapts program sections (e.g., 'final',
'initial', 'dynamic', 'static', 'base') to their incremental counterparts and
injects temporal control atoms as needed.

Usage:
    python tmode.py <input-files>

Classes:
    Transformer: Rewrites temporal logic programs for incremental solving.
    TModeApp:    Command-line application for temporal mode transformation.

Example:
    $ python tmode.py example.lp
"""

import sys
from collections.abc import Sequence
from functools import singledispatchmethod
from typing import Any

from clingo import ast
from clingo.app import App, AppOptions, clingo_main
from clingo.control import Control
from clingo.core import Library, Location
from clingo.symbol import Function, Number, Symbol, SymbolType

Term = ast.TermSymbolic | ast.TermBinaryOperation


class Transformer:
    """
    Transformer to convert temporal logic programs into incremental ones.

    Adds a time parameter to all symbolic atoms and program directives.
    Converts 'final' programs into 'static' ones with an additional
    'finally(T)' atom in their body.

    Args:
        lib: clingo AST library instance.

    Usage:
        transformer = Transformer(lib)
        new_program = transformer(["input.lp"])
    """

    _lib: Library
    _param: Symbol
    _current: str

    def __init__(self, lib):
        self._lib = lib
        self._param = Function(self._lib, "__t")
        self._final = False
        self._current = "initial"

    def _get_param(self, name: str, location: Location) -> tuple[str, Term]:
        """
        Extract the time parameter from the given name.

        Args:
            name: The name of the atom or directive.
            location: The AST location object.

        Returns:
            A tuple of form (base name, time parameter term).
        """
        n = name.replace("'", "")
        primes = len(name) - len(n)
        if self._current == "base":
            param: Term = ast.TermSymbolic(
                self._lib, location, Number(self._lib, -primes)
            )
        else:
            param = ast.TermSymbolic(self._lib, location, self._param)
            if primes > 0:
                param = ast.TermBinaryOperation(
                    self._lib,
                    location,
                    param,
                    ast.BinaryOperator.Minus,
                    ast.TermSymbolic(self._lib, location, Number(self._lib, primes)),
                )
        return n, param

    @singledispatchmethod
    def _rewrite_term(self, expr: Any) -> Any:
        """
        Recursively rewrite terms by adding the time parameter.

        Args:
            expr: AST term expression.

        Returns:
            AST term with time parameter added.
        """
        return expr.transform(self._lib, self._rewrite_term)

    @_rewrite_term.register
    def _(self, expr: ast.TermFunction) -> Any:
        name, param = self._get_param(expr.name, expr.location)
        pool = []
        for args in expr.pool:
            x = list(args.arguments)
            x.append(param)
            pool.append(args.update(self._lib, arguments=x))
        return expr.update(self._lib, name=name, pool=pool)

    @_rewrite_term.register
    def _(self, expr: ast.TermSymbolic) -> Any:
        if expr.symbol.type == SymbolType.Function:
            name, param = self._get_param(expr.symbol.name, expr.location)
            args: list[Term] = []
            for arg in expr.symbol.arguments:
                args.append(ast.TermSymbolic(self._lib, expr.location, arg))
            args.append(param)
            pool = [ast.ArgumentTuple(self._lib, args)]
            sym = ast.TermFunction(self._lib, expr.location, name, pool)
            return sym
        raise RuntimeError("not implemented")

    @singledispatchmethod
    def _rewrite_stm(self, expr: Any) -> Any:
        """
        Rewrite statements by adding the time parameter.

        If the program section is 'final', converts it to 'static' and adds a
        'finally(T)' atom to the body.

        Args:
            expr: AST statement.

        Returns:
            Rewritten AST statement.
        """
        ret = expr.transform(self._lib, self._rewrite_stm) or expr
        if self._final and hasattr(ret, "body"):
            loc = ret.location
            sym = ast.TermSymbolic(self._lib, loc, self._param)
            arg = ast.ArgumentTuple(self._lib, [sym])
            fun = ast.TermFunction(self._lib, loc, "finally", [arg], False)
            lit = ast.LiteralSymbolic(self._lib, loc, ast.Sign.NoSign, fun)
            bdy = list(ret.body)
            bdy.append(ast.BodySimpleLiteral(self._lib, lit))
            ret = ret.update(self._lib, body=bdy)
        return ret

    @_rewrite_stm.register
    def _(self, expr: ast.LiteralSymbolic) -> Any:
        return expr.update(self._lib, atom=self._rewrite_term(expr.atom))

    @_rewrite_stm.register
    def _(self, expr: ast.StatementProgram) -> Any:
        args = list(expr.arguments)
        name = expr.name
        self._final = False
        match expr.name:
            case "final":
                name = "check"
                self._final = True
            case "initial":
                name = "base"
            case "dynamic":
                name = "step"
            case "static":
                name = "check"
            case "base":
                name = "check"
        self._current = name
        if name != "base":
            args.append(self._param.name)

        return expr.update(self._lib, name=name, arguments=args)

    @_rewrite_stm.register
    def _(self, expr: ast.StatementShowSignature) -> Any:
        return expr.update(self._lib, arity=expr.arity + 1)

    @_rewrite_stm.register
    def _(self, expr: ast.StatementProjectSignature) -> Any:
        return expr.update(self._lib, arity=expr.arity + 1)

    def __call__(self, files: Sequence[str]) -> ast.Program:
        """
        Rewrite the given files and return the transformed AST program.

        Args:
            files: List of input file paths.

        Returns:
            The transformed program.
        """
        prg = ast.Program(self._lib)
        ast.parse_files(
            self._lib, files, lambda stm: prg.add(self._rewrite_stm(stm) or stm)
        )
        return prg


class TModeApp(App):
    """
    Temporal mode application.
    """

    _lib: Library

    def __init__(self, lib: Library):
        super().__init__("tmode", "1.0.0")
        self._lib = lib

    def register_options(self, options: AppOptions) -> None:
        """
        Command-line application for temporal mode transformation.

        Integrates the Transformer into a clingo.App for use with clingo_main.
        """
        # TODO:
        # - step -> state
        # - hide initially and query
        options.set_default_value("out-pred-sep", "\n")
        options.set_default_value("out-step", "last")
        options.set_default_value("iquery", "finally")

    def main(self, control: Control, files: Sequence[str]) -> None:
        """
        Run the temporal mode application.

        Args:
            control: clingo control object.
            files: List of input file paths.
        """
        control.join(Transformer(self._lib)(files))
        control.parse_string("#include <incmode>. #program base. initially(0).")
        control.main()


if __name__ == "__main__":
    LIB = Library()
    sys.exit(clingo_main(LIB, sys.argv[1:], TModeApp(LIB)))
