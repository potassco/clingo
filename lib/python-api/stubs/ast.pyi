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

The next example hows how to visit and AST and collect variables.

```python
from functools import singledispatch

from clingo.core import Library
from clingo import ast

@singledispatch
def collect(stm, vars):
    \"\"\"
    Collect all variables occurring in statements.
    \"\"\"
    return stm.visit(collect, vars)

@collect.register
def _(var: ast.TermVariable | ast.TheoryTermVariable, vars):
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

@singledispatch
def rename(stm, lib):
    \"\"\"
    Replaces all occurrences of variable `X` with `Y` in the given statement
    returning a new statement. If no replacement was made, `None` is returned.
    \"\"\"
    return stm.transform(lib, rename, lib)

@rename.register
def _(var: ast.TermVariable, lib):
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

from __future__ import annotations

import collections.abc
import enum
import types
import typing

import clingo.core
import clingo.symbol

__all__: list[str] = [
    "AggregateFunction",
    "ArgumentTuple",
    "BinaryOperator",
    "BodyAggregate",
    "BodyAggregateElement",
    "BodyConditionalLiteral",
    "BodyLiteral",
    "BodySetAggregate",
    "BodySimpleLiteral",
    "BodySort",
    "BodyTheoryAtom",
    "CommentType",
    "DisjunctionElement",
    "Edge",
    "FormatField",
    "FormatFieldExpression",
    "FormatFieldLiteral",
    "HeadAggregate",
    "HeadAggregateElement",
    "HeadConditionalLiteral",
    "HeadDisjunction",
    "HeadLiteral",
    "HeadSetAggregate",
    "HeadSimpleLiteral",
    "HeadTheoryAtom",
    "IncludeType",
    "LeftGuard",
    "Literal",
    "LiteralBoolean",
    "LiteralComparison",
    "LiteralSymbolic",
    "OptimizeElement",
    "OptimizeTuple",
    "OptimizeType",
    "Precedence",
    "Program",
    "ProgramPart",
    "Projection",
    "ProjectionMode",
    "Relation",
    "RewriteContext",
    "RightGuard",
    "SetAggregateElement",
    "Sign",
    "Statement",
    "StatementComment",
    "StatementConst",
    "StatementDefined",
    "StatementEdge",
    "StatementExternal",
    "StatementHeuristic",
    "StatementInclude",
    "StatementOptimize",
    "StatementParts",
    "StatementProgram",
    "StatementProject",
    "StatementProjectSignature",
    "StatementRule",
    "StatementScript",
    "StatementShow",
    "StatementShowNothing",
    "StatementShowSignature",
    "StatementTheory",
    "StatementWeakConstraint",
    "Term",
    "TermAbsolute",
    "TermBinaryOperation",
    "TermFormatString",
    "TermFunction",
    "TermOrArgumentTuple",
    "TermOrProjection",
    "TermSymbolic",
    "TermTuple",
    "TermUnaryOperation",
    "TermVariable",
    "TheoryAtomDefinition",
    "TheoryAtomElement",
    "TheoryAtomType",
    "TheoryGuardDefinition",
    "TheoryOperatorDefinition",
    "TheoryOperatorType",
    "TheoryRightGuard",
    "TheoryTerm",
    "TheoryTermDefinition",
    "TheoryTermFunction",
    "TheoryTermSymbolic",
    "TheoryTermTuple",
    "TheoryTermUnparsed",
    "TheoryTermVariable",
    "TheoryTupleType",
    "UnaryOperator",
    "UnparsedElement",
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

def _type_info_yaml() -> str:
    """
    Return a yaml description of the AST.

    This can be used to auto-generate most of the binding.
    """

def parse_body_literal(
    lib: clingo.core.Library, string: str
) -> (
    BodySimpleLiteral
    | BodyAggregate
    | BodySort
    | BodySetAggregate
    | BodyTheoryAtom
    | BodyConditionalLiteral
):
    """
    Parse a body literal.

    Args:
        lib: The library object for storing symbols.
        string: The string to parse.

    Returns:
        The parsed BodyLiteral object.
    """

def parse_files(
    lib: clingo.core.Library,
    files: typing.Sequence[str],
    callback: collections.abc.Callable[
        [
            StatementRule
            | StatementTheory
            | StatementOptimize
            | StatementWeakConstraint
            | StatementShow
            | StatementShowNothing
            | StatementShowSignature
            | StatementProject
            | StatementProjectSignature
            | StatementDefined
            | StatementExternal
            | StatementEdge
            | StatementHeuristic
            | StatementScript
            | StatementInclude
            | StatementProgram
            | StatementParts
            | StatementConst
            | StatementComment
        ],
        None,
    ],
    control: clingo.control.Control | None = None,
) -> None:
    """
    Parse the program in the given files.

    The parser follows clingo's handling of files on the command line. Filename
    "-" is treated as "STDIN" and if an empty list is given, then the parser will
    read from "STDIN".

    Args:
        lib: A library object to store symbols.
        files: The files to parse.
        callback: Function to report statements.
        control: Optional Control object to handle ASPIF.
    """

def parse_head_literal(
    lib: clingo.core.Library, string: str
) -> (
    HeadSimpleLiteral
    | HeadAggregate
    | HeadSetAggregate
    | HeadTheoryAtom
    | HeadDisjunction
):
    """
    Parse a head literal.

    Args:
        lib: The library object for storing symbols.
        string: The string to parse.

    Returns:
        The parsed HeadLiteral object.
    """

def parse_literal(
    lib: clingo.core.Library, string: str
) -> LiteralBoolean | LiteralComparison | LiteralSymbolic:
    """
    Parse a literal.

    Args:
        lib: The library object for storing symbols.
        string: The string to parse.

    Returns:
        The parsed Literal object.
    """

def parse_statement(
    lib: clingo.core.Library, string: str
) -> (
    StatementRule
    | StatementTheory
    | StatementOptimize
    | StatementWeakConstraint
    | StatementShow
    | StatementShowNothing
    | StatementShowSignature
    | StatementProject
    | StatementProjectSignature
    | StatementDefined
    | StatementExternal
    | StatementEdge
    | StatementHeuristic
    | StatementScript
    | StatementInclude
    | StatementProgram
    | StatementParts
    | StatementConst
    | StatementComment
):
    """
    Parse a statement.

    Args:
        lib: The library object for storing symbols.
        string: The string to parse.

    Returns:
        The parsed Statement object.
    """

def parse_string(
    lib: clingo.core.Library,
    program: str,
    callback: collections.abc.Callable[
        [
            StatementRule
            | StatementTheory
            | StatementOptimize
            | StatementWeakConstraint
            | StatementShow
            | StatementShowNothing
            | StatementShowSignature
            | StatementProject
            | StatementProjectSignature
            | StatementDefined
            | StatementExternal
            | StatementEdge
            | StatementHeuristic
            | StatementScript
            | StatementInclude
            | StatementProgram
            | StatementParts
            | StatementConst
            | StatementComment
        ],
        None,
    ],
    control: clingo.control.Control | None = None,
) -> None:
    """
    Parse the program in the given string.

    Args:
        lib: A library object to store symbols.
        program: The program to parse.
        callback: Function to report statements.
        control: Optional Control object to handle ASPIF.
    """

def parse_term(
    lib: clingo.core.Library, string: str
) -> (
    TermVariable
    | TermSymbolic
    | TermAbsolute
    | TermUnaryOperation
    | TermBinaryOperation
    | TermTuple
    | TermFunction
    | TermFormatString
):
    """
    Parse a term.

    Args:
        lib: The library object for storing symbols.
        string: The string to parse.

    Returns:
        The parsed Term object.
    """

def parse_theory_term(
    lib: clingo.core.Library, string: str
) -> (
    TheoryTermVariable
    | TheoryTermSymbolic
    | TheoryTermTuple
    | TheoryTermFunction
    | TheoryTermUnparsed
):
    """
    Parse a theory term.

    Args:
        lib: The library object for storing symbols.
        string: The string to parse.

    Returns:
        The parsed TheoryTerm object.
    """

def rewrite_statement(
    ctx: RewriteContext,
    statement: (
        StatementRule
        | StatementTheory
        | StatementOptimize
        | StatementWeakConstraint
        | StatementShow
        | StatementShowNothing
        | StatementShowSignature
        | StatementProject
        | StatementProjectSignature
        | StatementDefined
        | StatementExternal
        | StatementEdge
        | StatementHeuristic
        | StatementScript
        | StatementInclude
        | StatementProgram
        | StatementParts
        | StatementConst
        | StatementComment
    ),
) -> typing.Sequence[
    StatementRule
    | StatementTheory
    | StatementOptimize
    | StatementWeakConstraint
    | StatementShow
    | StatementShowNothing
    | StatementShowSignature
    | StatementProject
    | StatementProjectSignature
    | StatementDefined
    | StatementExternal
    | StatementEdge
    | StatementHeuristic
    | StatementScript
    | StatementInclude
    | StatementProgram
    | StatementParts
    | StatementConst
    | StatementComment
]:
    """
    Simplify the given statement.

    Args:
        ctx: The rewrite context.
        statement: The statement to rewrite.

    Returns:
        A list of rewritten statements.
    """

class AggregateFunction(enum.IntEnum):
    """
    Enumeration of aggregate functions.
    """

    Count: typing.ClassVar[AggregateFunction]  # value = <AggregateFunction.Count: 0>
    Max: typing.ClassVar[AggregateFunction]  # value = <AggregateFunction.Max: 4>
    Min: typing.ClassVar[AggregateFunction]  # value = <AggregateFunction.Min: 3>
    Sum: typing.ClassVar[AggregateFunction]  # value = <AggregateFunction.Sum: 1>
    Sump: typing.ClassVar[AggregateFunction]  # value = <AggregateFunction.Sump: 2>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class BinaryOperator(enum.IntEnum):
    """
    Available binary operators.
    """

    And: typing.ClassVar[BinaryOperator]  # value = <BinaryOperator.And: 0>
    Division: typing.ClassVar[BinaryOperator]  # value = <BinaryOperator.Division: 1>
    Minus: typing.ClassVar[BinaryOperator]  # value = <BinaryOperator.Minus: 2>
    Modulo: typing.ClassVar[BinaryOperator]  # value = <BinaryOperator.Modulo: 3>
    Multiplication: typing.ClassVar[
        BinaryOperator
    ]  # value = <BinaryOperator.Multiplication: 4>
    Or: typing.ClassVar[BinaryOperator]  # value = <BinaryOperator.Or: 5>
    Plus: typing.ClassVar[BinaryOperator]  # value = <BinaryOperator.Plus: 6>
    Power: typing.ClassVar[BinaryOperator]  # value = <BinaryOperator.Power: 7>
    Xor: typing.ClassVar[BinaryOperator]  # value = <BinaryOperator.Xor: 8>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class CommentType(enum.IntEnum):
    """
    Enumeration of comment types.
    """

    Block: typing.ClassVar[CommentType]  # value = <CommentType.Block: 1>
    Line: typing.ClassVar[CommentType]  # value = <CommentType.Line: 0>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class IncludeType(enum.IntEnum):
    """
    Enumeration of include types.
    """

    Inbuild: typing.ClassVar[IncludeType]  # value = <IncludeType.Inbuild: 1>
    System: typing.ClassVar[IncludeType]  # value = <IncludeType.System: 0>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class OptimizeType(enum.IntEnum):
    """
    Enumeration of optimization types.
    """

    Maximize: typing.ClassVar[OptimizeType]  # value = <OptimizeType.Maximize: 1>
    Minimize: typing.ClassVar[OptimizeType]  # value = <OptimizeType.Minimize: 0>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class Precedence(enum.IntEnum):
    """
    Enumeration of precedences values.
    """

    Default: typing.ClassVar[Precedence]  # value = <Precedence.Default: 0>
    Override: typing.ClassVar[Precedence]  # value = <Precedence.Override: 1>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class ProjectionMode(enum.IntEnum):
    """
    Available projection modes.
    """

    Anonymous: typing.ClassVar[ProjectionMode]  # value = <ProjectionMode.Anonymous: 1>
    Disabled: typing.ClassVar[ProjectionMode]  # value = <ProjectionMode.Disabled: 0>
    Pure: typing.ClassVar[ProjectionMode]  # value = <ProjectionMode.Pure: 2>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class Relation(enum.IntEnum):
    """
    Available relation symbols.
    """

    Equal: typing.ClassVar[Relation]  # value = <Relation.Equal: 0>
    Greater: typing.ClassVar[Relation]  # value = <Relation.Greater: 4>
    GreaterEqual: typing.ClassVar[Relation]  # value = <Relation.GreaterEqual: 5>
    Less: typing.ClassVar[Relation]  # value = <Relation.Less: 2>
    LessEqual: typing.ClassVar[Relation]  # value = <Relation.LessEqual: 3>
    NotEqual: typing.ClassVar[Relation]  # value = <Relation.NotEqual: 1>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class Sign(enum.IntEnum):
    """
    The available signs.
    """

    Double: typing.ClassVar[Sign]  # value = <Sign.Double: 2>
    NoSign: typing.ClassVar[Sign]  # value = <Sign.NoSign: 0>
    Single: typing.ClassVar[Sign]  # value = <Sign.Single: 1>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class TheoryAtomType(enum.IntEnum):
    """
    Enumeration of the theory atom types.
    """

    Any: typing.ClassVar[TheoryAtomType]  # value = <TheoryAtomType.Any: 2>
    Body: typing.ClassVar[TheoryAtomType]  # value = <TheoryAtomType.Body: 1>
    Directive: typing.ClassVar[TheoryAtomType]  # value = <TheoryAtomType.Directive: 3>
    Head: typing.ClassVar[TheoryAtomType]  # value = <TheoryAtomType.Head: 0>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class TheoryOperatorType(enum.IntEnum):
    """
    Enumeration of theory operators.
    """

    BinaryLeft: typing.ClassVar[
        TheoryOperatorType
    ]  # value = <TheoryOperatorType.BinaryLeft: 1>
    BinaryRight: typing.ClassVar[
        TheoryOperatorType
    ]  # value = <TheoryOperatorType.BinaryRight: 2>
    Unary: typing.ClassVar[TheoryOperatorType]  # value = <TheoryOperatorType.Unary: 0>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class TheoryTupleType(enum.IntEnum):
    """
    Enumeration of theory tuple types.
    """

    List: typing.ClassVar[TheoryTupleType]  # value = <TheoryTupleType.List: 2>
    Set: typing.ClassVar[TheoryTupleType]  # value = <TheoryTupleType.Set: 1>
    Tuple: typing.ClassVar[TheoryTupleType]  # value = <TheoryTupleType.Tuple: 0>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class UnaryOperator(enum.IntEnum):
    """
    Available unary operators.
    """

    Minus: typing.ClassVar[UnaryOperator]  # value = <UnaryOperator.Minus: 0>
    Negation: typing.ClassVar[UnaryOperator]  # value = <UnaryOperator.Negation: 1>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class ArgumentTuple:
    """
    A list of arguments for a function or tuple.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        arguments: typing.Iterable[
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
            | Projection
        ] = [],
    ) -> None:
        """
        Construct a ArgumentTuple object.

        Args:
            lib: The library object for storing symbols.
            arguments: The arguments of the tuple.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> ArgumentTuple | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> ArgumentTuple:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def arguments(
        self,
    ) -> typing.Sequence[
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
        | Projection
    ]:
        """
        The arguments of the tuple.
        """

class BodyAggregate:
    """
    An aggregate in a rule body.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        sign: Sign,
        left: LeftGuard | None,
        function: AggregateFunction,
        elements: typing.Iterable[BodyAggregateElement],
        right: RightGuard | None,
    ) -> None:
        """
        Construct a BodyAggregate object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            sign: The sign of the literal.
            left: The left guard of the aggregate.
            function: The aggregate function.
            elements: The aggregate elements.
            right: The right guard of the aggregate.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> BodyAggregate | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> BodyAggregate:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def elements(self) -> typing.Sequence[BodyAggregateElement]:
        """
        The aggregate elements.
        """

    @property
    def function(self) -> AggregateFunction:
        """
        The aggregate function.
        """

    @property
    def left(self) -> LeftGuard | None:
        """
        The left guard of the aggregate.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the element.
        """

    @property
    def right(self) -> RightGuard | None:
        """
        The right guard of the aggregate.
        """

    @property
    def sign(self) -> Sign:
        """
        The sign of the literal.
        """

class BodySort:
    """
    A sort literal in a rule body.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int: ...
    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        sign: Sign,
        outputs: Term,
        elements: typing.Iterable[BodyAggregateElement],
    ) -> None: ...
    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> BodySort | None: ...
    def update(self, lib: clingo.core.Library, **kwargs) -> BodySort: ...
    def visit(self, visitor: typing.Any, *args, **kwargs) -> None: ...

    @property
    def elements(self) -> typing.Sequence[BodyAggregateElement]: ...
    @property
    def location(self) -> clingo.core.Location: ...
    @property
    def outputs(self) -> Term: ...
    @property
    def sign(self) -> Sign: ...

class BodyAggregateElement:
    """
    An element of a body aggregate.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        tuple: typing.Iterable[
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ],
        condition: typing.Iterable[
            LiteralBoolean | LiteralComparison | LiteralSymbolic
        ],
    ) -> None:
        """
        Construct a BodyAggregateElement object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            tuple: The term tuple of the element.
            condition: The condition of the element.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> BodyAggregateElement | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> BodyAggregateElement:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def condition(
        self,
    ) -> typing.Sequence[LiteralBoolean | LiteralComparison | LiteralSymbolic]:
        """
        The condition of the element.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the element.
        """

    @property
    def tuple(
        self,
    ) -> typing.Sequence[
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ]:
        """
        The term tuple of the element.
        """

class BodyConditionalLiteral:
    """
    A conditional_literal.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        literal: LiteralBoolean | LiteralComparison | LiteralSymbolic,
        condition: typing.Iterable[
            LiteralBoolean | LiteralComparison | LiteralSymbolic
        ],
    ) -> None:
        """
        Construct a BodyConditionalLiteral object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            literal: The literal of the element.
            condition: The condition of the element.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> BodyConditionalLiteral | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> BodyConditionalLiteral:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def condition(
        self,
    ) -> typing.Sequence[LiteralBoolean | LiteralComparison | LiteralSymbolic]:
        """
        The condition of the element.
        """

    @property
    def literal(self) -> LiteralBoolean | LiteralComparison | LiteralSymbolic:
        """
        The literal of the element.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the element.
        """

class BodySetAggregate:
    """
    A set aggregate.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        sign: Sign,
        left: LeftGuard | None,
        elements: typing.Iterable[SetAggregateElement],
        right: RightGuard | None,
    ) -> None:
        """
        Construct a BodySetAggregate object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            sign: The sign of the literal.
            left: The left guard of the aggregate.
            elements: The aggregate elements.
            right: The right guard of the aggregate.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> BodySetAggregate | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> BodySetAggregate:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def elements(self) -> typing.Sequence[SetAggregateElement]:
        """
        The aggregate elements.
        """

    @property
    def left(self) -> LeftGuard | None:
        """
        The left guard of the aggregate.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the element.
        """

    @property
    def right(self) -> RightGuard | None:
        """
        The right guard of the aggregate.
        """

    @property
    def sign(self) -> Sign:
        """
        The sign of the literal.
        """

class BodySimpleLiteral:
    """
    A literal in a rule body.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        literal: LiteralBoolean | LiteralComparison | LiteralSymbolic,
    ) -> None:
        """
        Construct a BodySimpleLiteral object.

        Args:
            lib: The library object for storing symbols.
            literal: The literal.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> BodySimpleLiteral | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> BodySimpleLiteral:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def literal(self) -> LiteralBoolean | LiteralComparison | LiteralSymbolic:
        """
        The literal.
        """

class BodyTheoryAtom:
    """
    A theory atom.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        sign: Sign,
        name: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        elements: typing.Iterable[TheoryAtomElement],
        right: TheoryRightGuard | None,
    ) -> None:
        """
        Construct a BodyTheoryAtom object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            sign: The sign of the literal.
            name: The name of the theory atom.
            elements: The aggregate elements.
            right: The right guard of the theory atom.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> BodyTheoryAtom | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> BodyTheoryAtom:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def elements(self) -> typing.Sequence[TheoryAtomElement]:
        """
        The aggregate elements.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the element.
        """

    @property
    def name(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The name of the theory atom.
        """

    @property
    def right(self) -> TheoryRightGuard | None:
        """
        The right guard of the theory atom.
        """

    @property
    def sign(self) -> Sign:
        """
        The sign of the literal.
        """

class Edge:
    """
    An edge of an edge statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        u: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        v: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
    ) -> None:
        """
        Construct a Edge object.

        Args:
            lib: The library object for storing symbols.
            u: The start vertex.
            v: The end vertex.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> Edge | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> Edge:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def u(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The start vertex.
        """

    @property
    def v(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The end vertex.
        """

class FormatFieldExpression:
    """
    An expression part of a format string.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        left: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        right: str,
    ) -> None:
        """
        Construct a FormatFieldExpression object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the expression.
            left: The term of the expression.
            right: The format specifier of the expression.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> FormatFieldExpression | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> FormatFieldExpression:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def left(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The term of the expression.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the expression.
        """

    @property
    def right(self) -> str:
        """
        The format specifier of the expression.
        """

class FormatFieldLiteral:
    """
    A literal part of a format string.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self, lib: clingo.core.Library, location: clingo.core.Location, value: str
    ) -> None:
        """
        Construct a FormatFieldLiteral object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the literal.
            value: The value of the literal.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> FormatFieldLiteral | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> FormatFieldLiteral:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the literal.
        """

    @property
    def value(self) -> str:
        """
        The value of the literal.
        """

class HeadAggregate:
    """
    An aggregate in a rule head.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        left: LeftGuard | None,
        function: AggregateFunction,
        elements: typing.Iterable[HeadAggregateElement],
        right: RightGuard | None,
    ) -> None:
        """
        Construct a HeadAggregate object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            left: The left guard of the aggregate.
            function: The aggregate function.
            elements: The aggregate elements.
            right: The right guard of the aggregate.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> HeadAggregate | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> HeadAggregate:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def elements(self) -> typing.Sequence[HeadAggregateElement]:
        """
        The aggregate elements.
        """

    @property
    def function(self) -> AggregateFunction:
        """
        The aggregate function.
        """

    @property
    def left(self) -> LeftGuard | None:
        """
        The left guard of the aggregate.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the element.
        """

    @property
    def right(self) -> RightGuard | None:
        """
        The right guard of the aggregate.
        """

class HeadAggregateElement:
    """
    An element of a head aggregate.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        tuple: typing.Iterable[
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ],
        literal: LiteralBoolean | LiteralComparison | LiteralSymbolic,
        condition: typing.Iterable[
            LiteralBoolean | LiteralComparison | LiteralSymbolic
        ],
    ) -> None:
        """
        Construct a HeadAggregateElement object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            tuple: The term tuple of the element.
            literal: The literal of the element.
            condition: The condition of the element.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> HeadAggregateElement | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> HeadAggregateElement:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def condition(
        self,
    ) -> typing.Sequence[LiteralBoolean | LiteralComparison | LiteralSymbolic]:
        """
        The condition of the element.
        """

    @property
    def literal(self) -> LiteralBoolean | LiteralComparison | LiteralSymbolic:
        """
        The literal of the element.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the element.
        """

    @property
    def tuple(
        self,
    ) -> typing.Sequence[
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ]:
        """
        The term tuple of the element.
        """

class HeadConditionalLiteral:
    """
    A conditional_literal.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        literal: LiteralBoolean | LiteralComparison | LiteralSymbolic,
        condition: typing.Iterable[
            LiteralBoolean | LiteralComparison | LiteralSymbolic
        ],
    ) -> None:
        """
        Construct a HeadConditionalLiteral object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            literal: The literal of the element.
            condition: The condition of the element.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> HeadConditionalLiteral | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> HeadConditionalLiteral:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def condition(
        self,
    ) -> typing.Sequence[LiteralBoolean | LiteralComparison | LiteralSymbolic]:
        """
        The condition of the element.
        """

    @property
    def literal(self) -> LiteralBoolean | LiteralComparison | LiteralSymbolic:
        """
        The literal of the element.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the element.
        """

class HeadDisjunction:
    """
    A disjunction.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        elements: typing.Iterable[
            LiteralBoolean
            | LiteralComparison
            | LiteralSymbolic
            | HeadConditionalLiteral
        ],
    ) -> None:
        """
        Construct a HeadDisjunction object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            elements: The elements of the disjunction.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> HeadDisjunction | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> HeadDisjunction:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def elements(
        self,
    ) -> typing.Sequence[
        LiteralBoolean | LiteralComparison | LiteralSymbolic | HeadConditionalLiteral
    ]:
        """
        The elements of the disjunction.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the element.
        """

class HeadSetAggregate:
    """
    A set aggregate.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        left: LeftGuard | None,
        elements: typing.Iterable[SetAggregateElement],
        right: RightGuard | None,
    ) -> None:
        """
        Construct a HeadSetAggregate object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            left: The left guard of the aggregate.
            elements: The aggregate elements.
            right: The right guard of the aggregate.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> HeadSetAggregate | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> HeadSetAggregate:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def elements(self) -> typing.Sequence[SetAggregateElement]:
        """
        The aggregate elements.
        """

    @property
    def left(self) -> LeftGuard | None:
        """
        The left guard of the aggregate.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the element.
        """

    @property
    def right(self) -> RightGuard | None:
        """
        The right guard of the aggregate.
        """

class HeadSimpleLiteral:
    """
    A literal in a rule head.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        literal: LiteralBoolean | LiteralComparison | LiteralSymbolic,
    ) -> None:
        """
        Construct a HeadSimpleLiteral object.

        Args:
            lib: The library object for storing symbols.
            literal: The literal.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> HeadSimpleLiteral | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> HeadSimpleLiteral:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def literal(self) -> LiteralBoolean | LiteralComparison | LiteralSymbolic:
        """
        The literal.
        """

class HeadTheoryAtom:
    """
    A theory atom.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        elements: typing.Iterable[TheoryAtomElement],
        right: TheoryRightGuard | None,
    ) -> None:
        """
        Construct a HeadTheoryAtom object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            name: The name of the theory atom.
            elements: The aggregate elements.
            right: The right guard of the theory atom.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> HeadTheoryAtom | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> HeadTheoryAtom:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def elements(self) -> typing.Sequence[TheoryAtomElement]:
        """
        The aggregate elements.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the element.
        """

    @property
    def name(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The name of the theory atom.
        """

    @property
    def right(self) -> TheoryRightGuard | None:
        """
        The right guard of the theory atom.
        """

class LeftGuard:
    """
    A left hand side guard consisting of a term and relation.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        term: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        relation: Relation,
    ) -> None:
        """
        Construct a LeftGuard object.

        Args:
            lib: The library object for storing symbols.
            term: The term of the guard.
            relation: The relation of the guard.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> LeftGuard | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> LeftGuard:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def relation(self) -> Relation:
        """
        The relation of the guard.
        """

    @property
    def term(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The term of the guard.
        """

class LiteralBoolean:
    """
    A literal representing a Boolean constant.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        sign: Sign,
        value: bool,
    ) -> None:
        """
        Construct a LiteralBoolean object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the symbol.
            sign: The sign of the literal.
            value: The fixed value of the literal.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> LiteralBoolean | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> LiteralBoolean:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the symbol.
        """

    @property
    def sign(self) -> Sign:
        """
        The sign of the literal.
        """

    @property
    def value(self) -> bool:
        """
        The fixed value of the literal.
        """

class LiteralComparison:
    """
    A literal representing a (chain of) comparison(s).
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        sign: Sign,
        left: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        right: typing.Iterable[RightGuard],
    ) -> None:
        """
        Construct a LiteralComparison object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the symbol.
            sign: The sign of the literal.
            left: The first term of the comparison.
            right: The chain of comparisons.

                Note that the chain must have at least length one.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> LiteralComparison | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> LiteralComparison:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def left(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The first term of the comparison.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the symbol.
        """

    @property
    def right(self) -> typing.Sequence[RightGuard]:
        """
        The chain of comparisons.
        Note that the chain must have at least length one.
        """

    @property
    def sign(self) -> Sign:
        """
        The sign of the literal.
        """

class LiteralSymbolic:
    """
    A literal representing a symbolic literal.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        sign: Sign,
        atom: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
    ) -> None:
        """
        Construct a LiteralSymbolic object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the symbol.
            sign: The sign of the literal.
            atom: The term representing the atom.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> LiteralSymbolic | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> LiteralSymbolic:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def atom(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The term representing the atom.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the symbol.
        """

    @property
    def sign(self) -> Sign:
        """
        The sign of the literal.
        """

class OptimizeElement:
    """
    An element of an optimization statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        tuple: OptimizeTuple,
        condition: typing.Iterable[
            LiteralBoolean | LiteralComparison | LiteralSymbolic
        ],
    ) -> None:
        """
        Construct a OptimizeElement object.

        Args:
            lib: The library object for storing symbols.
            tuple: The tuple of the element.
            condition: The condition of the element.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> OptimizeElement | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> OptimizeElement:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def condition(
        self,
    ) -> typing.Sequence[LiteralBoolean | LiteralComparison | LiteralSymbolic]:
        """
        The condition of the element.
        """

    @property
    def tuple(self) -> OptimizeTuple:
        """
        The tuple of the element.
        """

class OptimizeTuple:
    """
    A tuple of an optimizization statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        weight: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        priority: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
            | None
        ),
        terms: typing.Iterable[
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ],
    ) -> None:
        """
        Construct a OptimizeTuple object.

        Args:
            lib: The library object for storing symbols.
            weight: The weight of the tuple.
            priority: An optional priority.
            terms: The remaining terms in the tuple.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> OptimizeTuple | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> OptimizeTuple:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def priority(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
        | None
    ):
        """
        An optional priority.
        """

    @property
    def terms(
        self,
    ) -> typing.Sequence[
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ]:
        """
        The remaining terms in the tuple.
        """

    @property
    def weight(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The weight of the tuple.
        """

class Program:
    """
    A non-ground program.
    """

    def __init__(self, lib: clingo.core.Library) -> None:
        """
        Create an empty non-ground program.

        Args:
            lib: A library object to store symbols.
        """

    def add(
        self,
        statement: (
            StatementRule
            | StatementTheory
            | StatementOptimize
            | StatementWeakConstraint
            | StatementShow
            | StatementShowNothing
            | StatementShowSignature
            | StatementProject
            | StatementProjectSignature
            | StatementDefined
            | StatementExternal
            | StatementEdge
            | StatementHeuristic
            | StatementScript
            | StatementInclude
            | StatementProgram
            | StatementParts
            | StatementConst
            | StatementComment
        ),
    ) -> None:
        """
        Add a statement to a program.

        Args:
            statement: The statement to add.
        """

class ProgramPart:
    """
    A program part to ground.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        name: str,
        arguments: typing.Iterable[clingo.symbol.Symbol],
    ) -> None:
        """
        Construct a ProgramPart object.

        Args:
            lib: The library object for storing symbols.
            name: The name of the program part.
            arguments: The arguments of the program part.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> ProgramPart | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> ProgramPart:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def arguments(self) -> typing.Sequence[clingo.symbol.Symbol]:
        """
        The arguments of the program part.
        """

    @property
    def name(self) -> str:
        """
        The name of the program part.
        """

class Projection:
    """
    A placeholder for an argument to project.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self, lib: clingo.core.Library, location: clingo.core.Location
    ) -> None:
        """
        Construct a Projection object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the placeholder.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> Projection | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> Projection:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the placeholder.
        """

class RewriteContext:
    """
    Context to rewrite statements.
    """

    def __init__(self, lib: clingo.core.Library) -> None:
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

    def add_theory(self, theory: StatementTheory) -> None:
        """
        Add a theory definition statement.

        The theory definition is used to rewrite theory atoms in statements.

        Args:
            theory: The theory statement to add.
        """

    def clear_params(self) -> None:
        """
        Remove previously added params
        """

    @property
    def project_anonymous(self) -> bool:
        """
        Whether to project anonymous variables in negative literals.
        """

    @project_anonymous.setter
    def project_anonymous(self, arg1: bool) -> None: ...
    @property
    def project_mode(self) -> ProjectionMode:
        """
        The active projection mode.
        """

    @project_mode.setter
    def project_mode(self, arg1: ProjectionMode) -> None: ...

class RightGuard:
    """
    A right hand side guard consisting of a relation and term.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        relation: Relation,
        term: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
    ) -> None:
        """
        Construct a RightGuard object.

        Args:
            lib: The library object for storing symbols.
            relation: The relation of the guard.
            term: The term of the guard.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> RightGuard | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> RightGuard:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def relation(self) -> Relation:
        """
        The relation of the guard.
        """

    @property
    def term(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The term of the guard.
        """

class SetAggregateElement:
    """
    An element of a set aggregate.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        literal: LiteralBoolean | LiteralComparison | LiteralSymbolic,
        condition: typing.Iterable[
            LiteralBoolean | LiteralComparison | LiteralSymbolic
        ],
    ) -> None:
        """
        Construct a SetAggregateElement object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            literal: The literal of the element.
            condition: The condition of the element.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> SetAggregateElement | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> SetAggregateElement:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def condition(
        self,
    ) -> typing.Sequence[LiteralBoolean | LiteralComparison | LiteralSymbolic]:
        """
        The condition of the element.
        """

    @property
    def literal(self) -> LiteralBoolean | LiteralComparison | LiteralSymbolic:
        """
        The literal of the element.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the element.
        """

class StatementComment:
    """
    A comment.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        value: str,
        comment_type: CommentType,
    ) -> None:
        """
        Construct a StatementComment object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the comment.
            value: The value of the comment.
            comment_type: The type of the comment.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementComment | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementComment:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def comment_type(self) -> CommentType:
        """
        The type of the comment.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the comment.
        """

    @property
    def value(self) -> str:
        """
        The value of the comment.
        """

class StatementConst:
    """
    A const statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: str,
        value: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        precedence: Precedence,
    ) -> None:
        """
        Construct a StatementConst object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the statement.
            value: The term of the statement.
            precedence: The precedence of the statement.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementConst | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementConst:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def name(self) -> str:
        """
        The name of the statement.
        """

    @property
    def precedence(self) -> Precedence:
        """
        The precedence of the statement.
        """

    @property
    def value(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The term of the statement.
        """

class StatementDefined:
    """
    A defined statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: str,
        arity: int,
        sign: bool = False,
    ) -> None:
        """
        Construct a StatementDefined object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the atom to project.
            arity: The arity of the atom to project.
            sign: The classical sign of the atom.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementDefined | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementDefined:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def arity(self) -> int:
        """
        The arity of the atom to project.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def name(self) -> str:
        """
        The name of the atom to project.
        """

    @property
    def sign(self) -> bool:
        """
        The classical sign of the atom.
        """

class StatementEdge:
    """
    An edge statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        pool: typing.Iterable[Edge],
        body: typing.Iterable[
            BodySimpleLiteral
            | BodyAggregate
            | BodySetAggregate
            | BodyTheoryAtom
            | BodyConditionalLiteral
        ],
    ) -> None:
        """
        Construct a StatementEdge object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            pool: The edge pool of the statement.
            body: The body of the statement.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementEdge | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementEdge:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def body(
        self,
    ) -> typing.Sequence[
        BodySimpleLiteral
        | BodyAggregate
        | BodySetAggregate
        | BodyTheoryAtom
        | BodyConditionalLiteral
    ]:
        """
        The body of the statement.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def pool(self) -> typing.Sequence[Edge]:
        """
        The edge pool of the statement.
        """

class StatementExternal:
    """
    An external statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        atom: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        body: typing.Iterable[
            BodySimpleLiteral
            | BodyAggregate
            | BodySetAggregate
            | BodyTheoryAtom
            | BodyConditionalLiteral
        ],
        external_type: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
            | None
        ) = None,
    ) -> None:
        """
        Construct a StatementExternal object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            atom: The atom to project.
            body: The body of the statement.
            external_type: The type of the external.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementExternal | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementExternal:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def atom(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The atom to project.
        """

    @property
    def body(
        self,
    ) -> typing.Sequence[
        BodySimpleLiteral
        | BodyAggregate
        | BodySetAggregate
        | BodyTheoryAtom
        | BodyConditionalLiteral
    ]:
        """
        The body of the statement.
        """

    @property
    def external_type(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
        | None
    ):
        """
        The type of the external.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

class StatementHeuristic:
    """
    A heuristic statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        atom: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        body: typing.Iterable[
            BodySimpleLiteral
            | BodyAggregate
            | BodySetAggregate
            | BodyTheoryAtom
            | BodyConditionalLiteral
        ],
        weight: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        modifier: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        priority: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
            | None
        ) = None,
    ) -> None:
        """
        Construct a StatementHeuristic object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            atom: The atom to heuristically modify.
            body: The body of the statement.
            weight: The weight of the heuristic modification.
            modifier: The heuristic modifier.
            priority: An optional priority.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementHeuristic | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementHeuristic:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def atom(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The atom to heuristically modify.
        """

    @property
    def body(
        self,
    ) -> typing.Sequence[
        BodySimpleLiteral
        | BodyAggregate
        | BodySetAggregate
        | BodyTheoryAtom
        | BodyConditionalLiteral
    ]:
        """
        The body of the statement.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def modifier(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The heuristic modifier.
        """

    @property
    def priority(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
        | None
    ):
        """
        An optional priority.
        """

    @property
    def weight(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The weight of the heuristic modification.
        """

class StatementInclude:
    """
    An include statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        value: str,
        include_type: IncludeType,
    ) -> None:
        """
        Construct a StatementInclude object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            value: The path of the statement.
            include_type: The type of the include.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementInclude | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementInclude:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def include_type(self) -> IncludeType:
        """
        The type of the include.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def value(self) -> str:
        """
        The path of the statement.
        """

class StatementOptimize:
    """
    An optimization statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        elements: typing.Iterable[OptimizeElement],
        optimize_type: OptimizeType,
    ) -> None:
        """
        Construct a StatementOptimize object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            elements: The elements of the statement.
            optimize_type: The type of the statement.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementOptimize | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementOptimize:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def elements(self) -> typing.Sequence[OptimizeElement]:
        """
        The elements of the statement.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def optimize_type(self) -> OptimizeType:
        """
        The type of the statement.
        """

class StatementParts:
    """
    A program parts statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        elements: typing.Iterable[ProgramPart],
        precedence: Precedence,
    ) -> None:
        """
        Construct a StatementParts object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            elements: The program parts to ground.
            precedence: The precedence of the statement.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementParts | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementParts:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def elements(self) -> typing.Sequence[ProgramPart]:
        """
        The program parts to ground.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def precedence(self) -> Precedence:
        """
        The precedence of the statement.
        """

class StatementProgram:
    """
    A program statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: str,
        arguments: typing.Iterable[str],
    ) -> None:
        """
        Construct a StatementProgram object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the program.
            arguments: The arguments of the program.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementProgram | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementProgram:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def arguments(self) -> typing.Sequence[str]:
        """
        The arguments of the program.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def name(self) -> str:
        """
        The name of the program.
        """

class StatementProject:
    """
    A project statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        atom: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        body: typing.Iterable[
            BodySimpleLiteral
            | BodyAggregate
            | BodySetAggregate
            | BodyTheoryAtom
            | BodyConditionalLiteral
        ],
    ) -> None:
        """
        Construct a StatementProject object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            atom: The atom to project.
            body: The body of the statement.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementProject | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementProject:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def atom(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The atom to project.
        """

    @property
    def body(
        self,
    ) -> typing.Sequence[
        BodySimpleLiteral
        | BodyAggregate
        | BodySetAggregate
        | BodyTheoryAtom
        | BodyConditionalLiteral
    ]:
        """
        The body of the statement.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

class StatementProjectSignature:
    """
    A project signature statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: str,
        arity: int,
        sign: bool = False,
    ) -> None:
        """
        Construct a StatementProjectSignature object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the atom to project.
            arity: The arity of the atom to project.
            sign: The classical sign of the atom.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementProjectSignature | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementProjectSignature:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def arity(self) -> int:
        """
        The arity of the atom to project.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def name(self) -> str:
        """
        The name of the atom to project.
        """

    @property
    def sign(self) -> bool:
        """
        The classical sign of the atom.
        """

class StatementRule:
    """
    A rule.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        head: (
            HeadSimpleLiteral
            | HeadAggregate
            | HeadSetAggregate
            | HeadTheoryAtom
            | HeadDisjunction
        ),
        body: typing.Iterable[
            BodySimpleLiteral
            | BodyAggregate
            | BodySetAggregate
            | BodyTheoryAtom
            | BodyConditionalLiteral
        ],
    ) -> None:
        """
        Construct a StatementRule object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            head: The head literal.
            body: The body of the statement.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementRule | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementRule:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def body(
        self,
    ) -> typing.Sequence[
        BodySimpleLiteral
        | BodyAggregate
        | BodySetAggregate
        | BodyTheoryAtom
        | BodyConditionalLiteral
    ]:
        """
        The body of the statement.
        """

    @property
    def head(
        self,
    ) -> (
        HeadSimpleLiteral
        | HeadAggregate
        | HeadSetAggregate
        | HeadTheoryAtom
        | HeadDisjunction
    ):
        """
        The head literal.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

class StatementScript:
    """
    A script statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        value: str,
        script_type: str,
    ) -> None:
        """
        Construct a StatementScript object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            value: The content of the script.
            script_type: The type of the script.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementScript | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementScript:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def script_type(self) -> str:
        """
        The type of the script.
        """

    @property
    def value(self) -> str:
        """
        The content of the script.
        """

class StatementShow:
    """
    A show statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        term: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        body: typing.Iterable[
            BodySimpleLiteral
            | BodyAggregate
            | BodySetAggregate
            | BodyTheoryAtom
            | BodyConditionalLiteral
        ],
    ) -> None:
        """
        Construct a StatementShow object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            term: The term to show.
            body: The body of the statement.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementShow | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementShow:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def body(
        self,
    ) -> typing.Sequence[
        BodySimpleLiteral
        | BodyAggregate
        | BodySetAggregate
        | BodyTheoryAtom
        | BodyConditionalLiteral
    ]:
        """
        The body of the statement.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def term(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The term to show.
        """

class StatementShowNothing:
    """
    An empty show statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self, lib: clingo.core.Library, location: clingo.core.Location
    ) -> None:
        """
        Construct a StatementShowNothing object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementShowNothing | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementShowNothing:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

class StatementShowSignature:
    """
    A show signature statement.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: str,
        arity: int,
        sign: bool = False,
        value: bool = True,
    ) -> None:
        """
        Construct a StatementShowSignature object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the predicate to show.
            arity: The arity of the predicate to show.
            sign: The classical sign of the atom.
            value: Whether to show or hide the predicate.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementShowSignature | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementShowSignature:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def arity(self) -> int:
        """
        The arity of the predicate to show.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def name(self) -> str:
        """
        The name of the predicate to show.
        """

    @property
    def sign(self) -> bool:
        """
        The classical sign of the atom.
        """

    @property
    def value(self) -> bool:
        """
        Whether to show or hide the predicate.
        """

class StatementTheory:
    """
    A theory definition.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: str,
        terms: typing.Iterable[TheoryTermDefinition],
        atoms: typing.Iterable[TheoryAtomDefinition],
    ) -> None:
        """
        Construct a StatementTheory object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the theory.
            terms: A list of term definitions.
            atoms: A list of atom definitions.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementTheory | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementTheory:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def atoms(self) -> typing.Sequence[TheoryAtomDefinition]:
        """
        A list of atom definitions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def name(self) -> str:
        """
        The name of the theory.
        """

    @property
    def terms(self) -> typing.Sequence[TheoryTermDefinition]:
        """
        A list of term definitions.
        """

class StatementWeakConstraint:
    """
    A weak constraint.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        body: typing.Iterable[
            BodySimpleLiteral
            | BodyAggregate
            | BodySetAggregate
            | BodyTheoryAtom
            | BodyConditionalLiteral
        ],
        tuple: OptimizeTuple,
    ) -> None:
        """
        Construct a StatementWeakConstraint object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            body: The body of the statement.
            tuple: The tuple of the statement.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> StatementWeakConstraint | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> StatementWeakConstraint:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def body(
        self,
    ) -> typing.Sequence[
        BodySimpleLiteral
        | BodyAggregate
        | BodySetAggregate
        | BodyTheoryAtom
        | BodyConditionalLiteral
    ]:
        """
        The body of the statement.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the statement.
        """

    @property
    def tuple(self) -> OptimizeTuple:
        """
        The tuple of the statement.
        """

class TermAbsolute:
    """
    A term representing the absolute operation.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        pool: typing.Iterable[
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ],
    ) -> None:
        """
        Construct a TermAbsolute object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the operation.
            pool: The argument pool.

                If there is more than one argument in the pool, the term is
                unpooled during preprocessing.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TermAbsolute | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TermAbsolute:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the operation.
        """

    @property
    def pool(
        self,
    ) -> typing.Sequence[
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ]:
        """
        The argument pool.
        If there is more than one argument in the pool, the term is unpooled during preprocessing.
        """

class TermBinaryOperation:
    """
    A term representing a binary operation.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        left: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
        operator_type: BinaryOperator,
        right: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
    ) -> None:
        """
        Construct a TermBinaryOperation object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the operation.
            left: The left argument of the operation.
            operator_type: The type of the operation.
            right: The right argument of the operation.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TermBinaryOperation | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TermBinaryOperation:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def left(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The left argument of the operation.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the operation.
        """

    @property
    def operator_type(self) -> BinaryOperator:
        """
        The type of the operation.
        """

    @property
    def right(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The right argument of the operation.
        """

class TermFormatString:
    """
    A term representing a format string.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        elements: typing.Iterable[FormatFieldLiteral | FormatFieldExpression],
    ) -> None:
        """
        Construct a TermFormatString object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the format string.
            elements: The elements of the format string.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TermFormatString | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TermFormatString:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def elements(self) -> typing.Sequence[FormatFieldLiteral | FormatFieldExpression]:
        """
        The elements of the format string.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the format string.
        """

class TermFunction:
    """
    A term representing a function.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: str,
        pool: typing.Iterable[ArgumentTuple],
        external: bool = False,
    ) -> None:
        """
        Construct a TermFunction object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the function.
            name: The name of the function.
            pool: The argument pool of the function.

                If there is more than one element in the pool, the term is
                unpooled during preprocessing.
            external: Whether the function is external.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TermFunction | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TermFunction:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def external(self) -> bool:
        """
        Whether the function is external.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the function.
        """

    @property
    def name(self) -> str:
        """
        The name of the function.
        """

    @property
    def pool(self) -> typing.Sequence[ArgumentTuple]:
        """
        The argument pool of the function.
        If there is more than one element in the pool, the term is unpooled during preprocessing.
        """

class TermSymbolic:
    """
    A term representing a symbol.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        symbol: clingo.symbol.Symbol,
    ) -> None:
        """
        Construct a TermSymbolic object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the symbol.
            symbol: The symbol.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TermSymbolic | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TermSymbolic:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the symbol.
        """

    @property
    def symbol(self) -> clingo.symbol.Symbol:
        """
        The symbol.
        """

class TermTuple:
    """
    A term representing a tuple.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        pool: typing.Iterable[
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
            | ArgumentTuple
        ],
    ) -> None:
        """
        Construct a TermTuple object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the tuple.
            pool: The argument pool of the tuple.

                If there is more than one element in the pool, the term is
                unpooled during preprocessing.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TermTuple | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TermTuple:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the tuple.
        """

    @property
    def pool(
        self,
    ) -> typing.Sequence[
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
        | ArgumentTuple
    ]:
        """
        The argument pool of the tuple.
        If there is more than one element in the pool, the term is unpooled during preprocessing.
        """

class TermUnaryOperation:
    """
    A term representing a unary operation.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        operator_type: UnaryOperator,
        right: (
            TermVariable
            | TermSymbolic
            | TermAbsolute
            | TermUnaryOperation
            | TermBinaryOperation
            | TermTuple
            | TermFunction
            | TermFormatString
        ),
    ) -> None:
        """
        Construct a TermUnaryOperation object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the operation.
            operator_type: The type of the operation.
            right: The argument of the operation.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TermUnaryOperation | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TermUnaryOperation:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the operation.
        """

    @property
    def operator_type(self) -> UnaryOperator:
        """
        The type of the operation.
        """

    @property
    def right(
        self,
    ) -> (
        TermVariable
        | TermSymbolic
        | TermAbsolute
        | TermUnaryOperation
        | TermBinaryOperation
        | TermTuple
        | TermFunction
        | TermFormatString
    ):
        """
        The argument of the operation.
        """

class TermVariable:
    """
    A term representing a variable.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: str,
        anonymous: bool = False,
    ) -> None:
        """
        Construct a TermVariable object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the variable.
            name: The name of the variable.
            anonymous: Whether the variable is anonymous.

                Anonymous variables receive a unique name during
                preprocessing.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TermVariable | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TermVariable:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def anonymous(self) -> bool:
        """
        Whether the variable is anonymous.
        Anonymous variables receive a unique name during preprocessing.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the variable.
        """

    @property
    def name(self) -> str:
        """
        The name of the variable.
        """

class TheoryAtomDefinition:
    """
    A theory atom definition.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: str,
        arity: int,
        term: str,
        guard: TheoryGuardDefinition | None,
        atom_type: TheoryAtomType,
    ) -> None:
        """
        Construct a TheoryAtomDefinition object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the definition.
            name: The name of the atom.
            arity: The arity of the atom.
            term: The name of a term definition.
            guard: An optional guard definition.
            atom_type: The type of the atom definition.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TheoryAtomDefinition | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TheoryAtomDefinition:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def arity(self) -> int:
        """
        The arity of the atom.
        """

    @property
    def atom_type(self) -> TheoryAtomType:
        """
        The type of the atom definition.
        """

    @property
    def guard(self) -> TheoryGuardDefinition | None:
        """
        An optional guard definition.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the definition.
        """

    @property
    def name(self) -> str:
        """
        The name of the atom.
        """

    @property
    def term(self) -> str:
        """
        The name of a term definition.
        """

class TheoryAtomElement:
    """
    An element of a theory atom elements.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        tuple: typing.Iterable[
            TheoryTermVariable
            | TheoryTermSymbolic
            | TheoryTermTuple
            | TheoryTermFunction
            | TheoryTermUnparsed
        ],
        condition: typing.Iterable[
            LiteralBoolean | LiteralComparison | LiteralSymbolic
        ],
    ) -> None:
        """
        Construct a TheoryAtomElement object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            tuple: The theory term tuple of the element.
            condition: The condition of the element.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TheoryAtomElement | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TheoryAtomElement:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def condition(
        self,
    ) -> typing.Sequence[LiteralBoolean | LiteralComparison | LiteralSymbolic]:
        """
        The condition of the element.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the element.
        """

    @property
    def tuple(
        self,
    ) -> typing.Sequence[
        TheoryTermVariable
        | TheoryTermSymbolic
        | TheoryTermTuple
        | TheoryTermFunction
        | TheoryTermUnparsed
    ]:
        """
        The theory term tuple of the element.
        """

class TheoryGuardDefinition:
    """
    A definition of a theory guard.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self, lib: clingo.core.Library, operators: typing.Iterable[str], term: str
    ) -> None:
        """
        Construct a TheoryGuardDefinition object.

        Args:
            lib: The library object for storing symbols.
            operators: A list of operator definition names.
            term: The name of a term definition.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TheoryGuardDefinition | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TheoryGuardDefinition:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def operators(self) -> typing.Sequence[str]:
        """
        A list of operator definition names.
        """

    @property
    def term(self) -> str:
        """
        The name of a term definition.
        """

class TheoryOperatorDefinition:
    """
    A theory operator definition.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: str,
        priority: int,
        operator_type: TheoryOperatorType,
    ) -> None:
        """
        Construct a TheoryOperatorDefinition object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the definition.
            name: The name of the definition.
            priority: The priority of the operator.
            operator_type: The type of the operator.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TheoryOperatorDefinition | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TheoryOperatorDefinition:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the definition.
        """

    @property
    def name(self) -> str:
        """
        The name of the definition.
        """

    @property
    def operator_type(self) -> TheoryOperatorType:
        """
        The type of the operator.
        """

    @property
    def priority(self) -> int:
        """
        The priority of the operator.
        """

class TheoryRightGuard:
    """
    A right hand side guard consisting of a theory operator and theory
    term.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        theory_operator: str,
        term: (
            TheoryTermVariable
            | TheoryTermSymbolic
            | TheoryTermTuple
            | TheoryTermFunction
            | TheoryTermUnparsed
        ),
    ) -> None:
        """
        Construct a TheoryRightGuard object.

        Args:
            lib: The library object for storing symbols.
            theory_operator: The operator of the guard.
            term: The theory term of the guard.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TheoryRightGuard | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TheoryRightGuard:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def term(
        self,
    ) -> (
        TheoryTermVariable
        | TheoryTermSymbolic
        | TheoryTermTuple
        | TheoryTermFunction
        | TheoryTermUnparsed
    ):
        """
        The theory term of the guard.
        """

    @property
    def theory_operator(self) -> str:
        """
        The operator of the guard.
        """

class TheoryTermDefinition:
    """
    A theory term definition.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: str,
        operators: typing.Iterable[TheoryOperatorDefinition],
    ) -> None:
        """
        Construct a TheoryTermDefinition object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the definition.
            name: The name of the definition.
            operators: The operator definitions to construct terms.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TheoryTermDefinition | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TheoryTermDefinition:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the definition.
        """

    @property
    def name(self) -> str:
        """
        The name of the definition.
        """

    @property
    def operators(self) -> typing.Sequence[TheoryOperatorDefinition]:
        """
        The operator definitions to construct terms.
        """

class TheoryTermFunction:
    """
    A theory term representing a function.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: str,
        arguments: typing.Iterable[
            TheoryTermVariable
            | TheoryTermSymbolic
            | TheoryTermTuple
            | TheoryTermFunction
            | TheoryTermUnparsed
        ],
    ) -> None:
        """
        Construct a TheoryTermFunction object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the function.
            name: The name of the function.
            arguments: The arguments of the function.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TheoryTermFunction | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TheoryTermFunction:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def arguments(
        self,
    ) -> typing.Sequence[
        TheoryTermVariable
        | TheoryTermSymbolic
        | TheoryTermTuple
        | TheoryTermFunction
        | TheoryTermUnparsed
    ]:
        """
        The arguments of the function.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the function.
        """

    @property
    def name(self) -> str:
        """
        The name of the function.
        """

class TheoryTermSymbolic:
    """
    A theory term representing a symbol.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        symbol: clingo.symbol.Symbol,
    ) -> None:
        """
        Construct a TheoryTermSymbolic object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the symbol.
            symbol: The symbol.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TheoryTermSymbolic | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TheoryTermSymbolic:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the symbol.
        """

    @property
    def symbol(self) -> clingo.symbol.Symbol:
        """
        The symbol.
        """

class TheoryTermTuple:
    """
    A theory term representing a tuple.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        tuple_type: TheoryTupleType,
        arguments: typing.Iterable[
            TheoryTermVariable
            | TheoryTermSymbolic
            | TheoryTermTuple
            | TheoryTermFunction
            | TheoryTermUnparsed
        ],
    ) -> None:
        """
        Construct a TheoryTermTuple object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the tuple.
            tuple_type: The type of the tuple.
            arguments: The arguments of the tuple.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TheoryTermTuple | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TheoryTermTuple:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def arguments(
        self,
    ) -> typing.Sequence[
        TheoryTermVariable
        | TheoryTermSymbolic
        | TheoryTermTuple
        | TheoryTermFunction
        | TheoryTermUnparsed
    ]:
        """
        The arguments of the tuple.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the tuple.
        """

    @property
    def tuple_type(self) -> TheoryTupleType:
        """
        The type of the tuple.
        """

class TheoryTermUnparsed:
    """
    A theory term representing an unparsed theory term.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        elements: typing.Iterable[UnparsedElement],
    ) -> None:
        """
        Construct a TheoryTermUnparsed object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the theory term.
            elements: The unparsed theory elements.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TheoryTermUnparsed | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TheoryTermUnparsed:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def elements(self) -> typing.Sequence[UnparsedElement]:
        """
        The unparsed theory elements.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the theory term.
        """

class TheoryTermVariable:
    """
    A theory term representing a variable.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        location: clingo.core.Location,
        name: str,
        anonymous: bool = False,
    ) -> None:
        """
        Construct a TheoryTermVariable object.

        Args:
            lib: The library object for storing symbols.
            location: The location of the variable.
            name: The name of the variable.
            anonymous: Whether the variable is anonymous.

                Anonymous variables receive a unique name during
                preprocessing.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> TheoryTermVariable | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> TheoryTermVariable:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def anonymous(self) -> bool:
        """
        Whether the variable is anonymous.
        Anonymous variables receive a unique name during preprocessing.
        """

    @property
    def location(self) -> clingo.core.Location:
        """
        The location of the variable.
        """

    @property
    def name(self) -> str:
        """
        The name of the variable.
        """

class UnparsedElement:
    """
    A list of unparsed theory terms and operators.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(
        self,
        lib: clingo.core.Library,
        operators: typing.Iterable[str],
        term: (
            TheoryTermVariable
            | TheoryTermSymbolic
            | TheoryTermTuple
            | TheoryTermFunction
            | TheoryTermUnparsed
        ),
    ) -> None:
        """
        Construct a UnparsedElement object.

        Args:
            lib: The library object for storing symbols.
            operators: The list of theory operators.
            term: The theory term.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str: ...
    def transform(
        self, lib: clingo.core.Library, transformer: typing.Any, *args, **kwargs
    ) -> UnparsedElement | None:
        """
        Transform the expression.

        Additional arguments are passed to the transformer.

        Args:
            lib: The library object for storing symbols.
            transformer: The transformer accepting the sub expressions.
        Returns:
            The transformed object or None.
        """

    def update(self, lib: clingo.core.Library, **kwargs) -> UnparsedElement:
        """
        Update the expression.

        Accepts keyword arguments with attributes to update.

        Args:
            lib: The library object for storing symbols.
        Returns:
            The updated object.
        """

    def visit(self, visitor: typing.Any, *args, **kwargs) -> None:
        """
        Visit the children of the expression.

        Args:
            visitor: The visitor accepting the sub expressions.
        """

    @property
    def operators(self) -> typing.Sequence[str]:
        """
        The list of theory operators.
        """

    @property
    def term(
        self,
    ) -> (
        TheoryTermVariable
        | TheoryTermSymbolic
        | TheoryTermTuple
        | TheoryTermFunction
        | TheoryTermUnparsed
    ):
        """
        The theory term.
        """

BodyLiteral = (
    BodySimpleLiteral
    | BodyAggregate
    | BodySort
    | BodySetAggregate
    | BodyTheoryAtom
    | BodyConditionalLiteral
)
DisjunctionElement = (
    LiteralBoolean | LiteralComparison | LiteralSymbolic | HeadConditionalLiteral
)
FormatField = FormatFieldLiteral | FormatFieldExpression
HeadLiteral = (
    HeadSimpleLiteral
    | HeadAggregate
    | HeadSetAggregate
    | HeadTheoryAtom
    | HeadDisjunction
)
Literal = LiteralBoolean | LiteralComparison | LiteralSymbolic
Statement = (
    StatementRule
    | StatementTheory
    | StatementOptimize
    | StatementWeakConstraint
    | StatementShow
    | StatementShowNothing
    | StatementShowSignature
    | StatementProject
    | StatementProjectSignature
    | StatementDefined
    | StatementExternal
    | StatementEdge
    | StatementHeuristic
    | StatementScript
    | StatementInclude
    | StatementProgram
    | StatementParts
    | StatementConst
    | StatementComment
)
Term = (
    TermVariable
    | TermSymbolic
    | TermAbsolute
    | TermUnaryOperation
    | TermBinaryOperation
    | TermTuple
    | TermFunction
    | TermFormatString
)
TermOrArgumentTuple = (
    TermVariable
    | TermSymbolic
    | TermAbsolute
    | TermUnaryOperation
    | TermBinaryOperation
    | TermTuple
    | TermFunction
    | TermFormatString
    | ArgumentTuple
)
TermOrProjection = (
    TermVariable
    | TermSymbolic
    | TermAbsolute
    | TermUnaryOperation
    | TermBinaryOperation
    | TermTuple
    | TermFunction
    | TermFormatString
    | Projection
)
TheoryTerm = (
    TheoryTermVariable
    | TheoryTermSymbolic
    | TheoryTermTuple
    | TheoryTermFunction
    | TheoryTermUnparsed
)
