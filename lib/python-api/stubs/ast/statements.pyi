"""
AST nodes for statements.
"""

from collections.abc import Sequence
from enum import IntEnum
from typing import TypeAlias, final

from clingo.ast import AST
from clingo.ast.body import BodyLiteral
from clingo.ast.head import HeadLiteral
from clingo.ast.literals import Literal
from clingo.ast.terms import Term
from clingo.core import Library, Location
from clingo.symbol import Symbol

__all__: list[str] = [
    "CommentType",
    "Edge",
    "IncludeType",
    "OptimizeElement",
    "OptimizeTuple",
    "OptimizeType",
    "Precedence",
    "ProgramPart",
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
    "TheoryAtomDefinition",
    "TheoryAtomType",
    "TheoryGuardDefinition",
    "TheoryOperatorDefinition",
    "TheoryOperatorType",
    "TheoryTermDefinition",
]

class TheoryOperatorType(IntEnum):
    """
    Enumeration of theory operator types.
    """

    Unary = 0
    """
    A unary operator.
    """
    BinaryLeft = 1
    """
    A left-associative binary operator.
    """
    BinaryRight = 2
    """
    A right-associative binary operator.
    """

class TheoryAtomType(IntEnum):
    """
    Enumeration of theory atom types.
    """

    Head = 0
    """
    A theory atom that may only occur in rule heads.
    """
    Body = 1
    """
    A theory atom that may only occur in rule bodies.
    """
    Any = 2
    """
    A theory atom that may occur in rule heads and bodies.
    """
    Directive = 3
    """
    A theory atom that may only occur in the head of a rule with an empty body.
    """

class OptimizeType(IntEnum):
    """
    Enumeration of optimization types.
    """

    Minimize = 0
    """
    A `#minimize` statement.
    """
    Maximize = 1
    """
    A `#maximize` statement.
    """

class IncludeType(IntEnum):
    """
    Enumeration of include types.
    """

    System = 0
    """
    An include of a file, for example `#include "file.lp".`
    """
    Inbuild = 1
    """
    An include of a built-in program, for example `#include <incmode>.`
    """

class Precedence(IntEnum):
    """
    Enumeration of precedences of definitions.
    """

    Default = 0
    """
    A definition providing a default value (`[default]`).
    """
    Override = 1
    """
    A definition overriding a default value (`[override]`).
    """

class CommentType(IntEnum):
    """
    Enumeration of comment types.
    """

    Line = 0
    """
    A line comment starting with `%`.
    """
    Block = 1
    """
    A block comment enclosed in `%*` and `*%`.
    """

@final
class TheoryOperatorDefinition(AST):
    """
    A theory operator definition.

    For example, `- : 0, unary`.
    """

    __match_args__ = ("location", "name", "priority", "operator_type")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        name: str,
        priority: int,
        operator_type: TheoryOperatorType,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the definition.
            name: The operator.
            priority: The priority of the operator.
            operator_type: The type of the operator.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        name: str = ...,
        priority: int = ...,
        operator_type: TheoryOperatorType = ...,
    ) -> TheoryOperatorDefinition:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the definition.
            name: The operator.
            priority: The priority of the operator.
            operator_type: The type of the operator.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the definition.
        """

    @property
    def name(self) -> str:
        """
        The operator.
        """

    @property
    def priority(self) -> int:
        """
        The priority of the operator.
        """

    @property
    def operator_type(self) -> TheoryOperatorType:
        """
        The type of the operator.
        """

@final
class TheoryTermDefinition(AST):
    """
    A theory term definition.

    For example, `term { - : 0, unary }`.
    """

    __match_args__ = ("location", "name", "operators")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        name: str,
        operators: Sequence[TheoryOperatorDefinition],
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the definition.
            name: The name of the theory term definition.
            operators: The operator definitions of the theory term definition.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        name: str = ...,
        operators: Sequence[TheoryOperatorDefinition] = ...,
    ) -> TheoryTermDefinition:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the definition.
            name: The name of the theory term definition.
            operators: The operator definitions of the theory term definition.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the definition.
        """

    @property
    def name(self) -> str:
        """
        The name of the theory term definition.
        """

    @property
    def operators(self) -> Sequence[TheoryOperatorDefinition]:
        """
        The operator definitions of the theory term definition.
        """

