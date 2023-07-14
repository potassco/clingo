#include <input/rewrite_anonymous.hh>

#include "transform.hh"

namespace Gringo::Input {

namespace {

class TermRewriter : public TermVisitor {
  public:
    TermRewriter(NameGen &gen, std::optional<STerm> &result) : gen_{gen}, result_{result} {}
    [[nodiscard]] auto tra(auto const &x) const {
        return Trans(x, [this](STerm const &term) { return rewrite_anonymous(*term, gen_); });
    }

    void visit(TermSymbol const &term) const override { static_cast<void>(term); }

    void visit(TermVariable const &term) const override {
        if (term.is_anonymous()) {
            result_ = Util::construct_shared<TermVariable, Term>(gen_.new_name(), true);
        }
    }

    void visit(TermFunction const &term) const override {
        result_ = transform_construct_shared<TermFunction, Term>(term.name(), tra(term.pool()), term.is_external());
    }

    void visit(TermTuple const &term) const override {
        result_ = transform_construct_shared<TermTuple, Term>(tra(term.pool()));
    }

    void visit(TermAbs const &term) const override {
        result_ = transform_construct_shared<TermAbs, Term>(tra(term.pool()));
    }

    void visit(TermUnary const &term) const override {
        result_ = transform_construct_shared<TermUnary, Term>(term.unary_operator(), tra(term.rhs()));
    }

    void visit(TermBinary const &term) const override {
        result_ =
            transform_construct_shared<TermBinary, Term>(tra(term.lhs()), term.binary_operator(), tra(term.rhs()));
    }

  private:
    NameGen &gen_;
    std::optional<STerm> &result_;
};

class LiteralRewriter : public LiteralVisitor {
  public:
    LiteralRewriter(NameGen &gen, std::optional<SLiteral> &result) : gen_{gen}, result_{result} {}

    [[nodiscard]] auto tra(auto const &x) const {
        return Trans(x, [this](STerm const &term) { return rewrite_anonymous(*term, gen_); });
    }

    void visit(LiteralBoolean const &lit) const override { static_cast<void>(lit); }

    void visit(LiteralRelation const &lit) const override {
        result_ = transform_construct_shared<LiteralRelation, Literal>(lit.sign(), tra(lit.lhs()), tra(lit.rhs()));
    }

    void visit(LiteralSymbolic const &lit) const override {
        result_ = transform_construct_shared<LiteralSymbolic, Literal>(lit.sign(), tra(lit.term()));
    }

  private:
    NameGen &gen_;
    std::optional<SLiteral> &result_;
};

} // namespace

[[nodiscard]] auto rewrite_anonymous(Term const &term, NameGen &gen) -> std::optional<STerm> {
    std::optional<STerm> result;
    term.accept(TermRewriter{gen, result});
    return result;
}

[[nodiscard]] auto rewrite_anonymous(Literal const &lit, NameGen &gen) -> std::optional<SLiteral> {
    std::optional<SLiteral> result;
    lit.accept(LiteralRewriter{gen, result});
    return result;
}

} // namespace Gringo::Input
