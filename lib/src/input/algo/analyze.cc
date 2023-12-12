#include <algorithm>

#include <util/algorithm.hh>

#include <input/algo/analyze.hh>
#include <input/algo/evaluate.hh>

namespace Gringo::Input {

namespace {

struct CheckType {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> bool = delete;

    // terms

    auto operator()(Term const &term) const -> bool { return std::visit(*this, term); }

    auto operator()(TermSymbol const &term) const -> bool {
        switch (term.value.type()) {
            case SymbolType::number: {
                auto num = term.value.num();
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
                    term.value.args().empty()) {
                    if (res != nullptr) {
                        res->identifier = term.value.name();
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
            return !term.external;
        }
        if ((type == TermCheckType::identifier || type == TermCheckType::signed_identifier) && !term.external &&
            term.pool.size() == 1 && term.pool.front().empty()) {
            if (res != nullptr) {
                res->identifier = term.name;
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
            return term.op == UnaryOperator::negate && std::visit(*this, *term.rhs);
        }
        if (type == TermCheckType::signed_identifier && term.op == UnaryOperator::negate &&
            std::visit(CheckType{TermCheckType::identifier, res}, *term.rhs)) {
            if (res != nullptr) {
                res->has_sign = true;
            }
            return true;
        }
        return false;
    }

    auto operator()(TermBinary const &term) const -> bool {
        if (type == TermCheckType::sig) {
            return term.op == BinaryOperator::div &&
                   std::visit(CheckType{TermCheckType::signed_identifier, res}, *term.lhs) &&
                   std::visit(CheckType{TermCheckType::pos_number, res}, *term.rhs);
        }
        return false;
    }

    TermCheckType type;
    CheckTypeResult *res;
};

struct AlwaysNumeric {
    auto operator()(Term const &term) const -> bool { return std::visit(*this, term); }

    auto operator()(auto const &term) const -> bool = delete;

    auto operator()(TermSymbol const &term) const -> bool { return term.value.type() == SymbolType::number; }

    auto operator()(TermVariable const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermFunction const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermTuple const &term) const -> bool {
        return term.pool.size() == 1 && std::holds_alternative<TupleVec>(term.pool.front());
    }

    auto operator()(TermAbs const &term) const -> bool {
        static_cast<void>(term);
        return true;
    }

    auto operator()(TermUnary const &term) const -> bool {
        return term.op == UnaryOperator::invert || std::visit(*this, *term.rhs);
    }

    auto operator()(TermBinary const &term) const -> bool {
        static_cast<void>(term);
        return true;
    }
};

struct NeverNumeric {
    auto operator()(Term const &term) const -> bool { return std::visit(*this, term); }

    auto operator()(auto const &term) const -> bool = delete;

    auto operator()(TermSymbol const &term) const -> bool { return term.value.type() != SymbolType::number; }

    auto operator()(TermVariable const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermFunction const &term) const -> bool {
        static_cast<void>(term);
        return true;
    }

    auto operator()(TermTuple const &term) const -> bool {
        return term.pool.size() == 1 && std::holds_alternative<TupleVec>(term.pool.front());
    }

    auto operator()(TermAbs const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermUnary const &term) const -> bool {
        return term.op == UnaryOperator::negate && std::visit(*this, *term.rhs);
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

    auto operator()(Literal const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(LiteralBoolean const &lit) const -> bool {
        static_cast<void>(lit);
        return true;
    }

    auto operator()(LiteralRelation const &lit) const -> bool {
        static_cast<void>(lit);
        return true;
    }

    auto operator()(LiteralSymbolic const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    // conditional literals

    auto operator()(ConditionalLiteralVec const &elems) const -> bool {
        return std::all_of(elems.begin(), elems.end(), [this](auto const &elem) {
            return elem.cond.empty() && std::all_of(elem.lits.begin(), elem.lits.end(),
                                                    [this](auto const &lit) { return this->operator()(lit); });
        });
    }

    template <bool Conjunctive> auto operator()(Junction<Conjunctive> const &lit) const -> bool {
        return operator()(lit.elems);
    }

    // aggregates

    template <bool HasSign> auto operator()(SetAggregate<HasSign> const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> bool { return operator()(lit.lit); }

    auto operator()(HeadAggregate const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(HeadTheoryAtom const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> bool { return operator()(lit.lit); }

    auto operator()(BodyAggregate const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(BodyTheoryAtom const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }
};

struct IsAtom {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> bool = delete;

    // literal

    auto operator()(Literal const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(LiteralBoolean const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(LiteralRelation const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(LiteralSymbolic const &lit) const -> bool { return lit.sign == Sign::none; }

    // conditional literals

    auto operator()(ConditionalLiteralVec const &elems) const -> bool {
        if (elems.size() != 1) {
            return false;
        }
        auto const &elem = elems.front();
        return elem.cond.empty() && elem.lits.size() == 1 && operator()(elem.lits.front());
    }

    template <bool Conjunctive> auto operator()(Junction<Conjunctive> const &lit) const -> bool {
        return operator()(lit.elems);
    }

    template <bool HasSign> auto operator()(SetAggregate<HasSign> const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> bool { return operator()(lit.lit); }

    auto operator()(HeadAggregate const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(HeadTheoryAtom const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> bool { return operator()(lit.lit); }

    auto operator()(BodyAggregate const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(BodyTheoryAtom const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }
};

struct IsClassical {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> bool = delete;

    // head literal

    auto operator()(HeadLiteral const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> bool { return !is_atom(lit.lit); }

    auto operator()(HeadAggregate const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(HeadSetAggregate const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(HeadTheoryAtom const &lit) const -> bool {
        static_cast<void>(lit);
        return false;
    }

    auto operator()(Disjunction const &lit) const -> bool {
        return std::all_of(lit.elems.begin(), lit.elems.end(), [](auto const &elem) {
            return !std::any_of(elem.lits.begin(), elem.lits.end(), static_cast<bool (&)(Literal const &)>(is_atom));
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

    auto operator()(TermSymbol const &term) const -> std::optional<Symbol> { return term.value; }

    auto operator()(TupleVec const &tuple) const -> std::optional<SymbolVec> {
        SymbolVec res;
        for (auto const &x : tuple) {
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
        if (term.pool.size() != 1 || term.external) {
            return std::nullopt;
        }
        if (auto res_args = operator()(term.pool.front()); res_args) {
            return store.fun(term.name, res_args.value(), false);
        }
        return std::nullopt;
    }

    auto operator()(TermTuple const &term) const -> std::optional<Symbol> {
        if (term.pool.size() != 1) {
            return std::nullopt;
        }
        return std::visit(
            [this](auto &&x) -> std::optional<Symbol> {
                GRINGO_MATCH(x, Term) { return operator()(x); }
                GRINGO_MATCH(x, TupleVec) {
                    if (auto tuple = operator()(x); tuple) {
                        return store.tup(*tuple);
                    }
                    return std::nullopt;
                }
            },
            term.pool.front());
    }

    auto operator()(TermAbs const &term) const -> std::optional<Symbol> {
        if (term.pool.size() != 1) {
            return std::nullopt;
        }
        if (auto arg = operator()(term.pool.front()); arg && arg->type() == SymbolType::number) {
            return store.num(abs(*arg->num()));
        }
        return std::nullopt;
    }

    auto operator()(TermUnary const &term) const -> std::optional<Symbol> {
        if (auto rhs = operator()(*term.rhs); rhs) {
            return evaluate(store, term.op, rhs.value());
        }
        return std::nullopt;
    }

    auto operator()(TermBinary const &term) const -> std::optional<Symbol> {
        if (term.op == BinaryOperator::dots) {
            return std::nullopt;
        }
        if (auto lhs = operator()(*term.lhs), rhs = operator()(*term.rhs); lhs && rhs) {
            return evaluate(store, *lhs, term.op, *rhs);
        }
        return std::nullopt;
    }

    SymbolStore &store;
};

} // namespace

auto check_type(Term const &term, TermCheckType type, CheckTypeResult *res) -> bool {
    return CheckType{type, res}(term);
}

[[nodiscard]] auto is_linear(TermBinary const &term) -> bool {
    if (term.op != BinaryOperator::plus) {
        return false;
    }
    auto const *mul = std::get_if<TermBinary>(term.lhs.get());
    if (mul == nullptr || mul->op != BinaryOperator::times) {
        return false;
    }
    auto const *n = std::get_if<TermSymbol>(term.rhs.get());
    if (n == nullptr || n->value.type() != SymbolType::number) {
        return false;
    }
    auto const *m = std::get_if<TermSymbol>(mul->lhs.get());
    if (m == nullptr || m->value.type() != SymbolType::number || *m->value.num() == 0) {
        return false;
    }
    return std::holds_alternative<TermVariable>(*mul->rhs);
}

[[nodiscard]] auto is_linear(Term const &term) -> bool {
    auto const *plus = std::get_if<TermBinary>(&term);
    return plus != nullptr && is_linear(*plus);
}

[[nodiscard]] auto is_interval(Term const &term) -> bool {
    auto const *bin = std::get_if<TermBinary>(&term);
    return bin != nullptr && is_interval(*bin);
}

[[nodiscard]] auto is_interval(TermBinary const &term) -> bool { return term.op == BinaryOperator::dots; }

[[nodiscard]] auto always_numeric(Term const &term) -> bool { return AlwaysNumeric{}(term); }

[[nodiscard]] auto never_numeric(Term const &term) -> bool { return NeverNumeric{}(term); }

auto is_atom(Literal const &lit) -> bool { return IsAtom{}(lit); }

auto is_atom(HeadLiteral const &lit) -> bool { return IsAtom{}(lit); }

auto is_atom(BodyLiteral const &lit) -> bool { return IsAtom{}(lit); }

auto is_test(Literal const &lit) -> bool { return IsTest{}(lit); }

auto is_test(HeadLiteral const &lit) -> bool { return IsTest{}(lit); }

auto is_test(BodyLiteral const &lit) -> bool { return IsTest{}(lit); }

auto is_classical(HeadLiteral const &lit) -> bool { return IsClassical{}(lit); }

auto is_fact(SymbolStore &store, Rule const &rule) -> std::optional<Symbol> {
    if (auto const *head = std::get_if<SimpleHeadLiteral>(&rule.head); head != nullptr && rule.body.empty()) {
        if (auto const *lit = std::get_if<LiteralSymbolic>(&head->lit); lit != nullptr && lit->sign == Sign::none) {
            return IsFact{store}(lit->term);
        }
    }
    return std::nullopt;
}

auto is_fact(SymbolStore &store, Statement const &stm) -> std::optional<Symbol> {
    if (auto const *rule = std::get_if<Rule>(&stm); rule != nullptr) {
        return is_fact(store, *rule);
    }
    return std::nullopt;
}

} // namespace Gringo::Input