@final
class TheoryGuardDefinition(AST):
    """
    A definition of the right guard of theory atoms.

    For example, `{<=, >=}, term` in `&sum/0: term, {<=, >=}, term, body`.
    """

    __match_args__ = ("operators", "term")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(self, lib: Library, operators: Sequence[str], term: str) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            operators: The theory operators allowed in the guard.
            term: The name of the theory term definition of the guard term.
        """

    def update(
        self, lib: Library, *, operators: Sequence[str] = ..., term: str = ...
    ) -> TheoryGuardDefinition:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            operators: The theory operators allowed in the guard.
            term: The name of the theory term definition of the guard term.

        Returns:
            The updated node.
        """

    @property
    def operators(self) -> Sequence[str]:
        """
        The theory operators allowed in the guard.
        """

    @property
    def term(self) -> str:
        """
        The name of the theory term definition of the guard term.
        """

@final
class TheoryAtomDefinition(AST):
    """
    A theory atom definition.

    For example, `&sum/0: term, {<=, >=}, term, body`.
    """

    __match_args__ = ("location", "name", "arity", "term", "guard", "atom_type")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        name: str,
        arity: int,
        term: str,
        guard: TheoryGuardDefinition | None,
        atom_type: TheoryAtomType,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the definition.
            name: The name of the atom.
            arity: The arity of the atom.
            term: The name of the theory term definition of the elements.
            guard: The optional definition of the right guard.
            atom_type: Where the theory atom may occur.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        name: str = ...,
        arity: int = ...,
        term: str = ...,
        guard: TheoryGuardDefinition | None = ...,
        atom_type: TheoryAtomType = ...,
    ) -> TheoryAtomDefinition:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the definition.
            name: The name of the atom.
            arity: The arity of the atom.
            term: The name of the theory term definition of the elements.
            guard: The optional definition of the right guard.
            atom_type: Where the theory atom may occur.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the definition.
        """

    @property
    def name(self) -> str:
        """
        The name of the atom.
        """

    @property
    def arity(self) -> int:
        """
        The arity of the atom.
        """

    @property
    def term(self) -> str:
        """
        The name of the theory term definition of the elements.
        """

    @property
    def guard(self) -> TheoryGuardDefinition | None:
        """
        The optional definition of the right guard.
        """

    @property
    def atom_type(self) -> TheoryAtomType:
        """
        Where the theory atom may occur.
        """

@final
class OptimizeTuple(AST):
    """
    The weight, priority, and terms of an optimization element.

    For example, `X@2,Y` in `#minimize { X@2,Y: p(X,Y) }`.
    """

    __match_args__ = ("weight", "priority", "terms")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, weight: Term, priority: Term | None, terms: Sequence[Term]
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            weight: The weight of the tuple.
            priority: The optional priority of the tuple.
            terms: The remaining terms in the tuple.
        """

    def update(
        self,
        lib: Library,
        *,
        weight: Term = ...,
        priority: Term | None = ...,
        terms: Sequence[Term] = ...,
    ) -> OptimizeTuple:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            weight: The weight of the tuple.
            priority: The optional priority of the tuple.
            terms: The remaining terms in the tuple.

        Returns:
            The updated node.
        """

    @property
    def weight(self) -> Term:
        """
        The weight of the tuple.
        """

    @property
    def priority(self) -> Term | None:
        """
        The optional priority of the tuple.
        """

    @property
    def terms(self) -> Sequence[Term]:
        """
        The remaining terms in the tuple.
        """

@final
class OptimizeElement(AST):
    """
    An element of an optimization statement.

    For example, `X@2,Y: p(X,Y)`.
    """

    __match_args__ = ("tuple", "condition")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, tuple: OptimizeTuple, condition: Sequence[Literal]
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            tuple: The tuple of the element.
            condition: The condition of the element.
        """

    def update(
        self,
        lib: Library,
        *,
        tuple: OptimizeTuple = ...,
        condition: Sequence[Literal] = ...,
    ) -> OptimizeElement:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            tuple: The tuple of the element.
            condition: The condition of the element.

        Returns:
            The updated node.
        """

    @property
    def tuple(self) -> OptimizeTuple:
        """
        The tuple of the element.
        """

    @property
    def condition(self) -> Sequence[Literal]:
        """
        The condition of the element.
        """

@final
class Edge(AST):
    """
    An edge of an edge statement.

    For example, `X,Y` in `#edge (X,Y): e(X,Y).`
    """

    __match_args__ = ("u", "v")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(self, lib: Library, u: Term, v: Term) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            u: The source vertex.
            v: The target vertex.
        """

    def update(self, lib: Library, *, u: Term = ..., v: Term = ...) -> Edge:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            u: The source vertex.
            v: The target vertex.

        Returns:
            The updated node.
        """

    @property
    def u(self) -> Term:
        """
        The source vertex.
        """

    @property
    def v(self) -> Term:
        """
        The target vertex.
        """

@final
class ProgramPart(AST):
    """
    A program part to ground.

    For example, `step(1)` in `#parts step(1).`
    """

    __match_args__ = ("name", "arguments")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(self, lib: Library, name: str, arguments: Sequence[Symbol]) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            name: The name of the program part.
            arguments: The arguments of the program part.
        """

    def update(
        self, lib: Library, *, name: str = ..., arguments: Sequence[Symbol] = ...
    ) -> ProgramPart:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            name: The name of the program part.
            arguments: The arguments of the program part.

        Returns:
            The updated node.
        """

    @property
    def name(self) -> str:
        """
        The name of the program part.
        """

    @property
    def arguments(self) -> Sequence[Symbol]:
        """
        The arguments of the program part.
        """

@final
class StatementRule(AST):
    """
    A rule.

    For example, `p(X) :- q(X).` Facts have an empty body and integrity constraints an empty disjunction as head.
    """

    __match_args__ = ("location", "head", "body")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        head: HeadLiteral,
        body: Sequence[BodyLiteral],
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            head: The head of the rule.
            body: The body of the rule.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        head: HeadLiteral = ...,
        body: Sequence[BodyLiteral] = ...,
    ) -> StatementRule:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            head: The head of the rule.
            body: The body of the rule.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def head(self) -> HeadLiteral:
        """
        The head of the rule.
        """

    @property
    def body(self) -> Sequence[BodyLiteral]:
        """
        The body of the rule.
        """

@final
class StatementTheory(AST):
    """
    A theory definition.

    For example, `#theory t { term { - : 0, unary }; &a/0: term, body }.`
    """

    __match_args__ = ("location", "name", "terms", "atoms")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        name: str,
        terms: Sequence[TheoryTermDefinition],
        atoms: Sequence[TheoryAtomDefinition],
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the theory.
            terms: The theory term definitions.
            atoms: The theory atom definitions.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        name: str = ...,
        terms: Sequence[TheoryTermDefinition] = ...,
        atoms: Sequence[TheoryAtomDefinition] = ...,
    ) -> StatementTheory:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the theory.
            terms: The theory term definitions.
            atoms: The theory atom definitions.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def name(self) -> str:
        """
        The name of the theory.
        """

    @property
    def terms(self) -> Sequence[TheoryTermDefinition]:
        """
        The theory term definitions.
        """

    @property
    def atoms(self) -> Sequence[TheoryAtomDefinition]:
        """
        The theory atom definitions.
        """

@final
class StatementOptimize(AST):
    """
    An optimization statement.

    For example, `#minimize { X@2,Y: p(X,Y) }.`
    """

    __match_args__ = ("location", "elements", "optimize_type")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        elements: Sequence[OptimizeElement],
        optimize_type: OptimizeType,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            elements: The elements of the statement.
            optimize_type: Whether to minimize or maximize.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        elements: Sequence[OptimizeElement] = ...,
        optimize_type: OptimizeType = ...,
    ) -> StatementOptimize:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            elements: The elements of the statement.
            optimize_type: Whether to minimize or maximize.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def elements(self) -> Sequence[OptimizeElement]:
        """
        The elements of the statement.
        """

    @property
    def optimize_type(self) -> OptimizeType:
        """
        Whether to minimize or maximize.
        """

@final
class StatementWeakConstraint(AST):
    """
    A weak constraint.

    For example, `:~ p(X). [X@1]`.
    """

    __match_args__ = ("location", "body", "tuple")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        body: Sequence[BodyLiteral],
        tuple: OptimizeTuple,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            body: The body of the weak constraint.
            tuple: The weight, priority, and terms of the weak constraint.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        body: Sequence[BodyLiteral] = ...,
        tuple: OptimizeTuple = ...,
    ) -> StatementWeakConstraint:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            body: The body of the weak constraint.
            tuple: The weight, priority, and terms of the weak constraint.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def body(self) -> Sequence[BodyLiteral]:
        """
        The body of the weak constraint.
        """

    @property
    def tuple(self) -> OptimizeTuple:
        """
        The weight, priority, and terms of the weak constraint.
        """

@final
class StatementShow(AST):
    """
    A show statement for terms.

    For example, `#show X: p(X).`
    """

    __match_args__ = ("location", "term", "body")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, term: Term, body: Sequence[BodyLiteral]
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            term: The term to show.
            body: The condition of the statement.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        term: Term = ...,
        body: Sequence[BodyLiteral] = ...,
    ) -> StatementShow:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            term: The term to show.
            body: The condition of the statement.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def term(self) -> Term:
        """
        The term to show.
        """

    @property
    def body(self) -> Sequence[BodyLiteral]:
        """
        The condition of the statement.
        """

@final
class StatementShowNothing(AST):
    """
    A show statement hiding all atoms.

    For example, `#show.`
    """

    __match_args__ = ("location",)
    """
    The names of the arguments for positional patterns.
    """
    def __init__(self, lib: Library, location: Location) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
        """

    def update(self, lib: Library, *, location: Location = ...) -> StatementShowNothing:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

