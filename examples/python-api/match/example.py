"""
Symbol pattern matching for logic programming.

This module provides functions for matching symbols against structured
patterns. It includes a parser for transforming textual patterns into matchers,
which can then be used to identify variable assignments in symbolic terms. The
matchers support exact symbol matching, variable binding, and function
structure recognition.
"""

import re
from abc import abstractmethod
from dataclasses import dataclass
from functools import lru_cache
from typing import Iterator, Sequence

from clingo.core import Library
from clingo.symbol import Infimum, Number, Supremum, Symbol, parse_term


@dataclass
class Match:
    """
    A simple container for a successful match result.

    Attributes:
        assignment: A mapping from variable names to their matched Symbol.
    """

    assignment: dict[str, Symbol]


class Matcher:
    """
    Abstract base class for matchers.

    A matcher encapsulates the logic required to match a Symbol against a pattern.
    Subclasses must implement the match method.
    """

    @abstractmethod
    def match(self, symbol: Symbol, assignment: dict[str, Symbol]) -> bool:
        """
        Attempt to match the given symbol and update the assignment if successful.

        Args:
            symbol: The symbol to be matched.
            assignment: A dictionary to record variable assignments.

        Returns:
            True if the match succeeds; False otherwise.
        """

    def __call__(self, symbol: Symbol) -> Match | None:
        """
        Make Matcher callable. Attempts to match the given symbol.

        If the match is successful, returns a Match object containing the assignment.
        Otherwise, returns None.

        Args:
            symbol: The symbol to match.

        Returns:
            The match result.
        """
        assignment = {}
        if self.match(symbol, assignment):
            return Match(assignment)
        return None


@dataclass
class FunctionMatcher(Matcher):
    """
    Matcher for functions and tuples.

    Matches a function symbol with a specific name, a sequence of argument matchers,
    and a polarity (positive or negative).

    Also handles tuples which must be positive with empty name.

    Attributes:
        name: The name of the function to match.
        arguments: Matchers for each argument of the function.
        positive: True if matching a positive function; False for negated function.
    """

    name: str
    arguments: Sequence[Matcher]
    positive: bool

    def match(self, symbol: Symbol, assignment: dict[str, Symbol]) -> bool:
        """
        Check if the given symbol is a function or tuple with the expected
        name, arguments, and polarity.

        For tuples only the arguments are checked.

        Args:
            symbol: The function symbol to match.
            assignment: Dictionary holding current variable assignments.

        Returns:
            True if the function symbol matches; False otherwise.
        """
        if not self.name:
            return symbol.match(len(self.arguments)) and all(
                m.match(arg, assignment)
                for m, arg in zip(self.arguments, symbol.arguments)
            )
        return symbol.match(self.name, len(self.arguments), self.positive) and all(
            m.match(arg, assignment) for m, arg in zip(self.arguments, symbol.arguments)
        )


@dataclass
class ValueMatcher(Matcher):
    """
    Matcher for concrete values.

    Attributes:
        value: The value to match.
    """

    value: str | int | Symbol

    def match(self, symbol: Symbol, assignment: dict[str, Symbol]) -> bool:
        """
        Check if the provided symbol matches the stored value.

        Args:
            symbol: The symbol to match.
            assignment: Current variable assignment (unused here).

        Returns:
            True if the symbols are equal.
        """
        assert assignment is not None
        if isinstance(self.value, Symbol):
            return symbol == self.value
        if isinstance(self.value, str):
            return symbol.string == self.value
        assert isinstance(self.value, int)
        return symbol.number == self.value


@dataclass
class VariableMatcher(Matcher):
    """
    Matcher for variables. It binds a symbol to a variable name in the assignment.

    Attributes:
        name: The variable name.
    """

    name: str

    def match(self, symbol: Symbol, assignment: dict[str, Symbol]) -> bool:
        """
        Attempt to match a variable. If the variable is already bound in the assignment,
        then the symbol must be equal to the bound symbol. Otherwise, bind the variable.

        Anonymous variables with name "_" are handled specially, they do not
        interact with the assignment and always match.

        Args:
            symbol: The symbol to bind or compare.
            assignment: The variable assignment mapping.

        Returns:
            bool: True if the variable matches or is successfully bound.
        """
        if self.name != "_":
            if self.name in assignment:
                return assignment[self.name] == symbol
            assignment[self.name] = symbol
        return True


@dataclass(frozen=True)
class _Token:
    """
    Represents a parsed token with token type and value.
    """

    token: str
    value: str


