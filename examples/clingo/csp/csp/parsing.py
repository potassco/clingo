"""
This module contains functions for parsing and normalizing constraints.
"""

from copy import copy
from abc import abstractmethod
from functools import reduce  # pylint: disable=redefined-builtin

import clingo
from clingo import ast
from csp.util import gcd, ABC, abstractproperty
from csp.astutil import match, Transformer, unpool_theory_atom, collect_variables


THEORY = """\
#theory cp {
    var_term  { };
    sum_term {
    -  : 3, unary;
    ** : 2, binary, right;
    *  : 1, binary, left;
    /  : 1, binary, left;
    \\ : 1, binary, left;
    +  : 0, binary, left;
    -  : 0, binary, left
    };
    dom_term {
    -  : 4, unary;
    ** : 3, binary, right;
    *  : 2, binary, left;
    /  : 2, binary, left;
    \\ : 2, binary, left;
    +  : 1, binary, left;
    -  : 1, binary, left;
    .. : 0, binary, left
    };
    &sum/1 : sum_term, {<=,=,!=,<,>,>=}, sum_term, any;
    &diff/1 : sum_term, {<=}, sum_term, any;
    &minimize/0 : sum_term, directive;
    &maximize/0 : sum_term, directive;
    &show/0 : sum_term, directive;
    &distinct/0 : sum_term, head;
    &dom/0 : dom_term, {=}, var_term, head
}.
"""


class AbstractConstraintBuilder(ABC):
    """
    CSP builder to use with the parse_theory function.
    """

    @abstractproperty
    def cc(self):
        """
        Return a ClauseCreator.
        """

    @abstractmethod
    def add_show(self):
        """
        Inform the builder that there is a show statement.
        """

    @abstractmethod
    def show_signature(self, name, arity):
        """
        Show variables with the given signature.
        """

    @abstractmethod
    def show_variable(self, var):
        """
        Show the given variable.
        """

    @abstractmethod
    def add_variable(self, var):
        """
        Get the integer representing a variable.
        """

    @abstractmethod
    def add_constraint(self, lit, elems, rhs, strict):
        """
        Add a constraint.
        """

    @abstractmethod
    def add_minimize(self, co, var):
        """
        Add a term to the minimize constraint.
        """

    @abstractmethod
    def add_distinct(self, literal, elems):
        """
        Add a distinct constraint.
        """

    @abstractmethod
    def add_dom(self, literal, var, elements):
        """
        Add a domain for the given variable.
        """


def parse_theory(builder, theory_atoms):
    """
    Parse the atoms in the given theory and pass them to the builder.
    """
    for atom in theory_atoms:
        is_sum = match(atom.term, "sum", 1)
        is_diff = match(atom.term, "diff", 1)
        if is_sum or is_diff:
            body = match(atom.term.arguments[0], "body", 0)
            _parse_constraint(builder, atom, is_sum, body)
        elif match(atom.term, "distinct", 0):
            _parse_distinct(builder, atom)
        elif match(atom.term, "show", 0):
            _parse_show(builder, atom)
        elif match(atom.term, "dom", 0):
            _parse_dom(builder, atom)
        elif match(atom.term, "minimize", 0):
            _parse_objective(builder, atom, 1)
        elif match(atom.term, "maximize", 0):
            _parse_objective(builder, atom, -1)


def _parse_objective(builder, atom, factor):
    """
    Parses minimize and maximize directives.
    """
    assert factor in (1, -1)
    for co, var in _parse_constraint_elems(builder, atom.elements, None, True):
        builder.add_minimize(factor * co, var)


def simplify(seq, drop_zero):
    """
    Combine coefficients of terms with the same variable and optionally drop
    zero weights and sum up terms without a variable.
    """
    elements = []
    seen = {}
    rhs = 0
    for co, var in seq:
        if co == 0:
            continue

        if var is None:
            rhs -= co
        elif var not in seen:
            seen[var] = len(elements)
            elements.append((co, var))
        else:
            co_old, var_old = elements[seen[var]]
            assert var_old == var
            elements[seen[var]] = (co_old + co, var)

    if drop_zero:
        elements = [(co, var) for co, var in elements if co != 0]

    return rhs, elements


def _parse_show(builder, atom):
    builder.add_show()

    for elem in atom.elements:
        if len(elem.terms) == 1 and not elem.condition:
            _parse_show_elem(builder, elem.terms[0])
        else:
            raise RuntimeError("Invalid Syntax")


