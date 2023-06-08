#pragma once

#include <literal.hh>
#include <term.hh>

class TheoryTerm {
  public:
    virtual ~TheoryTerm() = default;

    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string;
    friend auto operator<<(std::ostream &out, TheoryTerm const &term) -> std::ostream &;

    size_t refs = 0;
};

using STheoryTerm = shared_ptr<TheoryTerm>;
using STheoryTermVec = std::vector<STheoryTerm>;

class TheoryTermUnparsed : public TheoryTerm {
  public:
    using OpVec = std::vector<std::string>;
    using RHS = std::pair<OpVec, STheoryTerm>;
    using RHSVec = std::vector<RHS>;

    explicit TheoryTermUnparsed(OpVec ops, STheoryTerm term, RHSVec rhs = {})
        : ops_{std::move(ops)}, term_{std::move(term)}, rhs_{std::move(rhs)} {}
    explicit TheoryTermUnparsed(STheoryTerm term, RHSVec rhs) : term_{std::move(term)}, rhs_{std::move(rhs)} {}

    void print(std::ostream &out) const override;

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

  private:
    TheoryTermTupleType type_;
    ElementVec elems_;
};

class TheoryTermSymbol : public TheoryTerm {
  public:
    using Element = STheoryTerm;
    using ElementVec = std::vector<STheoryTerm>;

    TheoryTermSymbol(Symbol value) : value_{std::move(value)} {}

    void print(std::ostream &out) const override;

  private:
    Symbol value_;
};

class TheoryTermVariable : public TheoryTerm {
  public:
    using Element = STheoryTerm;
    using ElementVec = std::vector<STheoryTerm>;

    explicit TheoryTermVariable(std::string value) : name_{std::move(value)} {}

    void print(std::ostream &out) const override;

  private:
    std::string name_;
};

class TheoryTermFunction : public TheoryTerm {
  public:
    explicit TheoryTermFunction(std::string name, STheoryTermVec args = {})
        : name_(std::move(name)), args_{std::move(args)} {}

    void print(std::ostream &out) const override;

  private:
    std::string name_;
    STheoryTermVec args_;
};

class TheoryAtom {
  public:
    using Element = std::pair<STheoryTermVec, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    TheoryAtom(STerm name, ElementVec elems) : name_{std::move(name)}, elems_{std::move(elems)} {}
    TheoryAtom(STerm name, ElementVec elems, std::string rhs_op, STheoryTerm rhs_term)
        : name_{std::move(name)}, elems_{std::move(elems)},
          rhs_{std::make_pair(std::move(rhs_op), std::move(rhs_term))} {}

    friend auto operator<<(std::ostream &out, TheoryAtom const &atom) -> std::ostream &;

  private:
    STerm name_;
    ElementVec elems_;
    std::optional<std::pair<std::string, STheoryTerm>> rhs_;
};