class _Tokenizer:
    """
    Tokenizes a given expressions.

    Also provides convenience functions to help parsing.
    """

    def __init__(self, expression: str, patterns: dict[str, str]):
        """
        Initialize the tokenizer with an expression string.

        Args:
            expression: The matcher expression to tokenize.
        """
        regex = re.compile("|".join(f"(?P<{t}>{p})" for t, p in patterns.items()))
        self._tokens = _Tokenizer._tokenize(expression, regex)
        self._token = next(self._tokens)

    @staticmethod
    def _tokenize(expression: str, regex: re.Pattern) -> Iterator[_Token]:
        """
        Tokenize the input expression using the given regular.

        Args:
            expression: The expression to tokenize.

        Yields:
            Tokens capturing token type and value.
        """
        for m in re.finditer(regex, expression):
            token = m.lastgroup
            assert token is not None
            value = m.group(token)
            yield _Token(token, value)
        yield _Token("EOF", "")

    def consume(self) -> _Token:
        """
        Advance to the next token in the input and return the current one.

        Returns:
            The current token.
        """
        token = self._token
        # do not consume the end token
        if self._token.token != "EOF":
            self._token = next(self._tokens)
        return token

    def peek(self, expected_token: str, expected_value: str | None = None) -> bool:
        """
        Check if the current token matches the expected token type and (optionally) value.

        Args:
            expected_token: The expected token type.
            expected_value: The expected token value, if any.

        Returns:
            True if current token matches expectations.
        """
        return self._token.token == expected_token and (
            expected_value is None or self._token.value == expected_value
        )

    def expect(self, expected_token: str, expected_value: str | None = None) -> _Token:
        """
        Assert that the current token matches the expected type and value and consume it.

        Raises:
            SyntaxError: If the current token does not match.

        Returns:
            The current token.
        """
        if token := self.match(expected_token, expected_value):
            return token
        raise SyntaxError(f"Unexpected token: {self._token}")

    def match(
        self, expected_token: str, expected_value: str | None = None
    ) -> _Token | None:
        """
        If the current token matches the expected type and value, consume it.

        Args:
            expected_token: The token type to match.
            expected_value: The token value to match, if applicable.

        Returns:
            The current token.
        """
        if self.peek(expected_token, expected_value):
            return self.consume()
        return None


def unquote(quoted: str) -> str:
    """
    Unquote the given string as clingo would.

    Args:
        quoted: The quoted string.

    Returns:
        The unquoted string.
    """
    result = []
    slash = False
    for c in quoted[1:-1]:
        if slash:
            if c == "n":
                result.append("\n")
            elif c == "\\":
                result.append("\\")
            elif c == '"':
                result.append('"')
            else:
                raise ValueError(f"invalid escape sequence: \\{c}")
            slash = False
        elif c == "\\":
            slash = True
        else:
            result.append(c)
    return "".join(result)


