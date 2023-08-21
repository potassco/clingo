#include <input/algo/check_type.hh>

#include <util/algorithm.hh>

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
                if (type == TermCheckType::pos_number && num >= 0) {
                    if (res != nullptr) {
                        res->pos_number = num;
                    }
                    return true;
                }
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
        for (auto const &elem : lit.elems) {
            for (auto const &lit : elem.lits) {
                if (is_atom(lit)) {
                    return false;
                }
            }
        }
        return true;
    }
};

} // namespace

auto check_type(Term const &term, TermCheckType type, CheckTypeResult *res) -> bool {
    return CheckType{type, res}(term);
}

auto is_atom(Literal const &lit) -> bool { return IsAtom{}(lit); }

auto is_atom(HeadLiteral const &lit) -> bool { return IsAtom{}(lit); }

auto is_atom(BodyLiteral const &lit) -> bool { return IsAtom{}(lit); }

auto is_test(Literal const &lit) -> bool { return IsTest{}(lit); }

auto is_test(HeadLiteral const &lit) -> bool { return IsTest{}(lit); }

auto is_test(BodyLiteral const &lit) -> bool { return IsTest{}(lit); }

auto is_classical(HeadLiteral const &lit) -> bool { return IsClassical{}(lit); }

} // namespace Gringo::Input
