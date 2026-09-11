#include <clingo/input/print.hh>

#include <clingo/input/rewrite/analyze.hh>
#include <clingo/input/rewrite/evaluate.hh>
#include <clingo/input/rewrite/visit_variables.hh>

#include <clingo/util/algorithm.hh>
#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>

#include <algorithm>

#include "visit.hh"

namespace CppClingo::Input {

namespace {

//! Check if a given term matches a specific type or pattern, such as positive
//! number, atom, identifier, or signed identifier.
//!
//! It uses `std::visit` to recursively inspect the term's structure and type,
//! optionally storing information (like the matched identifier or sign) in a
//! result struct. It returns `true` if the term matches the requested type,
//! otherwise `false`.
struct CheckType {
    auto operator()(Term const &term) const -> bool { return std::visit(*this, term); }

    auto operator()(TermSymbol const &term) const -> bool {
        switch (term.value().type()) {
            case SymbolType::number: {
                auto const &num = term.value().num();
                if (type == TermCheckType::pos_number && num >= 0) {
                    if (res != nullptr) {
                        res->pos_number = &num;
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

    auto operator()(TermUnary const &term) const -> bool {
        if (type == TermCheckType::atom) {
            return term.op() == UnaryOperator::minus && std::visit(*this, *term.rhs());
        }
        if (type == TermCheckType::signed_identifier && term.op() == UnaryOperator::minus &&
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

    template <class T> auto operator()([[maybe_unused]] T const &x) const -> bool {
        static_assert(Util::is_among_v<T, TermVariable, TermTuple, TermAbs, TermFormatString>);
        return false;
    }

    TermCheckType type;
    CheckTypeResult *res;
};

struct AlwaysNumeric {
    auto operator()(Term const &term) const -> bool { return std::visit(*this, term); }

    auto operator()(TermSymbol const &term) const -> bool { return term.value().type() == SymbolType::number; }

    auto operator()(TermTuple const &term) const -> bool {
        return term.pool().size() == 1 && std::holds_alternative<ArgumentTuple>(term.pool().front());
    }

    auto operator()(TermUnary const &term) const -> bool {
        return term.op() == UnaryOperator::negate || std::visit(*this, *term.rhs());
    }

    template <class T> auto operator()([[maybe_unused]] T const &term) const -> bool {
        if constexpr (Util::is_among_v<T, TermAbs, TermBinary>) {
            return true;
        } else {
            static_assert(Util::is_among_v<T, TermVariable, TermFunction, TermFormatString>);
            return false;
        }
    }
};

struct NeverNumeric {
    auto operator()(Term const &term) const -> bool { return std::visit(*this, term); }

    auto operator()(TermSymbol const &term) const -> bool { return term.value().type() != SymbolType::number; }

    auto operator()(TermTuple const &term) const -> bool {
        return term.pool().size() == 1 && std::holds_alternative<ArgumentTuple>(term.pool().front());
    }

    auto operator()(TermUnary const &term) const -> bool {
        return term.op() == UnaryOperator::minus && std::visit(*this, *term.rhs());
    }

    template <class T> auto operator()([[maybe_unused]] T const &term) const -> bool {
        if constexpr (Util::is_among_v<T, TermAbs, TermBinary, TermVariable>) {
            return false;
        } else {
            static_assert(Util::is_among_v<T, TermFunction, TermFormatString>);
            return true;
        }
    }
};

struct IsMatchable {
    auto operator()(TermFunction const &term) const -> bool {
        return !term.external() && std::ranges::all_of(term.pool(), *this);
    }

    auto operator()(TermTuple const &term) const -> bool { return std::ranges::all_of(term.pool(), *this); }

    auto operator()(ArgumentTuple const &tuple) const -> bool { return std::ranges::all_of(tuple.elems(), *this); }

    auto operator()(TermUnary const &term) const -> bool {
        return term.op() == UnaryOperator::minus && std::visit(*this, *term.rhs());
    }

    auto operator()(TermBinary const &term) const -> bool { return is_linear(term); }

    template <class T> auto operator()(T const &term) const -> bool {
        if constexpr (Util::is_among_v<T, Term, Argument, TupleElement>) {
            return std::visit(*this, term);
        } else if constexpr (Util::is_among_v<T, TermAbs, TermFormatString>) {
            return false;
        } else if constexpr (Util::is_among_v<T, TermSymbol, Projection, TermVariable>) {
            return true;
        } else {
            static_assert(Util::is_among_v<T, void>);
            return false;
        }
    }
};

struct IsTest {
    template <class T> auto operator()([[maybe_unused]] T const &lit) const -> bool {
        if constexpr (Util::is_among_v<T, Lit, BdLit>) {
            return std::visit(*this, lit);
        } else if constexpr (Util::is_among_v<T, Lit, BdLitSimple>) {
            return operator()(lit.lit());
        } else if constexpr (Util::is_among_v<T, LitBool, LitComparison>) {
            return true;
        } else {
            static_assert(Util::is_among_v<T, BdLitConjunction, BdLitSetAggregate, BdLitAggregate, BdLitSort,
                                           BdLitTheoryAtom, LitSymbolic>);
            return false;
        }
    }
};

struct IsAtom {
    template <class T> auto operator()([[maybe_unused]] T const &lit) const -> bool {
        if constexpr (Util::is_among_v<T, LitBool, LitComparison, HdLitSetAggregate, BdLitSetAggregate, HdLitAggregate,
                                       HdLitTheoryAtom, BdLitConjunction, BdLitAggregate, BdLitSort, BdLitTheoryAtom>) {
            return false;
        } else if constexpr (Util::is_among_v<T, HdLitSimple, BdLitSimple>) {
            return operator()(lit.lit());
        } else {
            static_assert(Util::is_among_v<T, Lit, HdLit, BdLit>);
            return std::visit(*this, lit);
        }
    }

    auto operator()(LitSymbolic const &lit) const -> bool { return lit.sign() == Sign::none; }

    auto operator()(HdLitDisjunction const &lit) const -> bool {
        if (lit.elems().size() != 1) {
            return false;
        }
        auto const *front = std::get_if<Lit>(&lit.elems().front());
        return front != nullptr && operator()(*front);
    }
};

struct IsClassical {
    auto operator()(HdLit const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(HdLitSimple const &lit) const -> bool { return !is_atom(lit.lit()); }

    auto operator()(HdLitDisjunction const &lit) const -> bool {
        return std::ranges::all_of(lit.elems(), [](auto const &elem) {
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

    template <class T> auto operator()([[maybe_unused]] T const &lit) const -> bool {
        static_assert(Util::is_among_v<T, HdLitAggregate, HdLitSetAggregate, HdLitTheoryAtom>);
        return true;
    }
};

//! The IsFact checks if a given term can be represented as a symbol.
//!
//! It recursively visits terms, attempting to construct a `Symbol`. If
//! successful, it returns the corresponding `Symbol`; otherwise, it returns
//! `std::nullopt`.
class IsFact {
  public:
    IsFact(SymbolStore &store) : store_{&store} {}

    template <class T> auto operator()(T const &x) const -> bool = delete;

    auto operator()(Term const &term) const -> std::optional<Symbol> { return std::visit(*this, term); }

    auto operator()([[maybe_unused]] TermVariable const &term) const -> std::optional<Symbol> { return std::nullopt; }

    auto operator()(TermSymbol const &term) const -> std::optional<Symbol> { return term.value(); }

    auto operator()(TermFormatString const &term) const -> std::optional<Symbol> {
        auto res = std::string{};
        for (auto const &field : term.elems()) {
            if (!std::visit(
                    [&res]<class T>(T const &x) {
                        if constexpr (Util::is_among_v<T, FormatFieldLiteral>) {
                            res += x.value().view();
                            return true;
                        } else if constexpr (std::is_same_v<T, FormatFieldExpression>) {
                            return false;
                        } else {
                            static_assert(Util::is_among_v<T, void>);
                        }
                    },
                    field)) {
                return std::nullopt;
            }
        }
        return SymbolStore::str_ref(store_->string_ref(res));
    }

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
            res.emplace_back(*res_term);
        }
        return res;
    }

    auto operator()(TermFunction const &term) const -> std::optional<Symbol> {
        if (term.pool().size() != 1 || term.external()) {
            return std::nullopt;
        }
        if (auto res_args = operator()(term.pool().front()); res_args) {
            return store_->fun_ref(term.name(), res_args.value(), false);
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
                        return store_->tup_ref(*tuple);
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
            return store_->num_ref(abs(arg->num()));
        }
        return std::nullopt;
    }

    auto operator()(TermUnary const &term) const -> std::optional<Symbol> {
        if (auto rhs = operator()(*term.rhs()); rhs) {
            return evaluate(*store_, term.op(), rhs.value());
        }
        return std::nullopt;
    }

    auto operator()(TermBinary const &term) const -> std::optional<Symbol> {
        if (term.op() == BinaryOperator::dots) {
            return std::nullopt;
        }
        if (auto lhs = operator()(*term.lhs()), rhs = operator()(*term.rhs()); lhs && rhs) {
            return evaluate(*store_, *lhs, term.op(), *rhs);
        }
        return std::nullopt;
    }

  private:
    SymbolStore *store_;
};

struct GetSignature {
    auto operator()(TermFunction const &term) -> std::optional<std::tuple<String, size_t, bool>> {
        if (term.pool().size() == 1) {
            return std::tuple{term.name(), term.pool().front().elems().size(), false};
        }
        return std::nullopt;
    }
    auto operator()(TermUnary const &term) -> std::optional<std::tuple<String, size_t, bool>> {
        if (term.op() == UnaryOperator::minus) {
            return Util::transform(std::visit(*this, *term.rhs()), [](auto sig) {
                return std::tuple{std::get<0>(sig), std::get<1>(sig), !std::get<2>(sig)};
            });
        }
        return std::nullopt;
    }
    auto operator()(TermSymbol const &term) -> std::optional<std::tuple<String, size_t, bool>> {
        auto val = term.value();
        if (val.type() == SymbolType::function) {
            return std::tuple{val.name(), val.args().size(), val.has_sign()};
        }
        return std::nullopt;
    }
    auto operator()([[maybe_unused]] auto const &term) -> std::optional<std::tuple<String, size_t, bool>> {
        return std::nullopt;
    }
};

class ExtractCanFail : public Visitor<ExtractCanFail> {
  public:
    ExtractCanFail(std::vector<Term> &result) : result{&result} {}

    void accept(TermFunction const &term) const {
        if (term.external()) {
            result->emplace_back(term);
        } else {
            visit(term.pool());
        }
    }

    void accept(TermAbs const &term) const { result->emplace_back(term); }

    void accept(TermUnary const &term) const {
        if (is_symbol(*term.rhs())) {
            visit(*term.rhs());
        } else {
            result->emplace_back(term);
        }
    }

    void accept(TermBinary const &term) const { result->emplace_back(term); }

  private:
    std::vector<Term> *result;
};

} // namespace

auto check_type(Term const &term, TermCheckType type, CheckTypeResult *res) -> bool {
    return CheckType{type, res}(term);
}

[[nodiscard]] auto is_linear(TermBinary const &term) -> bool {
    if (term.op() != BinaryOperator::plus) {
        return false;
    }
    auto const *mul = std::get_if<TermBinary>(&term.lhs().get());
    if (mul == nullptr || mul->op() != BinaryOperator::times) {
        return false;
    }
    auto const *n = std::get_if<TermSymbol>(&term.rhs().get());
    if (n == nullptr || n->value().type() != SymbolType::number) {
        return false;
    }
    auto const *m = std::get_if<TermSymbol>(&mul->lhs().get());
    if (m == nullptr || m->value().type() != SymbolType::number || m->value().num() == 0) {
        return false;
    }
    return std::get_if<TermVariable>(&mul->rhs().get()) != nullptr;
}

[[nodiscard]] auto is_linear(Term const &term) -> bool {
    auto const *bin = std::get_if<TermBinary>(&term);
    return bin != nullptr && is_linear(*bin);
}

//! Check if the given term is a linear term and return an object to access its elements.
auto check_linear(TermBinary const &term) -> std::optional<LinearTerm> {
    if (is_linear(term)) {
        return LinearTerm{term};
    }
    return std::nullopt;
}

//! Check if the given term is a linear term and return an object to access its elements.
auto check_linear(Term const &term) -> std::optional<LinearTerm> {
    if (auto const *bin = std::get_if<TermBinary>(&term); bin != nullptr) {
        return check_linear(*bin);
    }
    return std::nullopt;
}

[[nodiscard]] auto is_interval(Term const &term) -> bool {
    auto const *bin = std::get_if<TermBinary>(&term);
    return bin != nullptr && is_interval(*bin);
}

[[nodiscard]] auto is_external(Term const &term) -> bool {
    auto const *fun = std::get_if<TermFunction>(&term);
    return fun != nullptr && fun->external();
}

[[nodiscard]] auto is_interval(TermBinary const &term) -> bool {
    return term.op() == BinaryOperator::dots;
}

[[nodiscard]] auto always_numeric(Term const &term) -> bool {
    return AlwaysNumeric{}(term);
}

[[nodiscard]] auto never_numeric(Term const &term) -> bool {
    return NeverNumeric{}(term);
}

auto is_atom(Term const &term) -> bool {
    return std::visit(
        []<class T>(T const &term) -> bool {
            auto is_pos_atom = []<class U>(U const &term) -> bool {
                if constexpr (Util::matches<U, TermFunction>) {
                    return true;
                } else if constexpr (Util::matches<U, TermSymbol>) {
                    return term.value().type() == SymbolType::function;
                } else {
                    return false;
                }
            };
            if constexpr (Util::matches<T, TermUnary>) {
                return std::visit(is_pos_atom, *term.rhs());
            } else {
                return is_pos_atom(term);
            }
        },
        term);
}

auto is_atom(Lit const &lit) -> bool {
    return IsAtom{}(lit);
}

auto is_atom(HdLit const &lit) -> bool {
    return IsAtom{}(lit);
}

auto is_atom(BdLit const &lit) -> bool {
    return IsAtom{}(lit);
}

auto is_test(Lit const &lit) -> bool {
    return IsTest{}(lit);
}

auto is_test(BdLit const &lit) -> bool {
    return IsTest{}(lit);
}

auto is_classical(HdLit const &lit) -> bool {
    return IsClassical{}(lit);
}

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
        [&]([[maybe_unused]] Location const &loc, String var) {
            if (!var.starts_with("$") && global.contains(var) != new_global.contains(var)) {
                unsafe.emplace_back(var);
            }
        },
        VariableContext::all);
    if (!unsafe.empty()) {
        std::ranges::sort(unsafe);
        unsafe.erase(std::ranges::unique(unsafe).begin(), unsafe.end());
        CLINGO_REPORT_LOC(log, error, location(stm)) << "unsafe variables in:\n"
                                                     << "  " << stm << "\n"
                                                     << "note: the following variables are unsafe:\n"
                                                     << "  " << Util::p_range(unsafe, ", ");
        return false;
    }
    return true;
}

auto is_matchable(Term const &term) -> bool {
    return std::visit(IsMatchable{}, term);
}

auto signature(Term const &term) -> std::optional<std::tuple<String, size_t, bool>> {
    return std::visit(GetSignature{}, term);
}

void extract_can_fail(Term const &term, std::vector<Term> &result) {
    ExtractCanFail{result}.visit(term);
}

} // namespace CppClingo::Input
