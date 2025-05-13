#include "transform.hh"

#include <clingo/input/rewrite/rewrite_anonymous.hh>
#include <clingo/input/rewrite/visit_variables.hh>

namespace CppClingo::Input {

namespace {

class RewriteAnonymous : public Transformer<RewriteAnonymous> {
  public:
    RewriteAnonymous(NameGen &gen) : gen_{&gen} {}

    // protect ourselves -> no unintended overloads

    template <class T> [[nodiscard]] auto accept(T const &x) const = delete;

    // term

    [[nodiscard]] auto accept(TermVariable const &term) const -> std::optional<Term> {
        if (term.anonymous()) {
            return TermVariable{term.loc(), gen_->new_name(), true};
        }
        return std::nullopt;
    }

    // theory

    [[nodiscard]] auto accept(TheoryTermVariable const &term) const -> std::optional<TheoryTerm> {
        if (term.anonymous()) {
            return TheoryTermVariable{term.loc(), gen_->new_name(), true};
        }
        return std::nullopt;
    }

  private:
    NameGen *gen_;
};

} // namespace

[[nodiscard]] auto rewrite_anonymous(Term const &term, NameGen &gen) -> std::optional<Term> {
    return RewriteAnonymous{gen}.transform(term);
}

[[nodiscard]] auto rewrite_anonymous(TheoryTerm const &term, NameGen &gen) -> std::optional<TheoryTerm> {
    return RewriteAnonymous{gen}.transform(term);
}

[[nodiscard]] auto rewrite_anonymous(Lit const &lit, NameGen &gen) -> std::optional<Lit> {
    return RewriteAnonymous{gen}.transform(lit);
}

[[nodiscard]] auto rewrite_anonymous(HdLit const &lit, NameGen &gen) -> std::optional<HdLit> {
    return RewriteAnonymous{gen}.transform(lit);
}

[[nodiscard]] auto rewrite_anonymous(BdLit const &lit, NameGen &gen) -> std::optional<BdLit> {
    return RewriteAnonymous{gen}.transform(lit);
}

[[nodiscard]] auto rewrite_anonymous(SymbolStore &store, Stm const &stm) -> std::optional<Stm> {
    auto gen = NameGen{store, select_variables(stm, VariableContext::all), "__A_"};
    return RewriteAnonymous{gen}.transform(stm);
}

} // namespace CppClingo::Input