@final
class StatementShowSignature(AST):
    """
    A show statement for a signature.

    For example, `#show p/1.`
    """

    __match_args__ = ("location", "name", "arity", "sign", "value")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        name: str,
        arity: int,
        sign: bool = False,
        value: bool = True,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the signature.
            arity: The arity of the signature.
            sign: Whether the signature is classically negated, for example, in `#show -p/1.`
            value: Whether to show or hide the atoms over the signature.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        name: str = ...,
        arity: int = ...,
        sign: bool = ...,
        value: bool = ...,
    ) -> StatementShowSignature:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the signature.
            arity: The arity of the signature.
            sign: Whether the signature is classically negated, for example, in `#show -p/1.`
            value: Whether to show or hide the atoms over the signature.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def name(self) -> str:
        """
        The name of the signature.
        """

    @property
    def arity(self) -> int:
        """
        The arity of the signature.
        """

    @property
    def sign(self) -> bool:
        """
        Whether the signature is classically negated, for example, in `#show -p/1.`
        """

    @property
    def value(self) -> bool:
        """
        Whether to show or hide the atoms over the signature.
        """

@final
class StatementProject(AST):
    """
    A project statement for atoms.

    For example, `#project p(X): q(X).`
    """

    __match_args__ = ("location", "atom", "body")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, atom: Term, body: Sequence[BodyLiteral]
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            atom: The atom to project on.
            body: The condition of the statement.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        atom: Term = ...,
        body: Sequence[BodyLiteral] = ...,
    ) -> StatementProject:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            atom: The atom to project on.
            body: The condition of the statement.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def atom(self) -> Term:
        """
        The atom to project on.
        """

    @property
    def body(self) -> Sequence[BodyLiteral]:
        """
        The condition of the statement.
        """