class _Parser:
    """
    A recursive descent parser that tokenizes and parses matcher expressions.

    The parser converts a string representation of a matcher (such as 'f(X,10)')
    into a corresponding Matcher object.
    """

    TOKEN_PATTERNS = {
        "NEG": r"-",
        "SUP": "#sup",
        "INF": "#inf",
        "STR": r'"([^\\"\n\000]|\\"|\\\\|\\n)*"',
        "NUM": r"\d+",
        "VAR": r"_|[A-Z][a-zA-Z_']*",
        "IDF": r"[_']*[a-z]['A-Za-z0-9_]*",
        "PUN": r"[(),]",
    }

    def __init__(self, expression: str):
        """
        Initialize the parser with the given expression.

        Args:
            expression: The expression to parse.
        """
        self._tokenizer = _Tokenizer(expression, _Parser.TOKEN_PATTERNS)

    def _parse_matcher(self) -> Matcher:
        """
        Parse a matcher expression and return the corresponding Matcher object.

        This function handles numbers, strings, variables, negations, and functions.

        Returns:
            The parsed matcher object.
        """
        if token := self._tokenizer.match("SUP"):
            return ValueMatcher(Supremum)
        if token := self._tokenizer.match("INF"):
            return ValueMatcher(Infimum)
        if token := self._tokenizer.match("NUM"):
            return ValueMatcher(int(token.value))
        if token := self._tokenizer.match("STR"):
            return ValueMatcher(unquote(token.value))
        if token := self._tokenizer.match("VAR"):
            return VariableMatcher(token.value)
        if self._tokenizer.peek("PUN", "("):
            return self._parse_function("", True)
        if self._tokenizer.match("NEG"):
            if token := self._tokenizer.match("IDF"):
                return self._parse_function(token.value, False)
            token = self._tokenizer.expect("NUM")
            return ValueMatcher(-int(token.value))
        token = self._tokenizer.expect("IDF")
        return self._parse_function(token.value, True)

    def _parse_function(self, name: str, positive: bool) -> Matcher:
        """
        Parse a function or tuple matcher from the current position.

        A function may have arguments inside parentheses. If no parentheses are found,
        the function is treated as having no arguments.

        Also supports tuples for which the name must be empty and the polarity
        true. Tuples can have a trailing comma in their argument list.

        Args:
            name: The name of the function.
            positive: The polarity of the function (True for positive, False for negated).

        Returns:
            The constructed function matcher.
        """
        if not self._tokenizer.match("PUN", "("):
            return FunctionMatcher(name, [], positive)
        args = []
        trail = bool(name)

        if not trail and self._tokenizer.match("PUN", ","):
            self._tokenizer.expect("PUN", ")")
            trail = True
        else:
            while not self._tokenizer.match("PUN", ")"):
                if args:
                    self._tokenizer.expect("PUN", ",")
                    if not trail and self._tokenizer.match("PUN", ")"):
                        trail = True
                        break
                args.append(self._parse_matcher())

        if not trail and len(args) == 1:
            return args[0]
        return FunctionMatcher(name, args, positive)

    def parse(self) -> Matcher:
        """
        Parse the complete expression and return the Matcher.

        Ensures that the entire input has been consumed.

        Returns:
            The final matcher constructed from the input expression.
        """
        result = self._parse_matcher()
        self._tokenizer.expect("EOF")
        return result


@lru_cache
def compile_matcher(expression: str) -> Matcher:
    """
    Compile an expression string into a Matcher object.

    Args:
        expression: The matcher expression to compile.

    Returns:
        The compiled matcher.
    """
    return _Parser(expression).parse()


def match(expression: str, symbol: Symbol) -> Match | None:
    """
    Convenience function to compile an expression and match it against a symbol.

    Args:
        expression: The matcher expression.
        symbol: The symbol to match against.

    Returns:
        A Match object with the variable assignment if the symbol matches,
        or None if it does not.
    """
    return compile_matcher(expression)(symbol)


def main():
    """
    Some tests for exposition.
    """
    with Library() as lib:
        m = match("(_,_)", parse_term(lib, "(1,2)"))
        assert m and not m.assignment
        m = match("(#sup,#inf)", parse_term(lib, "(#sup,#inf)"))
        assert m
        m = match("f(X,(a))", parse_term(lib, "f(1,(a))"))
        assert m and m.assignment == {"X": Number(lib, 1)}
        m = match("f(X,(a,))", parse_term(lib, "f(1,(a,))"))
        assert m and m.assignment == {"X": Number(lib, 1)}
        m = match("f(X,(a,b))", parse_term(lib, "f(1,(a,b))"))
        assert m and m.assignment == {"X": Number(lib, 1)}
        m = match("f(X,(a,b,))", parse_term(lib, "f(1,(a,b))"))
        assert m and m.assignment == {"X": Number(lib, 1)}
        m = match("f(X,(,))", parse_term(lib, "f(1,())"))
        assert m and m.assignment == {"X": Number(lib, 1)}
        m = match("f(X,())", parse_term(lib, "f(1,())"))
        assert m and m.assignment == {"X": Number(lib, 1)}
        m = match(r'f(X,"a\n\"bc\\")', parse_term(lib, r'f(1,"a\n\"bc\\")'))
        assert m and m.assignment == {"X": Number(lib, 1)}
        m = match("f(X,10)", parse_term(lib, "f(1,10)"))
        assert m and m.assignment == {"X": Number(lib, 1)}
        assert match("f(X,10)", parse_term(lib, "f(1,11)")) is None
        m = match("f(-g(X,-5,X,Y),10)", parse_term(lib, "f(-g(1,-5,1,2),10)"))
        assert m and m.assignment == {"X": Number(lib, 1), "Y": Number(lib, 2)}
        assert (
            match("f(-g(X,-5,X,Y),10)", parse_term(lib, "f(-g(1,-5,2,2),10)")) is None
        )


if __name__ == "__main__":
    main()
