#pragma once

#include <functional>

#include <input/literal.hh>
#include <input/term.hh>

namespace Gringo::Input {

class TheoryTerm;
class TheoryTermVisitor;
using STheoryTerm = Util::shared_ptr<TheoryTerm>;
using STheoryTermVec = std::vector<STheoryTerm>;

class TheoryTerm {
  public:
    virtual ~TheoryTerm() = default;

    virtual void visit_variables(VarVisitFun fun) const = 0;

    //! Visit theory terms with the given visitor.
    virtual void accept(TheoryTermVisitor const &visitor) const = 0;

  private:
    friend void inc_ref_count(TheoryTerm &term) { ++term.refs_; }
    friend void dec_ref_count(TheoryTerm &term) { ++term.refs_; }
    [[nodiscard]] friend auto get_ref_count(TheoryTerm const &term) -> size_t { return term.refs_; }

    size_t refs_ = 0;
};

class TheoryTermUnparsed : public TheoryTerm {
  public:
    using OpVec = std::vector<std::string>;
    using Element = std::pair<OpVec, STheoryTerm>;
    using ElementVec = std::vector<Element>;

    explicit TheoryTermUnparsed(ElementVec elems) : elems_{std::move(elems)} {}

    void visit_variables(VarVisitFun fun) const override;

    void accept(TheoryTermVisitor const &visitor) const override;

    //! Get the elements of the unparsed term.
    [[nodiscard]] auto elements() const -> ElementVec const & { return elems_; }

  private:
    ElementVec elems_;
};

enum class TheoryTermTupleType { Tuple, Set, List };

class TheoryTermTuple : public TheoryTerm {
  public:
    using Element = STheoryTerm;
    using ElementVec = std::vector<STheoryTerm>;

    explicit TheoryTermTuple(TheoryTermTupleType type, ElementVec elems) : type_{type}, elems_{std::move(elems)} {}

    //! Get the type of the theory term tuple.
    [[nodiscard]] auto type() const -> TheoryTermTupleType { return type_; }
    //! Get the elements of the theory term tuple.
    [[nodiscard]] auto elements() const -> ElementVec const & { return elems_; }

    void visit_variables(VarVisitFun fun) const override;

    void accept(TheoryTermVisitor const &visitor) const override;

  private:
    TheoryTermTupleType type_;
    ElementVec elems_;
};

class TheoryTermSymbol : public TheoryTerm {
  public:
    using Element = STheoryTerm;
    using ElementVec = std::vector<STheoryTerm>;

    explicit TheoryTermSymbol(Symbol value) : value_{std::move(value)} {}

    //! Get the symbol of the theory term symbol.
    [[nodiscard]] auto symbol() const -> Symbol { return value_; }

    void visit_variables(VarVisitFun fun) const override;

    void accept(TheoryTermVisitor const &visitor) const override;

  private:
    Symbol value_;
};

class TheoryTermVariable : public TheoryTerm {
  public:
    using Element = STheoryTerm;
    using ElementVec = std::vector<STheoryTerm>;

    explicit TheoryTermVariable(std::string value, bool is_anonymous = false)
        : name_{std::move(value)}, is_anonymous_{is_anonymous} {}

    //! Get the variable name of the theory term variable.
    [[nodiscard]] auto name() const -> std::string const & { return name_; }
    //! Check whether the variable is anonymous.
    [[nodiscard]] auto is_anonymous() const -> bool { return is_anonymous_; }

    void visit_variables(VarVisitFun fun) const override;

    void accept(TheoryTermVisitor const &visitor) const override;

  private:
    std::string name_;
    bool is_anonymous_;
};

class TheoryTermFunction : public TheoryTerm {
  public:
    explicit TheoryTermFunction(std::string name, STheoryTermVec args = {})
        : name_(std::move(name)), args_{std::move(args)} {}

    void visit_variables(VarVisitFun fun) const override;

    //! Get the name of the theory term function.
    [[nodiscard]] auto name() const -> std::string const & { return name_; }
    //! Get the arguments of the theory term function.
    [[nodiscard]] auto arguments() const -> STheoryTermVec const & { return args_; }

    void accept(TheoryTermVisitor const &visitor) const override;

  private:
    std::string name_;
    STheoryTermVec args_;
};

//! A visitor for available theory term types.
class TheoryTermVisitor {
  public:
    //! Virtual destructor.
    virtual ~TheoryTermVisitor() = default;

    //! Visit an unparsed theory term.
    virtual void visit(TheoryTermUnparsed const &term) const = 0;
    //! Visit a theory term symbol.
    virtual void visit(TheoryTermSymbol const &term) const = 0;
    //! Visit a theory term variable.
    virtual void visit(TheoryTermVariable const &term) const = 0;
    //! Visit a tuple theory term.
    virtual void visit(TheoryTermTuple const &term) const = 0;
    //! Visit a function theory term.
    virtual void visit(TheoryTermFunction const &term) const = 0;
};

class TheoryAtom {
  public:
    using RGuard = std::optional<std::pair<std::string, STheoryTerm>>;
    using Element = std::pair<STheoryTermVec, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit TheoryAtom(Term name, ElementVec elems, RGuard rhs)
        : name_{std::move(name)}, elems_{std::move(elems)}, rhs_{std::move(rhs)} {}
    explicit TheoryAtom(Term name, ElementVec elems) : name_{std::move(name)}, elems_{std::move(elems)} {}
    explicit TheoryAtom(Term name, ElementVec elems, std::string rhs_op, STheoryTerm rhs_term)
        : name_{std::move(name)}, elems_{std::move(elems)},
          rhs_{std::in_place, std::move(rhs_op), std::move(rhs_term)} {}

    //! Get the name of the theory atom.
    [[nodiscard]] auto name() const -> Term const & { return name_; }
    //! Get the elements of the theory atom.
    [[nodiscard]] auto elements() const -> ElementVec const & { return elems_; }
    //! Get the right-hand-side of the theory atom.
    [[nodiscard]] auto rhs() const -> RGuard const & { return rhs_; }

    [[nodiscard]] auto unpool() const -> std::optional<std::vector<TheoryAtom>>;
    void visit_variables(VarVisitFun fun, VariableContext ctx) const;
    [[nodiscard]] auto project_anonymous() const -> std::optional<TheoryAtom>;

  private:
    Term name_;
    ElementVec elems_;
    RGuard rhs_;
};

} // namespace Gringo::Input
