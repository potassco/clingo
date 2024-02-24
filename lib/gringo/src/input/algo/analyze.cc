#include <algorithm>

#include <gringo/util/algorithm.hh>
#include <gringo/util/print.hh>

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/evaluate.hh>
#include <gringo/input/algo/print.hh>
#include <gringo/input/algo/visit_variables.hh>

namespace Gringo::Input {

namespace {

struct CheckType {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> bool = delete;

    // terms

    auto operator()(Term const &term) const -> bool { return std::visit(*this, term); }

    auto operator()(TermSymbol const &term) const -> bool {
        switch (term.value().type()) {
            case SymbolType::number: {
                auto num = term.value().num();
                if (type == TermCheckType::pos_number && *num >= 0) {
                    if (res != nullptr) {
                        res->pos_number = num;
                    }
                    return true;
                }
                return false;
            }
            case SymbolType::function: {
                if (type == TermCheckType::atom) {
                    return true;
                }
                if ((type == TermCheckType::identifier || type == TermCheckType::signed_identifier) &&
                    term.value().args().empty()) {
                    if (res != nullptr) {
                        res->identifier = term.value().name();
                    }
                    return true;
                }
                return false;
            }
            default: {
                return false;
            }
        }
    }

    auto operator()(TermVariable const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermTuple const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermFunction const &term) const -> bool {
        if (type == TermCheckType::atom) {
            return !term.external();
        }
        if ((type == TermCheckType::identifier || type == TermCheckType::signed_identifier) && !term.external() &&
            term.pool().size() == 1 && term.pool().front().elems().empty()) {
            if (res != nullptr) {
                res->identifier = term.name();
            }
            return true;
        }
        return false;
    }

    auto operator()(TermAbs const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermUnary const &term) const -> bool {
        if (type == TermCheckType::atom) {
            return term.op() == UnaryOperator::negate && std::visit(*this, *term.rhs());
        }
        if (type == TermCheckType::signed_identifier && term.op() == UnaryOperator::negate &&
            std::visit(CheckType{TermCheckType::identifier, res}, *term.rhs())) {
            if (res != nullptr) {
                res->has_sign = true;
            }
            return true;
        }
        return false;
    }

    auto operator()(TermBinary const &term) const -> bool {
        if (type == TermCheckType::sig) {
            return term.op() == BinaryOperator::div &&
                   std::visit(CheckType{TermCheckType::signed_identifier, res}, *term.lhs()) &&
                   std::visit(CheckType{TermCheckType::pos_number, res}, *term.rhs());
        }
        return false;
    }

    TermCheckType type;
    CheckTypeResult *res;
};

struct AlwaysNumeric {
    auto operator()(Term const &term) const -> bool { return std::visit(*this, term); }

    auto operator()(auto const &term) const -> bool = delete;

    auto operator()(TermSymbol const &term) const -> bool { return term.value().type() == SymbolType::number; }

    auto operator()(TermVariable const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermFunction const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermTuple const &term) const -> bool {
        return term.pool().size() == 1 && std::holds_alternative<ArgumentTuple>(term.pool().front());
    }

    auto operator()(TermAbs const &term) const -> bool {
        static_cast<void>(term);
        return true;
    }

    auto operator()(TermUnary const &term) const -> bool {
        return term.op() == UnaryOperator::invert || std::visit(*this, *term.rhs());
    }

    auto operator()(TermBinary const &term) const -> bool {
        static_cast<void>(term);
        return true;
    }
};

struct NeverNumeric {
    auto operator()(Term const &term) const -> bool { return std::visit(*this, term); }

    auto operator()(auto const &term) const -> bool = delete;

    auto operator()(TermSymbol const &term) const -> bool { return term.value().type() != SymbolType::number; }

    auto operator()(TermVariable const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermFunction const &term) const -> bool {
        static_cast<void>(term);
        return true;
    }

    auto operator()(TermTuple const &term) const -> bool {
        return term.pool().size() == 1 && std::holds_alternative<ArgumentTuple>(term.pool().front());
    }

    auto operator()(TermAbs const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermUnary const &term) const -> bool {
        return term.op() == UnaryOperator::negate && std::visit(*this, *term.rhs());
    }

    auto operator()(TermBinary const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }
};

struct IsTest {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> bool = delete;

    // literals

