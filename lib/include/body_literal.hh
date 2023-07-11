#pragma once

//! @file
//! This file contains the body literal interface and derived body literals.

#include <tuple>

#include <aggregate.hh>
#include <literal.hh>
#include <theory.hh>

class BodyLiteral;
class BodyAggregate;
class BodySetAggregate;
//! A shared pointer to a body literal.
using SBodyLiteral = shared_ptr<BodyLiteral>;
//! A vector of shared pointers to body literals.
using SBodyLiteralVec = std::vector<SBodyLiteral>;

//! The body literal interface.
class BodyLiteral {
  public:
    //! The virtual destructor.
    virtual ~BodyLiteral() = default;

    //! Convert the body literal to string.
    [[nodiscard]] auto to_string() const -> std::string;
    //! Output the body literal to the given stream.
    friend auto operator<<(std::ostream &out, BodyLiteral const &literal) -> std::ostream &;

    //! Add a sign to the body literal.
    //!
    //! Note that this function has to be used with care because the library uses shared pointers to literals.
    //! This function is currently only used during construction in the parser.
    virtual void add_sign(Sign sign) = 0;
    //! Remove all pooled arguments from the literal.
    [[nodiscard]] virtual auto unpool() const -> std::optional<SBodyLiteralVec> = 0;
    //! Output the body literal to the given stream.
    virtual void print(std::ostream &out) const = 0;
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
    //! Give anonymous variables a unique name.
    [[nodiscard]] virtual auto rewrite_anonymous(NameGen &gen) const -> std::optional<SBodyLiteral> = 0;

  private:
    // When making the class compatible with C, this will require another solution that will avoid this.
    friend shared_ptr<BodyLiteral>;
    friend shared_ptr<BodyAggregate>;
    friend shared_ptr<BodySetAggregate>;

    //! The reference count of the body literal.
    size_t refs = 0;
};

class Conjunction : public BodyLiteral {
  public:
    using Element = std::pair<SLiteralVec, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit Conjunction(ElementVec elems) : elems_{std::move(elems)} {}

    void add_sign(Sign sign) override;
    [[nodiscard]] auto unpool() const -> std::optional<SBodyLiteralVec> override;
    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project, bool in_classical_scope) const
        -> std::optional<SBodyLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SBodyLiteral> override;
    [[nodiscard]] auto is_atom() const -> bool override;
    [[nodiscard]] auto is_test() const -> bool override;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<SBodyLiteral> override;

  private:
    ElementVec elems_;
};

class BodyAggregate : public BodyLiteral {
  public:
    using Element = std::tuple<STermVec, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit BodyAggregate(Sign sign, LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : sign_{sign}, fun_(fun), elems_(std::move(elems)), lhs_{std::move(lhs)}, rhs_{std::move(rhs)} {}
    explicit BodyAggregate(AggregateFunction fun, ElementVec elems) : fun_(fun), elems_(std::move(elems)) {}
    explicit BodyAggregate(AggregateFunction fun, ElementVec elems, Relation rel, STerm rhs)
        : fun_(fun), elems_(std::move(elems)), rhs_(std::make_pair(rel, std::move(rhs))) {}

    void add_sign(Sign sign) override;
    void set_left_guard(STerm lhs, Relation rel);
    [[nodiscard]] auto unpool() const -> std::optional<SBodyLiteralVec> override;
    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project, bool in_classical_scope) const
        -> std::optional<SBodyLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SBodyLiteral> override;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<SBodyLiteral> override;

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

    void add_sign(Sign sign) override;
    void set_left_guard(STerm lhs, Relation rel);
    [[nodiscard]] auto unpool() const -> std::optional<SBodyLiteralVec> override;
    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project, bool in_classical_scope) const
        -> std::optional<SBodyLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SBodyLiteral> override;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<SBodyLiteral> override;

  private:
    Sign sign_ = Sign::none;
    SetAggregate aggr_;
};

class BodyTheoryAtom : public BodyLiteral {
  public:
    explicit BodyTheoryAtom(TheoryAtom atom) : atom_(std::move(atom)) {}
    explicit BodyTheoryAtom(Sign sign, TheoryAtom atom) : sign_{sign}, atom_(std::move(atom)) {}

    void add_sign(Sign sign) override;
    [[nodiscard]] auto unpool() const -> std::optional<SBodyLiteralVec> override;
    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project, bool in_classical_scope) const
        -> std::optional<SBodyLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SBodyLiteral> override;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<SBodyLiteral> override;

  private:
    Sign sign_ = Sign::none;
    TheoryAtom atom_;
};