def _parse_show_elem(builder, term):
    if match(term, "/", 2):
        a = _evaluate_term(term.arguments[0])
        if a.type != clingo.SymbolType.Function or a.arguments:
            raise RuntimeError("Invalid Syntax")

        b = _evaluate_term(term.arguments[1])
        if b.type != clingo.SymbolType.Number:
            raise RuntimeError("Invalid Syntax")

        builder.show_signature(a.name, b.number)
    else:
        a = _evaluate_term(term)
        if a.type == clingo.SymbolType.Number:
            raise RuntimeError("Invalid Syntax")

        builder.show_variable(a)


def _parse_dom(builder, atom):
    elements = []
    for elem in atom.elements:
        if len(elem.terms) == 1 and not elem.condition:
            elements.append(_parse_dom_elem(elem.terms[0]))
        else:
            raise RuntimeError("Invalid Syntax")

    var = _evaluate_term(atom.guard[1])
    if var.type == clingo.SymbolType.Number:
        raise RuntimeError("Invalid Syntax")

    builder.add_dom(builder.cc.solver_literal(atom.literal), builder.add_variable(var), elements)


def _parse_dom_elem(term):
    if match(term, "..", 2):
        a = _evaluate_term(term.arguments[0])
        if a.type != clingo.SymbolType.Number:
            raise RuntimeError("Invalid Syntax")

        b = _evaluate_term(term.arguments[1])
        if b.type != clingo.SymbolType.Number:
            raise RuntimeError("Invalid Syntax")

        return (a.number, b.number+1)

    a = _evaluate_term(term)
    if a.type != clingo.SymbolType.Number:
        raise RuntimeError("Invalid Syntax")

    return (a.number, a.number+1)


def _parse_distinct(builder, atom):
    """
    Currently only distinct constraints in the head are supported. Supporting
    them in the body would also be possible where they should be strict.
    """

    elements = []
    for elem in atom.elements:
        if elem.terms and not elem.condition:
            elements.append(simplify(_parse_constraint_elem(builder, elem.terms[0], True), False))
        else:
            raise RuntimeError("Invalid Syntax")

    builder.add_distinct(builder.cc.solver_literal(atom.literal), elements)