    auto operator()(Lit const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(LitBool const &lit) const -> bool {
        static_cast<void>(lit);
        return true;
    }

    auto operator()(LitComparison const &lit) const -> bool {
        static_cast<void>(lit);
        return true;
    }

    auto operator()(LitSymbolic const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    // body literal

    auto operator()(BdLit const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(BdLitSimple const &lit) const -> bool { return operator()(lit.lit()); }

    auto operator()(BdLitConjunction const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(BdLitSetAggregate const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(BdLitAggregate const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(BdLitTheoryAtom const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }
};

struct IsAtom {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> bool = delete;

    // literal

    auto operator()(Lit const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(LitBool const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(LitComparison const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(LitSymbolic const &lit) const -> bool { return lit.sign() == Sign::none; }

    // conditional literals

    template <bool HasSign> auto operator()(SetAggregate<HasSign> const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    // head literal

    auto operator()(HdLit const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(HdLitSimple const &lit) const -> bool { return operator()(lit.lit()); }

    auto operator()(HdLitDisjunction const &lit) const -> bool {
        if (lit.elems().size() != 1) {
            return false;
        }
        auto const *front = std::get_if<Lit>(&lit.elems().front());
        return front != nullptr && operator()(*front);
    }

    auto operator()(HdLitAggregate const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(HdLitTheoryAtom const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    // body literal

    auto operator()(BdLit const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(BdLitSimple const &lit) const -> bool { return operator()(lit.lit()); }

    auto operator()(BdLitConjunction const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(BdLitAggregate const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(BdLitTheoryAtom const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }
};

struct IsClassical {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> bool = delete;

    // head literal

    auto operator()(HdLit const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(HdLitSimple const &lit) const -> bool { return !is_atom(lit.lit()); }

    auto operator()(HdLitAggregate const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(HdLitSetAggregate const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(HdLitTheoryAtom const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(HdLitDisjunction const &lit) const -> bool {
        return std::all_of(lit.elems().begin(), lit.elems().end(), [](auto const &elem) {
            return std::visit(
                []<class T>(T const &lit) {
                    if constexpr (std::is_same_v<T, Lit>) {
                        return !is_atom(lit);
                    }
                    if constexpr (std::is_same_v<T, CondLit>) {
                        return !is_atom(lit.lit());
                    }
                },
                elem);
        });
    }
};

struct IsFact {
    IsFact(SymbolStore &store) : store{store} {}

    template <class T> auto operator()(T const &x) const -> bool = delete;

    auto operator()(Term const &term) const -> std::optional<Symbol> { return std::visit(*this, term); }

    auto operator()(TermVariable const &term) const -> std::optional<Symbol> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermSymbol const &term) const -> std::optional<Symbol> { return term.value(); }

    auto operator()(ArgumentTuple const &tuple) const -> std::optional<SymbolVec> {
        SymbolVec res;
        for (auto const &x : tuple.elems()) {
            auto const *term = std::get_if<Term>(&x);
            if (term == nullptr) {
                return std::nullopt;
            }
            auto res_term = operator()(*term);
            if (!res_term) {
                return std::nullopt;
            }
            res.emplace_back(std::move(res_term).value());
        }
        return res;
    }

    auto operator()(TermFunction const &term) const -> std::optional<Symbol> {
        static_cast<void>(term);
        if (term.pool().size() != 1 || term.external()) {
            return std::nullopt;
        }
        if (auto res_args = operator()(term.pool().front()); res_args) {
            return store.fun(term.name(), res_args.value(), false);
        }
        return std::nullopt;
    }

    auto operator()(TermTuple const &term) const -> std::optional<Symbol> {
        if (term.pool().size() != 1) {
            return std::nullopt;
        }
        return std::visit(
            [this]<class T>(T const &x) -> std::optional<Symbol> {
                if constexpr (std::is_same_v<T, Term>) {
                    return operator()(x);
                }
                if constexpr (std::is_same_v<T, ArgumentTuple>) {
                    if (auto tuple = operator()(x); tuple) {
                        return store.tup(*tuple);
                    }
                    return std::nullopt;
                }
            },
            term.pool().front());
    }

    auto operator()(TermAbs const &term) const -> std::optional<Symbol> {
        if (term.pool().size() != 1) {
            return std::nullopt;
        }
        if (auto arg = operator()(term.pool().front()); arg && arg->type() == SymbolType::number) {
            return store.num(abs(*arg->num()));
        }
        return std::nullopt;
    }

    auto operator()(TermUnary const &term) const -> std::optional<Symbol> {
        if (auto rhs = operator()(*term.rhs()); rhs) {
            return evaluate(store, term.op(), rhs.value());
        }
        return std::nullopt;
    }

    auto operator()(TermBinary const &term) const -> std::optional<Symbol> {
        if (term.op() == BinaryOperator::dots) {
            return std::nullopt;
        }
        if (auto lhs = operator()(*term.lhs()), rhs = operator()(*term.rhs()); lhs && rhs) {
            return evaluate(store, *lhs, term.op(), *rhs);
        }
        return std::nullopt;
    }

    SymbolStore &store;
};

} // namespace

auto check_type(Term const &term, TermCheckType type, CheckTypeResult *res) -> bool {
    return CheckType{type, res}(term);
}

[[nodiscard]] auto is_linear(TermBinary const &term) -> std::optional<String> {
    if (term.op() != BinaryOperator::plus) {
        return std::nullopt;
    }
    auto const *mul = std::get_if<TermBinary>(&term.lhs().get());
    if (mul == nullptr || mul->op() != BinaryOperator::times) {
        return std::nullopt;
    }
    auto const *n = std::get_if<TermSymbol>(&term.rhs().get());
    if (n == nullptr || n->value().type() != SymbolType::number) {
        return std::nullopt;
    }
    auto const *m = std::get_if<TermSymbol>(&mul->lhs().get());
    if (m == nullptr || m->value().type() != SymbolType::number || *m->value().num() == 0) {
        return std::nullopt;
    }
    auto const *v = std::get_if<TermVariable>(&mul->rhs().get());
    if (v == nullptr) {
        return std::nullopt;
    }
    return v->name();
}

[[nodiscard]] auto is_linear(Term const &term) -> std::optional<String> {
    auto const *plus = std::get_if<TermBinary>(&term);
    if (plus == nullptr) {
        return std::nullopt;
    }
    return is_linear(*plus);
}

[[nodiscard]] auto is_interval(Term const &term) -> bool {
    auto const *bin = std::get_if<TermBinary>(&term);
    return bin != nullptr && is_interval(*bin);
}

[[nodiscard]] auto is_interval(TermBinary const &term) -> bool { return term.op() == BinaryOperator::dots; }

[[nodiscard]] auto always_numeric(Term const &term) -> bool { return AlwaysNumeric{}(term); }

[[nodiscard]] auto never_numeric(Term const &term) -> bool { return NeverNumeric{}(term); }

auto is_atom(Lit const &lit) -> bool { return IsAtom{}(lit); }

auto is_atom(HdLit const &lit) -> bool { return IsAtom{}(lit); }

auto is_atom(BdLit const &lit) -> bool { return IsAtom{}(lit); }

auto is_test(Lit const &lit) -> bool { return IsTest{}(lit); }

// auto is_test(HeadLiteral const &lit) -> bool { return IsTest{}(lit); }

auto is_test(BdLit const &lit) -> bool { return IsTest{}(lit); }

auto is_classical(HdLit const &lit) -> bool { return IsClassical{}(lit); }

auto is_fact(SymbolStore &store, StmRule const &rule) -> std::optional<Symbol> {
    if (auto const *head = std::get_if<HdLitSimple>(&rule.head()); head != nullptr && rule.body().empty()) {
        if (auto const *lit = std::get_if<LitSymbolic>(&head->lit()); lit != nullptr && lit->sign() == Sign::none) {
            return IsFact{store}(lit->term());
        }
    }
    return std::nullopt;
}

auto is_fact(SymbolStore &store, Stm const &stm) -> std::optional<Symbol> {
    if (auto const *rule = std::get_if<StmRule>(&stm); rule != nullptr) {
        return is_fact(store, *rule);
    }
    return std::nullopt;
}

auto check_global(Logger &log, VariableSet const &global, Stm const &stm) -> bool {
    VariableSet new_global = select_variables(stm, VariableContext::global, global.size());
    std::vector<String> unsafe;
    visit_variables(
        stm,
        [&](Location const &loc, String var) {
            static_cast<void>(loc);
            if (!var.starts_with("$") && global.contains(var) != new_global.contains(var)) {
                unsafe.emplace_back(var);
            }
        },
        VariableContext::all);
    if (!unsafe.empty()) {
        std::sort(unsafe.begin(), unsafe.end());
        unsafe.erase(std::unique(unsafe.begin(), unsafe.end()), unsafe.end());
        GRINGO_REPORT_LOC(log, error, location(stm)) << "unsafe variables in:\n"
                                                     << "  " << stm << "\n"
                                                     << "note: the following variables are unsafe:\n"
                                                     << "  " << Util::p_range{unsafe, ", "};
        return false;
    }
    return true;
}

} // namespace Gringo::Input