@final
class StatementProjectSignature(AST):
    """
    A project statement for a signature.

    For example, `#project p/1.`
    """

    __match_args__ = ("location", "name", "arity", "sign")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        name: str,
        arity: int,
        sign: bool = False,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the signature.
            arity: The arity of the signature.
            sign: Whether the signature is classically negated.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        name: str = ...,
        arity: int = ...,
        sign: bool = ...,
    ) -> StatementProjectSignature:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the signature.
            arity: The arity of the signature.
            sign: Whether the signature is classically negated.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def name(self) -> str:
        """
        The name of the signature.
        """

    @property
    def arity(self) -> int:
        """
        The arity of the signature.
        """

    @property
    def sign(self) -> bool:
        """
        Whether the signature is classically negated.
        """

@final
class StatementDefined(AST):
    """
    A statement declaring a predicate as defined.

    For example, `#defined p/1.`
    """

    __match_args__ = ("location", "name", "arity", "sign")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        name: str,
        arity: int,
        sign: bool = False,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the signature.
            arity: The arity of the signature.
            sign: Whether the signature is classically negated.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        name: str = ...,
        arity: int = ...,
        sign: bool = ...,
    ) -> StatementDefined:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the signature.
            arity: The arity of the signature.
            sign: Whether the signature is classically negated.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def name(self) -> str:
        """
        The name of the signature.
        """

    @property
    def arity(self) -> int:
        """
        The arity of the signature.
        """

    @property
    def sign(self) -> bool:
        """
        Whether the signature is classically negated.
        """