def _parse_constraint(builder, atom, is_sum, strict):
    """
    Adds constraints from the given theory atom to the builder.

    If `is_sum` is true parses a sum constraint. Otherwise, it parses a
    difference constraint as supported by clingo-dl.

    Contraints are represented as a triple of a literal, its elements, and an
    upper bound.
    """

    elements = []
    rhs = 0

    # map literal
    literal = builder.cc.solver_literal(atom.literal)

    # combine coefficients
    rhs, elements = simplify(_parse_constraint_elems(builder, atom.elements, atom.guard[1], is_sum), True)

    # divide by gcd
    d = reduce(lambda a, b: gcd(a, b[0]), elements, rhs)
    if d > 1:
        elements = [(co//d, var) for co, var in elements]
        rhs //= d

    _normalize_constraint(builder, literal, elements, atom.guard[0], rhs, strict)


def _normalize_constraint(builder, literal, elements, op, rhs, strict):
    # rerwrite > and < guard
    if op == ">":
        op = ">="
        rhs = rhs + 1
    elif op == "<":
        op = "<="
        rhs -= 1

    # rewrite >= guard
    if op == ">=":
        op = "<="
        rhs = -rhs
        elements = [(-co, var) for co, var in elements]

    if op == "<=":
        if strict and len(elements) == 1:
            builder.add_constraint(literal, elements, rhs, True)
            return
        builder.add_constraint(literal, elements, rhs, False)

    elif op == "=":
        if strict:
            if builder.cc.assignment.is_true(literal):
                a = b = 1
            else:
                a = builder.cc.add_literal()
                b = builder.cc.add_literal()

            # Note: this cannot fail because constraint normalization does not propagate
            builder.cc.add_clause([-literal, a])
            builder.cc.add_clause([-literal, b])
            builder.cc.add_clause([-a, -b, literal])
        else:
            a = b = literal

        _normalize_constraint(builder, a, elements, "<=", rhs, strict)
        _normalize_constraint(builder, b, elements, ">=", rhs, strict)

        if strict:
            return

    elif op == "!=":
        if strict:
            _normalize_constraint(builder, -literal, elements, "=", rhs, True)
            return

        a = builder.cc.add_literal()
        b = builder.cc.add_literal()

        # Note: this cannot fail
        builder.cc.add_clause([a, b, -literal])
        builder.cc.add_clause([-a, -b])

        _normalize_constraint(builder, a, elements, "<", rhs, False)
        _normalize_constraint(builder, b, elements, ">", rhs, False)

    if strict:
        if op == "<=":
            op = ">"
        elif op == "!=":
            op = "="

        _normalize_constraint(builder, -literal, elements, op, rhs, False)


def _parse_constraint_elems(builder, elems, rhs, is_sum):
    if not is_sum and len(elems) != 1:
        raise RuntimeError("Invalid Syntax")

    for elem in elems:
        if elem.terms and not elem.condition:
            for co, var in _parse_constraint_elem(builder, elem.terms[0], is_sum):
                yield co, var
        else:
            raise RuntimeError("Invalid Syntax")

    if is_sum:
        if rhs is not None:
            for co, var in _parse_constraint_elem(builder, rhs, is_sum):
                yield -co, var
    else:
        term = _evaluate_term(rhs)
        if term.type == clingo.SymbolType.Number:
            yield -term.number, None
        else:
            raise RuntimeError("Invalid Syntax")


def _parse_constraint_elem(builder, term, is_sum):
    assert term is not None
    if not is_sum:
        if match(term, "-", 2):
            a = _evaluate_term(term.arguments[0])
            if a.type == clingo.SymbolType.Number:
                yield a.number, None
            else:
                yield 1, builder.add_variable(a)

            b = _evaluate_term(term.arguments[1])
            if b.type == clingo.SymbolType.Number:
                yield -b.number, None
            else:
                yield -1, builder.add_variable(b)

        else:
            raise RuntimeError("Invalid Syntax for difference constraint")

    else:
        if match(term, "+", 2):
            for co, var in _parse_constraint_elem(builder, term.arguments[0], True):
                yield co, var
            for co, var in _parse_constraint_elem(builder, term.arguments[1], True):
                yield co, var

        elif match(term, "-", 2):
            for co, var in _parse_constraint_elem(builder, term.arguments[0], True):
                yield co, var
            for co, var in _parse_constraint_elem(builder, term.arguments[1], True):
                yield -co, var

        elif match(term, "*", 2):
            lhs = list(_parse_constraint_elem(builder, term.arguments[0], True))
            for co_prime, var_prime in _parse_constraint_elem(builder, term.arguments[1], True):
                for co, var in lhs:
                    if var is None:
                        yield co*co_prime, var_prime
                    elif var_prime is None:
                        yield co*co_prime, var
                    else:
                        raise RuntimeError("Invalid Syntax, only linear constraints allowed")

        elif match(term, "-", 1):
            for co, var in _parse_constraint_elem(builder, term.arguments[0], True):
                yield -co, var

        elif match(term, "+", 1):
            for co, var in _parse_constraint_elem(builder, term.arguments[0], True):
                yield co, var

        elif term.type == clingo.TheoryTermType.Number:
            yield term.number, None

        elif term.type in (clingo.TheoryTermType.Symbol, clingo.TheoryTermType.Function, clingo.TheoryTermType.Tuple):
            yield 1, builder.add_variable(_evaluate_term(term))

        else:
            raise RuntimeError("Invalid Syntax for linear constraint")


_BOP = {"+": lambda a, b: a+b,
        "-": lambda a, b: a-b,
        "*": lambda a, b: a*b,
        "**": lambda a, b: a**b,
        "\\": lambda a, b: a%b,
        "/": lambda a, b: a//b}


def _evaluate_term(term):
    # pylint: disable=too-many-return-statements

    # tuples
    if term.type == clingo.TheoryTermType.Tuple:
        return clingo.Tuple(_evaluate_term(x) for x in term.arguments)

    # functions and arithmetic operations
    if term.type == clingo.TheoryTermType.Function:
        # binary operations
        if term.name in _BOP and len(term.arguments) == 2:
            a = _evaluate_term(term.arguments[0])
            b = _evaluate_term(term.arguments[1])

            if a.type != clingo.SymbolType.Number or b.type != clingo.SymbolType.Number:
                raise RuntimeError("Invalid Binary Operation")

            if term.name in ("/", "\\") and b.number == 0:
                raise RuntimeError("Division by Zero")

            return clingo.Number(_BOP[term.name](a.number, b.number))

        # unary operations
        if term.name == "-" and len(term.arguments) == 1:
            a = _evaluate_term(term.arguments[0])

            if a.type == clingo.SymbolType.Number:
                return clingo.Number(-a.number)

            if a.type == clingo.SymbolType.Function and a.name:
                return clingo.Function(a.name, a.arguments, not a.positive)

            raise RuntimeError("Invalid Unary Operation")

        # invalid operators
        if term.name == ".." and len(term.arguments) == 2:
            raise RuntimeError("Invalid Interval")

        # functions
        return clingo.Function(term.name, (_evaluate_term(x) for x in term.arguments))

    # constants
    if term.type == clingo.TheoryTermType.Symbol:
        return clingo.Function(term.name)

    # numbers
    if term.type == clingo.TheoryTermType.Number:
        return clingo.Number(term.number)

    raise RuntimeError("Invalid Syntax")


def _negate_relation(name):
    if name == "=":
        return "!="
    if name == "!=":
        return "="
    if name == "<":
        return ">="
    if name == "<=":
        return ">"
    if name == ">=":
        return "<"
    if name == ">":
        return "<="
    raise RuntimeError("unknown relation")


class HeadBodyTransformer(Transformer):
    """
    Transforms sum/diff theory atoms in heads and bodies of rules by turning
    the name of each theory atom into a function with head or body as argument.
    Shift constraints into heads of integrity constraints. And make sure that
    diff, sum, and distinct constraints are treated as multisets.
    """
    # pylint: disable=invalid-name

    def __init__(self, shift):
        self._shift = shift

    def visit_Rule(self, rule):
        """
        Shift constraints in integrity constraints and visit head/body
        literals.
        """
        # Note: This implements clingcon's don't care propagation. We can shift
        # one constraint from the body of an integrity constraint to the head
        # of a rule. This way the constraint is no longer strict and can be
        # represented internally with less constraints.
        head = rule.head
        body = rule.body
        if self._shift and head.type == ast.ASTType.Literal and head.atom.type == ast.ASTType.BooleanConstant and not head.atom.value:
            for literal in body:
                if literal.type == ast.ASTType.Literal and literal.atom.type == ast.ASTType.TheoryAtom:
                    atom = literal.atom
                    term = atom.term
                    if term.name in ["sum", "diff"] and not term.arguments:
                        body = copy(body)
                        body.remove(literal)
                        if literal.sign != ast.Sign.Negation:
                            atom = copy(atom)
                            atom.guard = copy(atom.guard)
                            atom.guard.operator_name = _negate_relation(atom.guard.operator_name)
                        head = atom
                        break

        # tag heads and bodies
        head = self.visit(head, loc="head")
        body = self.visit(body, loc="body")

        return ast.Rule(rule.location, head, body)

    def _rewrite_body(self, x):
        """
        Rewrite the body of the given statement.
        """
        body = self.visit(x.body, loc="body")
        if x.body is not body:
            x = copy(x)
            x.body = body
        return x

    def visit_ShowTerm(self, x):
        """
        Rewrite statements with a body.
        """
        return self._rewrite_body(x)

    def visit_Minimize(self, x):
        """
        Rewrite statements with a body.
        """
        return self._rewrite_body(x)

    def visit_External(self, x):
        """
        Rewrite statements with a body.
        """
        return self._rewrite_body(x)

    def visit_Edge(self, x):
        """
        Rewrite statements with a body.
        """
        return self._rewrite_body(x)

    def visit_Heuristic(self, x):
        """
        Rewrite statements with a body.
        """
        return self._rewrite_body(x)

    def visit_ProjectAtom(self, x):
        """
        Rewrite statements with a body.
        """
        return self._rewrite_body(x)

    def _rewrite_tuple(self, element, number):
        """
        Add variables to tuple to ensure multiset semantics.
        """
        if len(element.tuple) != 1:
            raise RuntimeError("Invalid Syntax")

        in_condition = collect_variables(element.condition)
        for name in collect_variables(element.tuple):
            if name in in_condition:
                del in_condition[name]

        element.tuple = list(element.tuple)
        if number is not None:
            element.tuple.append(ast.Symbol(element.tuple[0].location, clingo.Number(number)))
        element.tuple.extend(in_condition[name] for name in sorted(in_condition))

        return element

    def _rewrite_tuples(self, elements):
        """
        Add variables to tuples of elements to ensure multiset semantics.
        """
        if len(elements) == 1:
            return [self._rewrite_tuple(elements[0], None)]

        return [self._rewrite_tuple(element, n) for n, element in enumerate(elements)]

    def visit_TheoryAtom(self, atom, loc="body"):
        """
        Mark head/body literals and ensure multiset semantics for theory atoms.
        """
        term = atom.term

        # ensure multi set semantics for theory atoms
        if term.name in ["sum", "diff", "distinct", "minimize", "maximize"] and not term.arguments:
            atom = unpool_theory_atom(atom)
            atom.elements = self._rewrite_tuples(atom.elements)

        # annotate theory atoms in heads and bodies
        if term.name in ["sum", "diff"] and not term.arguments:
            atom.term = ast.Function(term.location, term.name, [ast.Function(term.location, loc, [], False)], False)

        return atom


def transform(builder, s, shift):
    """
    Transform the program with csp constraints in the given file and pass it to
    the builder.
    """
    t = HeadBodyTransformer(shift)
    clingo.parse_program(s, lambda stm: builder.add(t.visit(stm)))
