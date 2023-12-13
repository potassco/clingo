#include <input/algo/rewrite_anonymous.hh>
#include <input/algo/visit_variables.hh>

#include "transform.hh"

namespace Gringo::Input {

namespace {

struct RewriteAnonymous : Transformer<RewriteAnonymous> {
    RewriteAnonymous(NameGen &gen) : gen{gen} {}

    // protect ourselves -> no unintended overloads

    template <class T> [[nodiscard]] auto accept(T const &x) const -> std::optional<T> = delete;

    // term

    [[nodiscard]] auto accept(TermVariable const &term) const -> std::optional<Term> {
        if (term.is_anonymous) {
            return TermVariable{term.loc, gen.new_name(), true};
        }
        return std::nullopt;
    }

    // theory

    [[nodiscard]] auto accept(TheoryTermVariable const &term) const -> std::optional<TheoryTerm> {
        if (term.is_anonymous) {
            return TheoryTermVariable{term.loc, gen.new_name(), true};
        }
        return std::nullopt;
    }

    NameGen &gen;
};

} // namespace

[[nodiscard]] auto rewrite_anonymous(Term const &term, NameGen &gen) -> std::optional<Term> {
    return RewriteAnonymous{gen}.transform(term);
}

[[nodiscard]] auto rewrite_anonymous(TheoryTerm const &term, NameGen &gen) -> std::optional<TheoryTerm> {
    return RewriteAnonymous{gen}.transform(term);
}

[[nodiscard]] auto rewrite_anonymous(Literal const &lit, NameGen &gen) -> std::optional<Literal> {
    return RewriteAnonymous{gen}.transform(lit);
}

[[nodiscard]] auto rewrite_anonymous(HeadLiteral const &lit, NameGen &gen) -> std::optional<HeadLiteral> {
    return RewriteAnonymous{gen}.transform(lit);
}

[[nodiscard]] auto rewrite_anonymous(BodyLiteral const &lit, NameGen &gen) -> std::optional<BodyLiteral> {
    return RewriteAnonymous{gen}.transform(lit);
}

[[nodiscard]] auto rewrite_anonymous(SymbolStore &store, Statement const &stm) -> std::optional<Statement> {
    auto gen = NameGen{store, select_variables(stm, VariableContext::all), "__A_"};
    return RewriteAnonymous{gen}.transform(stm);
}

} // namespace Gringo::Input
