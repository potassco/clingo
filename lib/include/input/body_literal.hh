#pragma once

//! @file
//! This file contains the body literal interface and derived body literals.

#include <tuple>

#include <input/aggregate.hh>
#include <input/literal.hh>
#include <input/theory.hh>

namespace Gringo::Input {

class BodyLiteral;
class BodyLiteralVisitor;
//! A shared pointer to a body literal.
using SBodyLiteral = Util::shared_ptr<BodyLiteral>;
//! A vector of shared pointers to body literals.
using SBodyLiteralVec = std::vector<SBodyLiteral>;

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

  private:
    //! Increment reference count of the body literal.
    friend void inc_ref_count(BodyLiteral &lit) { ++lit.refs_; }
    //! Decrement reference count of the body literal.
    friend void dec_ref_count(BodyLiteral &lit) { ++lit.refs_; }
    //! Get reference count of the body literal.
    [[nodiscard]] friend auto get_ref_count(BodyLiteral const &lit) -> size_t { return lit.refs_; }

    //! The reference count of the body literal.
    size_t refs_ = 0;
};

class Conjunction : public BodyLiteral {
  public:
    using Element = std::pair<SLiteralVec, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit Conjunction(ElementVec elems) : elems_{std::move(elems)} {}

    //! Get the elements of the conjunction.
    [[nodiscard]] auto elements() const -> ElementVec const & { return elems_; }

    void add_sign(Sign sign) override;
    [[nodiscard]] auto unpool() const -> std::optional<SBodyLiteralVec> override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project, bool in_classical_scope) const
        -> std::optional<SBodyLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SBodyLiteral> override;
    [[nodiscard]] auto is_atom() const -> bool override;
    [[nodiscard]] auto is_test() const -> bool override;

    void accept(BodyLiteralVisitor const &visitor) const override;

  private:
    ElementVec elems_;
};

class BodyAggregate : public BodyLiteral {
  public:
    using Element = std::tuple<TermVec, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit BodyAggregate(Sign sign, LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : sign_{sign}, fun_(fun), elems_(std::move(elems)), lhs_{std::move(lhs)}, rhs_{std::move(rhs)} {}
    explicit BodyAggregate(AggregateFunction fun, ElementVec elems) : fun_(fun), elems_(std::move(elems)) {}
    explicit BodyAggregate(AggregateFunction fun, ElementVec elems, Relation rel, Term rhs)
        : fun_(fun), elems_(std::move(elems)), rhs_(std::make_pair(rel, std::move(rhs))) {}

    //! Get the sign of the literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! Get the aggregate function.
    [[nodiscard]] auto function() const -> AggregateFunction { return fun_; }
    //! Get the aggregate elements.
    [[nodiscard]] auto elements() const -> ElementVec const & { return elems_; }
    //! Get the left-hand-side.
    [[nodiscard]] auto lhs() const -> LGuard const & { return lhs_; }
    //! Get the right-hand-side.
    [[nodiscard]] auto rhs() const -> RGuard const & { return rhs_; }

    void add_sign(Sign sign) override;
    void set_left_guard(Term lhs, Relation rel);
    [[nodiscard]] auto unpool() const -> std::optional<SBodyLiteralVec> override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project, bool in_classical_scope) const
        -> std::optional<SBodyLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SBodyLiteral> override;

    void accept(BodyLiteralVisitor const &visitor) const override;

  private:
    Sign sign_ = Sign::none;
    AggregateFunction fun_;
    ElementVec elems_;
    LGuard lhs_;
    RGuard rhs_;
};

class BodySetAggregate : public BodyLiteral {
  public:
    explicit BodySetAggregate(Sign sign, SetAggregate aggr) : sign_{sign}, aggr_{std::move(aggr)} {}
    explicit BodySetAggregate(SetAggregate aggr) : aggr_{std::move(aggr)} {}

    //! Get the sign of the literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! Get the set aggregate atom.
    [[nodiscard]] auto atom() const -> SetAggregate const & { return aggr_; }

    void add_sign(Sign sign) override;
    void set_left_guard(Term lhs, Relation rel);
    [[nodiscard]] auto unpool() const -> std::optional<SBodyLiteralVec> override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project, bool in_classical_scope) const
        -> std::optional<SBodyLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SBodyLiteral> override;

    void accept(BodyLiteralVisitor const &visitor) const override;

  private:
    Sign sign_ = Sign::none;
    SetAggregate aggr_;
};

class BodyTheoryAtom : public BodyLiteral {
  public:
    explicit BodyTheoryAtom(TheoryAtom atom) : atom_(std::move(atom)) {}
    explicit BodyTheoryAtom(Sign sign, TheoryAtom atom) : sign_{sign}, atom_(std::move(atom)) {}

    //! Get the sign of the literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! Get the theory atom.
    [[nodiscard]] auto atom() const -> TheoryAtom const & { return atom_; }

    void add_sign(Sign sign) override;
    [[nodiscard]] auto unpool() const -> std::optional<SBodyLiteralVec> override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project, bool in_classical_scope) const
        -> std::optional<SBodyLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SBodyLiteral> override;

    void accept(BodyLiteralVisitor const &visitor) const override;

  private:
    Sign sign_ = Sign::none;
    TheoryAtom atom_;
};

//! A visitor for available body literal types.
class BodyLiteralVisitor {
  public:
    //! Virtual destructor.
    virtual ~BodyLiteralVisitor() = default;

    //! Visit a disjunction.
    virtual void visit(Conjunction const &lit) const = 0;
    //! Visit a head set aggregate.
    virtual void visit(BodySetAggregate const &lit) const = 0;
    //! Visit a head aggregate.
    virtual void visit(BodyAggregate const &lit) const = 0;
    //! Visit a theory atom in the head.
    virtual void visit(BodyTheoryAtom const &lit) const = 0;
};

} // namespace Gringo::Input