@final
class StatementExternal(AST):
    """
    An external statement.

    For example, `#external p(X): q(X). [true]`.
    """

    __match_args__ = ("location", "atom", "body", "external_type")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        atom: Term,
        body: Sequence[BodyLiteral],
        external_type: Term | None = None,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            atom: The external atom.
            body: The condition of the statement.
            external_type: The optional initial value of the external atom, for example, `true` or `free`.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        atom: Term = ...,
        body: Sequence[BodyLiteral] = ...,
        external_type: Term | None = ...,
    ) -> StatementExternal:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            atom: The external atom.
            body: The condition of the statement.
            external_type: The optional initial value of the external atom, for example, `true` or `free`.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def atom(self) -> Term:
        """
        The external atom.
        """

    @property
    def body(self) -> Sequence[BodyLiteral]:
        """
        The condition of the statement.
        """

    @property
    def external_type(self) -> Term | None:
        """
        The optional initial value of the external atom, for example, `true` or `free`.
        """

@final
class StatementEdge(AST):
    """
    An edge statement.

    For example, `#edge (X,Y): e(X,Y).`
    """

    __match_args__ = ("location", "pool", "body")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        pool: Sequence[Edge],
        body: Sequence[BodyLiteral],
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            pool: The pool of edges.
            body: The condition of the statement.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        pool: Sequence[Edge] = ...,
        body: Sequence[BodyLiteral] = ...,
    ) -> StatementEdge:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            pool: The pool of edges.
            body: The condition of the statement.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def pool(self) -> Sequence[Edge]:
        """
        The pool of edges.
        """

    @property
    def body(self) -> Sequence[BodyLiteral]:
        """
        The condition of the statement.
        """

@final
class StatementHeuristic(AST):
    """
    A heuristic statement.

    For example, `#heuristic p(X): q(X). [X@1,level]`.
    """

    __match_args__ = ("location", "atom", "body", "weight", "modifier", "priority")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        atom: Term,
        body: Sequence[BodyLiteral],
        weight: Term,
        modifier: Term,
        priority: Term | None = None,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            atom: The atom to heuristically modify.
            body: The condition of the statement.
            weight: The weight of the modification.
            modifier: The modifier, for example, `level` or `sign`.
            priority: The optional priority of the modification.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        atom: Term = ...,
        body: Sequence[BodyLiteral] = ...,
        weight: Term = ...,
        modifier: Term = ...,
        priority: Term | None = ...,
    ) -> StatementHeuristic:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            atom: The atom to heuristically modify.
            body: The condition of the statement.
            weight: The weight of the modification.
            modifier: The modifier, for example, `level` or `sign`.
            priority: The optional priority of the modification.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def atom(self) -> Term:
        """
        The atom to heuristically modify.
        """

    @property
    def body(self) -> Sequence[BodyLiteral]:
        """
        The condition of the statement.
        """

    @property
    def weight(self) -> Term:
        """
        The weight of the modification.
        """

    @property
    def modifier(self) -> Term:
        """
        The modifier, for example, `level` or `sign`.
        """

    @property
    def priority(self) -> Term | None:
        """
        The optional priority of the modification.
        """

