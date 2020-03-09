"""
Tools to work with ASTs.
"""

from copy import copy
from itertools import chain, product

import clingo
from clingo import ast


def match(term, name, arity):
    """
    Match the given term if it is a function with signature `name/arity`.
    """
    return (term.type in (clingo.TheoryTermType.Function, clingo.TheoryTermType.Symbol) and
            term.name == name and
            len(term.arguments) == arity)


class Visitor(object):
    """
    Visit `clingo.ast.AST` objects by visiting all child nodes.

    Implement `visit_<type>` where `<type>` is the type of the nodes to be
    visited.
    """
    def visit_children(self, x, *args, **kwargs):
        """
        Visit all child nodes of the current node.
        """
        for key in x.child_keys:
            self.visit(getattr(x, key), *args, **kwargs)

    def visit_list(self, x, *args, **kwargs):
        """
        Visit a list of AST nodes.
        """
        for y in x:
            self.visit(y, *args, **kwargs)

    def visit_tuple(self, x, *args, **kwargs):
        """
        Visit a list of AST nodes.
        """
        for y in x:
            self.visit(y, *args, **kwargs)

    def visit_none(self, *args, **kwargs):
        """
        Visit none.

        This, is to handle optional arguments that do not have a visit method.
        """

    def visit(self, x, *args, **kwargs):
        """
        Default visit method to dispatch calls to child nodes.
        """
        if isinstance(x, ast.AST):
            attr = "visit_" + str(x.type)
            if hasattr(self, attr):
                return getattr(self, attr)(x, *args, **kwargs)
            return self.visit_children(x, *args, **kwargs)
        if isinstance(x, list):
            return self.visit_list(x, *args, **kwargs)
        if isinstance(x, tuple):
            return self.visit_tuple(x, *args, **kwargs)
        if x is None:
            return self.visit_none(x, *args, **kwargs)
        raise TypeError("unexpected type: {}".format(x))


class Transformer(Visitor):
    """
    Transforms `clingo.ast.AST` objects by visiting all child nodes.

    Implement `visit_<type>` where `<type>` is the type of the nodes to be
    visited.
    """

    def visit_children(self, x, *args, **kwargs):
        """
        Visit all child nodes of the current node.
        """
        copied = False
        for key in x.child_keys:
            y = getattr(x, key)
            z = self.visit(y, *args, **kwargs)
            if y is not z:
                if not copied:
                    copied = True
                    x = copy(x)
                setattr(x, key, z)
        return x

    def _seq(self, i, z, x, args, kwargs):
        for y in x[:i]:
            yield y
        yield z
        for y in x[i+1:]:
            yield self.visit(y, *args, **kwargs)

    def visit_list(self, x, *args, **kwargs):
        """
        Visit a list of AST nodes.
        """
        for i, y in enumerate(x):
            z = self.visit(y, *args, **kwargs)
            if y is not z:
                return list(self._seq(i, z, x, args, kwargs))
        return x

    def visit_tuple(self, x, *args, **kwargs):
        """
        Visit a tuple of AST nodes.
        """
        for i, y in enumerate(x):
            z = self.visit(y, *args, **kwargs)
            if y is not z:
                return tuple(self._seq(i, z, x, args, kwargs))
        return x


class _VarCollector(Visitor):
    """
    Helper to gather variables in an AST.
    """
    # pylint: disable=invalid-name

    def visit_Variable(self, x, variables):
        """
        Add variable x to variables.
        """
        variables[x.name] = x


def collect_variables(x):
    """
    Get all variables in the given AST.
    """
    variables = {}
    _VarCollector().visit(x, variables)
    return variables


def unpool_list_with(f, x):
    """
    Cross product of sequence x where each element of x is unpooled with f.
    """
    return product(*(f(y) for y in x))


class _TermUnpooler(Visitor):
    """
    Helper to unpool terms.
    """
    # pylint: disable=invalid-name

    def visit_Symbol(self, x):
        """
        Symbol(location: Location, symbol: clingo.Symbol)
        """
        return [x]

    def visit_Variable(self, x):
        """
        Variable(location: Location, name: str)
        """
        return [x]

    def visit_UnaryOperation(self, x):
        """
        UnaryOperation(location: Location, operator: UnaryOperator, argument: term)
        """
        return [ast.UnaryOperation(x.location, x.operator, y) for y in self.visit(x.term)]

    def visit_BinaryOperation(self, x):
        """
        BinaryOperation(location: Location, operator: BinaryOperator, left: term, right: term)
        """
        return [ast.BinaryOperation(x.location, x.operator, l, r) for l, r in product(self.visit(x.left), self.visit(x.right))]

    def visit_Interval(self, x):
        """
        Interval(location: Location, left: term, right: term)
        """
        return [ast.Interval(x.location, l, r) for l, r in product(self.visit(x.left), self.visit(x.right))]

    def visit_Function(self, x):
        """
        Function(location: Location, name: str, arguments: term*, external: bool)
        """
        return [ast.Function(x.location, x.name, args, x.external)
                for args in unpool_list_with(self.visit, x.arguments)]

    def visit_Pool(self, x):
        """
        Pool(location: Location, arguments: term*)
        """
        return list(chain.from_iterable(self.visit(y) for y in x.arguments))


def unpool_term(x):
    """
    Remove pools from the given term returning a list of terms.

    Care has to be taken when inplace modifying terms afterward because some
    terms might be shared if there are crossproducts. An implementation that
    always return shallow copies of modified objects, like the Transformer,
    will work without problems.
    """
    return _TermUnpooler().visit(x)


class _AtomUnpooler(Visitor):
    """
    Helper to unpool (simple) atoms.
    """
    # pylint: disable=invalid-name

    def visit_Comparison(self, x):
        """
        Unpool comparisons of form `l op r`.
        """
        return [ast.Comparison(x.comparison, l, r)
                for l, r in product(unpool_term(x.left), unpool_term(x.right))]

    def visit_BooleanConstant(self, x):
        """
        Unpool #true or #false.
        """
        return [x]

    def visit_SymbolicAtom(self, x):
        """
        Unpool atoms of form p(X).
        """
        return [ast.SymbolicAtom(y) for y in unpool_term(x.term)]


def unpool_literal(x):
    """
    Unpool a (simple) literal.

    See note in unpool_term.
    """
    if x.type != ast.ASTType.Literal:
        raise ValueError("literal expected")

    return [ast.Literal(x.location, x.sign, y) for y in _AtomUnpooler().visit(x.atom)]


def unpool_theory_atom(x):
    """
    Removes all pools from the conditions of theory atoms.

    See note in unpool_term.
    """
    x.elements = list(chain.from_iterable(
        (ast.TheoryAtomElement(element.tuple, condition)
         for condition in unpool_list_with(unpool_literal, element.condition))
        for element in x.elements))

    return ast.TheoryAtom(x.location, x.term, x.elements, x.guard)
