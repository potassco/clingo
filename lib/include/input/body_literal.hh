#pragma once

//! @file
//! This file contains the body literal interface and derived body literals.

#include <tuple>

#include <input/aggregate.hh>
#include <input/literal.hh>
#include <input/theory.hh>

namespace Gringo::Input {

/*
//! The body literal interface.
class BodyLiteral {
  public:
    //! The virtual destructor.
    virtual ~BodyLiteral() = default;

    //! Visit head literals with the given visitor.
    virtual void accept(BodyLiteralVisitor const &visitor) const = 0;

    //! Add a sign to the body literal.
    //!
    //! Note that this function has to be used with care because the library uses shared pointers to literals.
    //! This function is currently only used during construction in the parser.
    virtual void add_sign(Sign sign) = 0;
    //! Remove all pooled arguments from the literal.
    [[nodiscard]] virtual auto unpool() const -> std::optional<SBodyLiteralVec> = 0;
    //! Visit variables with the given function.
    virtual void visit_variables(VarVisitFun const &fun, VariableContext ctx) const = 0;
    //! Project variables according to given projection mode and scope.
    //!
    //! Some literal occurrences cannot be projected preserving equivalence.
    //! For example, variables in nonmonotone aggregates are only projected in classical scope.
    [[nodiscard]] virtual auto project(Projection project, bool is_classical_scope) const
        -> std::optional<SBodyLiteral> = 0;
    //! Project anonymous variables in (nested) negated symbolic literals.
    //!
    //! This is a deprecated feature to support old programs.
    //! The projection star should be used instead.
    [[nodiscard]] virtual auto project_anonymous() const -> std::optional<SBodyLiteral> = 0;
    //! Check whether the literal is an atoms.
    //!
    //! A literal is an atom if it is a symbolic literal without a sign.
    //! This corresponds for example to a conjunction with one element equal to a such an atom.
    [[nodiscard]] virtual auto is_atom() const -> bool;
    //! Check whether the literal is a test.
    //!
    //! A test is a negated or not a symbolic literal.
    //! This corresponds to conjunctions where the right-hand-side of elements is empty
    //! and the left-hand-side is composed of tests.
    [[nodiscard]] virtual auto is_test() const -> bool;
};
*/

struct Conjunction {
    explicit Conjunction(ConditionalLiteralVec elems) : elems{std::move(elems)} {}

    ConditionalLiteralVec elems;
};

struct BodyAggregate {
    struct Element {
        Element(TermVec tuple, LiteralVec cond) : tuple{std::move(tuple)}, cond{std::move(cond)} {}
        TermVec tuple;
        LiteralVec cond;
    };
    using ElementVec = std::vector<Element>;

    explicit BodyAggregate(Sign sign, LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : sign{sign}, fun(fun), elems(std::move(elems)), lhs{std::move(lhs)}, rhs{std::move(rhs)} {}
    explicit BodyAggregate(AggregateFunction fun, ElementVec elems, Relation rel, Term rhs)
        : BodyAggregate{Sign::none, std::nullopt, fun, std::move(elems), std::make_pair(rel, std::move(rhs))} {}
    explicit BodyAggregate(AggregateFunction fun, ElementVec elems)
        : BodyAggregate{Sign::none, std::nullopt, fun, std::move(elems), std::nullopt} {}

    Sign sign;
    AggregateFunction fun;
    ElementVec elems;
    LGuard lhs;
    RGuard rhs;
};

struct BodySetAggregate {
    explicit BodySetAggregate(Sign sign, SetAggregate aggr) : sign{sign}, aggr{std::move(aggr)} {}
    explicit BodySetAggregate(SetAggregate aggr) : BodySetAggregate{Sign::none, std::move(aggr)} {}

    Sign sign;
    SetAggregate aggr;
};

struct BodyTheoryAtom {
    explicit BodyTheoryAtom(Sign sign, TheoryAtom atom) : sign{sign}, atom{std::move(atom)} {}
    explicit BodyTheoryAtom(TheoryAtom atom) : BodyTheoryAtom{Sign::none, std::move(atom)} {}

    Sign sign;
    TheoryAtom atom;
};

using BodyLiteral = std::variant<Conjunction, BodyAggregate, BodySetAggregate, BodyTheoryAtom>;
using BodyLiteralVec = std::vector<BodyLiteral>;

} // namespace Gringo::Input