@final
class StatementScript(AST):
    """
    A script statement.

    For example, `#script (python) ... #end.`
    """

    __match_args__ = ("location", "value", "script_type")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, value: str, script_type: str
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            value: The code of the script.
            script_type: The language of the script, for example, `python` or `lua`.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        value: str = ...,
        script_type: str = ...,
    ) -> StatementScript:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            value: The code of the script.
            script_type: The language of the script, for example, `python` or `lua`.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def value(self) -> str:
        """
        The code of the script.
        """

    @property
    def script_type(self) -> str:
        """
        The language of the script, for example, `python` or `lua`.
        """

@final
class StatementProgram(AST):
    """
    A program statement.

    For example, `#program step(t).`
    """

    __match_args__ = ("location", "name", "arguments")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, name: str, arguments: Sequence[str]
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the program part.
            arguments: The parameters of the program part.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        name: str = ...,
        arguments: Sequence[str] = ...,
    ) -> StatementProgram:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the program part.
            arguments: The parameters of the program part.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def name(self) -> str:
        """
        The name of the program part.
        """

    @property
    def arguments(self) -> Sequence[str]:
        """
        The parameters of the program part.
        """

@final
class StatementInclude(AST):
    """
    An include statement.

    For example, `#include "file.lp".` or `#include <incmode>.`
    """

    __match_args__ = ("location", "value", "include_type")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, value: str, include_type: IncludeType
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            value: The file or built-in program to include.
            include_type: Whether a file or a built-in program is included.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        value: str = ...,
        include_type: IncludeType = ...,
    ) -> StatementInclude:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            value: The file or built-in program to include.
            include_type: Whether a file or a built-in program is included.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def value(self) -> str:
        """
        The file or built-in program to include.
        """

    @property
    def include_type(self) -> IncludeType:
        """
        Whether a file or a built-in program is included.
        """

@final
class StatementConst(AST):
    """
    A constant definition.

    For example, `#const n=10.`
    """

    __match_args__ = ("location", "name", "value", "precedence")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        name: str,
        value: Term,
        precedence: Precedence,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the constant.
            value: The value of the constant.
            precedence: Whether the definition provides or overrides a default value.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        name: str = ...,
        value: Term = ...,
        precedence: Precedence = ...,
    ) -> StatementConst:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            name: The name of the constant.
            value: The value of the constant.
            precedence: Whether the definition provides or overrides a default value.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def name(self) -> str:
        """
        The name of the constant.
        """

    @property
    def value(self) -> Term:
        """
        The value of the constant.
        """

    @property
    def precedence(self) -> Precedence:
        """
        Whether the definition provides or overrides a default value.
        """

@final
class StatementParts(AST):
    """
    A statement selecting the program parts to ground.

    For example, `#parts base, step(1).`
    """

    __match_args__ = ("location", "elements", "precedence")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        elements: Sequence[ProgramPart],
        precedence: Precedence,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            elements: The program parts to ground.
            precedence: Whether the statement provides or overrides a default value.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        elements: Sequence[ProgramPart] = ...,
        precedence: Precedence = ...,
    ) -> StatementParts:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the statement.
            elements: The program parts to ground.
            precedence: Whether the statement provides or overrides a default value.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the statement.
        """

    @property
    def elements(self) -> Sequence[ProgramPart]:
        """
        The program parts to ground.
        """

    @property
    def precedence(self) -> Precedence:
        """
        Whether the statement provides or overrides a default value.
        """

@final
class StatementComment(AST):
    """
    A comment.

    For example, `% comment` or `%* comment *%`.
    """

    __match_args__ = ("location", "value", "comment_type")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, value: str, comment_type: CommentType
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the comment.
            value: The text of the comment including the comment markers.
            comment_type: The type of the comment.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        value: str = ...,
        comment_type: CommentType = ...,
    ) -> StatementComment:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the comment.
            value: The text of the comment including the comment markers.
            comment_type: The type of the comment.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the comment.
        """

    @property
    def value(self) -> str:
        """
        The text of the comment including the comment markers.
        """

    @property
    def comment_type(self) -> CommentType:
        """
        The type of the comment.
        """

Statement: TypeAlias = (
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
"""
The available statements.
"""
