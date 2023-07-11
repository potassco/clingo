#pragma once

#include <functional>

#include <input/literal.hh>
#include <input/term.hh>

namespace Gringo::Input {

class TheoryTerm;
using STheoryTerm = Util::shared_ptr<TheoryTerm>;
using STheoryTermVec = std::vector<STheoryTerm>;

class TheoryTerm {
  public:
    virtual ~TheoryTerm() = default;

    virtual void print(std::ostream &out) const = 0;
    virtual void visit_variables(VarVisitFun fun) const = 0;
    [[nodiscard]] virtual auto rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> = 0;

    [[nodiscard]] auto to_string() const -> std::string;
    friend auto operator<<(std::ostream &out, TheoryTerm const &term) -> std::ostream &;

  private:
    friend void inc_ref_count(TheoryTerm &term) { ++term.refs_; }
    friend void dec_ref_count(TheoryTerm &term) { ++term.refs_; }
    [[nodiscard]] friend auto get_ref_count(TheoryTerm const &term) -> size_t { return term.refs_; }

    size_t refs_ = 0;
};

class TheoryTermUnparsed : public TheoryTerm {
  public:
    using OpVec = std::vector<std::string>;
    using RHS = std::pair<OpVec, STheoryTerm>;
    using RHSVec = std::vector<RHS>;

    explicit TheoryTermUnparsed(OpVec ops, STheoryTerm term, RHSVec rhs = {})
        : ops_{std::move(ops)}, term_{std::move(term)}, rhs_{std::move(rhs)} {}
    explicit TheoryTermUnparsed(STheoryTerm term, RHSVec rhs) : term_{std::move(term)}, rhs_{std::move(rhs)} {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun fun) const override;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> override;

  private:
    OpVec ops_;
    STheoryTerm term_;
    RHSVec rhs_;
};

enum class TheoryTermTupleType { Tuple, Set, List };

auto left_bracket(TheoryTermTupleType type) -> char;

auto right_bracket(TheoryTermTupleType type) -> char;

class TheoryTermTuple : public TheoryTerm {
  public:
    using Element = STheoryTerm;
    using ElementVec = std::vector<STheoryTerm>;

    explicit TheoryTermTuple(TheoryTermTupleType type, ElementVec elems) : type_{type}, elems_{std::move(elems)} {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun fun) const override;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> override;

  private:
    TheoryTermTupleType type_;
    ElementVec elems_;
};

class TheoryTermSymbol : public TheoryTerm {
  public:
    using Element = STheoryTerm;
    using ElementVec = std::vector<STheoryTerm>;

    explicit TheoryTermSymbol(Symbol value) : value_{std::move(value)} {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun fun) const override;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> override;

  private:
    Symbol value_;
};

class TheoryTermVariable : public TheoryTerm {
  public:
    using Element = STheoryTerm;
    using ElementVec = std::vector<STheoryTerm>;

    explicit TheoryTermVariable(std::string value, bool is_anonymous = false)
        : name_{std::move(value)}, is_anonymous_{is_anonymous} {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun fun) const override;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> override;

  private:
    std::string name_;
    bool is_anonymous_;
};

class TheoryTermFunction : public TheoryTerm {
  public:
    explicit TheoryTermFunction(std::string name, STheoryTermVec args = {})
        : name_(std::move(name)), args_{std::move(args)} {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun fun) const override;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> override;

  private:
    std::string name_;
    STheoryTermVec args_;
};

class TheoryAtom {
  public:
    using RGuard = std::optional<std::pair<std::string, STheoryTerm>>;
    using Element = std::pair<STheoryTermVec, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit TheoryAtom(STerm name, ElementVec elems, RGuard rhs)
        : name_{std::move(name)}, elems_{std::move(elems)}, rhs_{std::move(rhs)} {}
    explicit TheoryAtom(STerm name, ElementVec elems) : name_{std::move(name)}, elems_{std::move(elems)} {}
    explicit TheoryAtom(STerm name, ElementVec elems, std::string rhs_op, STheoryTerm rhs_term)
        : name_{std::move(name)}, elems_{std::move(elems)},
          rhs_{std::in_place, std::move(rhs_op), std::move(rhs_term)} {}

    [[nodiscard]] auto unpool() const -> std::optional<std::vector<TheoryAtom>>;
    void visit_variables(VarVisitFun fun, VariableContext ctx) const;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<TheoryAtom>;
    [[nodiscard]] auto project_anonymous() const -> std::optional<TheoryAtom>;

    friend auto operator<<(std::ostream &out, TheoryAtom const &atom) -> std::ostream &;

  private:
    STerm name_;
    ElementVec elems_;
    RGuard rhs_;
};

} // namespace Gringo::Input
