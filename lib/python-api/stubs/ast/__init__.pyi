"""
This module provides functions to work with Abstract Syntax Trees of logic programs.

# Examples

The following example shows how to parse individual statements and add them to a control object.

```python
>>> from clingo.core import Library
>>> from clingo.control import Control
>>> from clingo import ast
>>>
>>> lib = Library()
>>> ctl = Control(lib, ["--convert=text"])
>>> ctl.parse_string("a(1).")
>>> prg = ast.Program(lib)
>>> prg.add(ast.parse_statement(lib, "b(X+1) :- a(X)."))
>>> prg.add(ast.parse_statement(lib, "c(X+1) :- b(X)."))
>>> ctl.join(prg)
>>> ctl.ground()
>>> ctl.buffer
\"\"\"
a(1).
b(2).
c(3).
#show.
#show a/1.
#show b/1.
#show c/1.
\"\"\"
```

The next example shows how to visit an AST and collect variables.

```python
from functools import singledispatch

from clingo.core import Library
from clingo import ast
from clingo.ast import terms, theory

@singledispatch
def collect(stm, vars):
    \"\"\"
    Collect all variables occurring in statements.
    \"\"\"
    return stm.visit(collect, vars)

@collect.register
def _(var: terms.TermVariable | theory.TheoryTermVariable, vars):
    vars.add(var.name)

lib = Library()
vars = set()

ast.parse_string(
    lib,
    \"\"\"\\
    a(X) :- b(X,Y).
    b(Z).
    \"\"\",
    lambda stm: collect(stm, vars),
)

print(vars)
```

The last example shows how to use a transformer to modify an AST.

```python
from functools import singledispatch

from clingo.core import Library
from clingo import ast
from clingo.ast import terms

@singledispatch
def rename(stm, lib):
    \"\"\"
    Replaces all occurrences of variable `X` with `Y` in the given statement
    returning a new statement. If no replacement was made, `None` is returned.
    \"\"\"
    return stm.transform(lib, rename, lib)

@rename.register
def _(var: terms.TermVariable, lib):
    return var.update(lib, name="Y") if var.name == "X" else None

lib = Library()
stms = []

ast.parse_string(
    lib,
    \"\"\"\\
    b(1,1).
    b(2,3).
    a(X) :- b(X,Y).
    \"\"\",
    stms.append,
)

for stm in stms:
    print(rename(stm, lib) or stm)
```
"""

from collections.abc import Callable, Sequence
from enum import IntEnum
from typing import Self

from clingo.ast.body import BodyLiteral
from clingo.ast.head import HeadLiteral
from clingo.ast.literals import Literal
from clingo.ast.statements import Statement, StatementTheory
from clingo.ast.terms import Term
from clingo.ast.theory import TheoryTerm
from clingo.control import Control
from clingo.core import Library

__all__: list[str] = [
    "AST",
    "Program",
    "ProjectionMode",
    "RewriteContext",
    "parse_body_literal",
    "parse_files",
    "parse_head_literal",
    "parse_literal",
    "parse_statement",
    "parse_string",
    "parse_term",
    "parse_theory_term",
    "rewrite_statement",
]

class AST:
    """
    The base class of all AST nodes.

    AST nodes are immutable. They can be compared, ordered, hashed, and
    converted to strings in clingo's input language. The node types are defined
    in the submodules of this module. Their arguments are available via
    read-only properties and `__match_args__` to support pattern matching.
    Modified copies of nodes can be created with `update` and `transform`.
    """

    def __eq__(self, other: object, /) -> bool:
        """
        Compare with another object for equality.

        Args:
            other: The object to compare with.

        Returns:
            Whether the two objects are equal.
        """

    def __ne__(self, other: object, /) -> bool:
        """
        Compare with another object for inequality.

        Args:
            other: The object to compare with.

        Returns:
            Whether the two objects are not equal.
        """

    def __lt__(self, other: AST, /) -> bool:
        """
        Compare with another node for less than.

        Args:
            other: The node to compare with.

        Returns:
            Whether this node is less than the other node.
        """

    def __le__(self, other: AST, /) -> bool:
        """
        Compare with another node for less than or equal to.

        Args:
            other: The node to compare with.

        Returns:
            Whether this node is less than or equal to the other node.
        """

    def __gt__(self, other: AST, /) -> bool:
        """
        Compare with another node for greater than.

        Args:
            other: The node to compare with.

        Returns:
            Whether this node is greater than the other node.
        """

    def __ge__(self, other: AST, /) -> bool:
        """
        Compare with another node for greater than or equal to.

        Args:
            other: The node to compare with.

        Returns:
            Whether this node is greater than or equal to the other node.
        """

    def __hash__(self) -> int:
        """
        Compute a hash value for the node.

        Returns:
            The hash value.
        """

    def visit(
        self, visitor: Callable[..., object], *args: object, **kwargs: object
    ) -> None:
        """
        Call the visitor with each child node.

        Child nodes are the arguments of the node that hold nodes. Elements of
        sequences are visited one by one and absent optional nodes are skipped.

        Args:
            visitor: The function to call with each child node followed by the
                additional arguments.
            args: Additional positional arguments passed to the visitor.
            kwargs: Additional keyword arguments passed to the visitor.
        """

    def transform(
        self,
        lib: Library,
        transformer: Callable[..., AST | None],
        *args: object,
        **kwargs: object,
    ) -> Self | None:
        """
        Transform the child nodes of the node.

        The transformer is called with each child node (see `visit`). If it
        returns a node, the child is replaced by it; if it returns `None`, the
        child is kept.

        Args:
            lib: The library object for storing symbols.
            transformer: The function to call with each child node followed by
                the additional arguments.
            args: Additional positional arguments passed to the transformer.
            kwargs: Additional keyword arguments passed to the transformer.

        Returns:
            A new node with the replaced children or `None` if no child was
            replaced.
        """

class ProjectionMode(IntEnum):
    """
    Available projection modes.
    """

    Disabled = 0
    """
    Disable projection.
    """
    Anonymous = 1
    """
    Only project anonymous variables.
    """
    Pure = 2
    """
    Project pure variables.
    """

class Program:
    """
    A non-ground program.

    The program keeps its library alive.
    """

    def __init__(self, lib: Library) -> None:
        """
        Create an empty non-ground program.

        Args:
            lib: A library object to store symbols.
        """

    def add(self, statement: Statement) -> None:
        """
        Add a statement to the program.

        Args:
            statement: The statement to add.
        """

class RewriteContext:
    """
    Context to rewrite statements.

    The context keeps its library alive.
    """

    def __init__(self, lib: Library) -> None:
        """
        Create a context to rewrite statements.

        Args:
            lib: A library object to store symbols.
        """

    def add_param(self, name: str) -> None:
        """
        Add a parameter.

        Parameters are protected from simplification.

        Args:
            name: The name of the parameter.
        """

    def clear_params(self) -> None:
        """
        Remove all previously added parameters.
        """

    def add_theory(self, theory: StatementTheory) -> None:
        """
        Add a theory definition statement.

        The theory definition is used to rewrite theory atoms in statements.

        Args:
            theory: The theory statement to add.
        """

    @property
    def project_anonymous(self) -> bool:
        """
        Whether to project anonymous variables in negative literals.
        """

    @project_anonymous.setter
    def project_anonymous(self, value: bool) -> None: ...
    @property
    def project_mode(self) -> ProjectionMode:
        """
        The active projection mode.
        """

    @project_mode.setter
    def project_mode(self, value: ProjectionMode) -> None: ...

def parse_term(lib: Library, string: str) -> Term:
    """
    Parse a term.

    Args:
        lib: The library object for storing symbols.
        string: The string to parse.

    Returns:
        The parsed term.
    """

def parse_theory_term(lib: Library, string: str) -> TheoryTerm:
    """
    Parse a theory term.

    Args:
        lib: The library object for storing symbols.
        string: The string to parse.

    Returns:
        The parsed theory term.
    """

def parse_literal(lib: Library, string: str) -> Literal:
    """
    Parse a literal.

    Args:
        lib: The library object for storing symbols.
        string: The string to parse.

    Returns:
        The parsed literal.
    """

def parse_head_literal(lib: Library, string: str) -> HeadLiteral:
    """
    Parse a head literal.

    Args:
        lib: The library object for storing symbols.
        string: The string to parse.

    Returns:
        The parsed head literal.
    """

def parse_body_literal(lib: Library, string: str) -> BodyLiteral:
    """
    Parse a body literal.

    Args:
        lib: The library object for storing symbols.
        string: The string to parse.

    Returns:
        The parsed body literal.
    """

def parse_statement(lib: Library, string: str) -> Statement:
    """
    Parse a statement.

    Args:
        lib: The library object for storing symbols.
        string: The string to parse.

    Returns:
        The parsed statement.
    """

def parse_string(
    lib: Library,
    program: str,
    callback: Callable[[Statement], object],
    control: Control | None = None,
) -> None:
    """
    Parse the program in the given string.

    Args:
        lib: A library object to store symbols.
        program: The program to parse.
        callback: Function to report statements.
        control: Optional control object to handle ASPIF.
    """

def parse_files(
    lib: Library,
    files: Sequence[str],
    callback: Callable[[Statement], object],
    control: Control | None = None,
) -> None:
    """
    Parse the program in the given files.

    The parser follows clingo's handling of files on the command line. Filename
    "-" is treated as "STDIN" and if an empty list is given, then the parser
    will read from "STDIN".

    Args:
        lib: A library object to store symbols.
        files: The files to parse.
        callback: Function to report statements.
        control: Optional control object to handle ASPIF.
    """

def rewrite_statement(ctx: RewriteContext, statement: Statement) -> list[Statement]:
    """
    Simplify the given statement.

    Args:
        ctx: The rewrite context.
        statement: The statement to rewrite.

    Returns:
        The list of rewritten statements.
    """

def _type_info() -> dict[str, object]:
    """
    Get a description of the AST structure.

    The description is used to generate the stubs of the AST submodules.
    """
