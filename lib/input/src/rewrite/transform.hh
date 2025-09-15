#pragma once

#include <clingo/input/program.hh>

#include <clingo/util/algorithm.hh>
#include <clingo/util/immutable_value.hh>
#include <clingo/util/optional.hh>
#include <clingo/util/type_traits.hh>

#include <optional>
#include <variant>
#include <vector>

namespace CppClingo::Input {

namespace Detail {

template <class T, class U, class V = void> struct transformer_has_accept : std::false_type {};

template <class T, class U>
struct transformer_has_accept<T, U, std::void_t<decltype(std::declval<T const *>()->accept(std::declval<U>()))>>
    : std::true_type {};

} // namespace Detail

template <class T> class Transformer {
  public:
    template <class U> auto transform(U const &x) const {
        if constexpr (Detail::transformer_has_accept<T, U>::value) {
            return static_cast<T const *>(this)->accept(x);
        } else {
            return accept_(x);
        }
    }

    template <class U, class... Attr> auto rewrite(U const &expr, Attr... attrs) const -> std::optional<U> {
        return [&]<size_t... Indices>([[maybe_unused]] std::index_sequence<Indices...> seq) {
            auto args =
                std::make_tuple(std::optional<std::decay_t<decltype(expr.template get_value<Attr::tag>())>>{}...);
            ((std::get<Indices>(args) = transform(expr.template get_value<Attr::tag>())), ...);
            return expr.rewrite((attrs = std::move(get<Indices>(args)))...);
        }(std::index_sequence_for<Attr...>());
    }

  private:
    template <class U> auto accept_(std::optional<U> const &opt) const -> std::optional<std::optional<U>> {
        if (opt.has_value()) {
            // Note that the transformer will never remove an optional. If this
            // is desired, then the visitor extending the transformer should
            // handle this case.
            if (auto ret = transform(opt.value()); ret.has_value()) {
                return {std::move(ret)};
            }
        }
        return std::nullopt;
    }

    template <class U>
    auto accept_(Util::immutable_value<U> const &ptr) const -> std::optional<Util::immutable_value<U>> {
        return Util::transform(transform(*ptr), [](U val) { return Util::make_immutable<U>(std::move(val)); });
    }

    template <class U, class V> auto accept_(std::pair<U, V> const &pair) const -> std::optional<std::pair<U, V>> {
        auto first = transform(pair.first);
        auto second = transform(pair.second);
        if (first.has_value() || second.has_value()) {
            return std::pair<U, V>{std::move(first).value_or(pair.first), std::move(second).value_or(pair.second)};
        }
        return std::nullopt;
    }

    template <class... Args>
    auto accept_(std::tuple<Args...> const &tuple) const -> std::optional<std::tuple<Args...>> {
        return [&, this]<size_t... Indices>(std::index_sequence<Indices...>) -> std::optional<std::tuple<Args...>> {
            auto res = std::tuple{this->transform(std::get<Indices>(tuple))...};
            if ((std::get<Indices>(res).has_value() || ...)) {
                return std::tuple<Args...>{std::move(std::get<Indices>(res)).value_or(std::get<Indices>(tuple))...};
            }
            return std::nullopt;
        }(std::index_sequence_for<Args...>());
    }

    template <class... U> auto accept_(std::variant<U...> const &var) const -> std::optional<std::variant<U...>> {
        return std::visit(
            [this](auto const &x) -> std::optional<std::variant<U...>> {
                if (auto ret = this->transform(x); ret.has_value()) {
                    return std::variant<U...>{std::move(ret).value()};
                }
                return std::nullopt;
            },
            var);
    }

    template <class U>
    auto accept_(Util::immutable_array<U> const &vec) const -> std::optional<Util::immutable_array<U>> {
        size_t n = 0;
        std::optional<std::vector<std::remove_const_t<U>>> ret;
        for (auto const &elem : vec) {
            auto transformed = transform(elem);
            if (transformed && !ret.has_value()) {
                ret = Util::copy_n(vec, n);
            }
            if (ret.has_value()) {
                ret->emplace_back(std::move(transformed).value_or(elem));
            }
            ++n;
        }
        return ret;
    }

    // ignore

    template <class E>
        requires Util::is_among_v<E, Projection, String, SharedString, Relation>
    [[nodiscard]] auto accept_([[maybe_unused]] E const &term) const -> std::optional<E> {
        return std::nullopt;
    }

    // term

    [[nodiscard]] auto accept_(ArgumentTuple const &tuple) const -> std::optional<ArgumentTuple> {
        return rewrite(tuple, a_elems);
    }

    template <class E>
        requires Util::is_among_v<E, TermSymbol, TermVariable>
    [[nodiscard]] auto accept_([[maybe_unused]] E const &term) const -> std::optional<Term> {
        return std::nullopt;
    }

    template <class E>
        requires Util::is_among_v<E, TermFunction, TermTuple, TermAbs>
    [[nodiscard]] auto accept_([[maybe_unused]] E const &term) const -> std::optional<Term> {
        return rewrite(term, a_pool);
    }

    [[nodiscard]] auto accept_(FormatFieldExpression const &field) const -> std::optional<FormatFieldExpression> {
        return rewrite(field, a_lhs);
    }

    [[nodiscard]] auto accept_([[maybe_unused]] FormatFieldLiteral const &field) const
        -> std::optional<FormatFieldLiteral> {
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(TermFormatString const &term) const -> std::optional<Term> {
        return rewrite(term, a_elems);
    }

    [[nodiscard]] auto accept_(TermUnary const &term) const -> std::optional<Term> { return rewrite(term, a_rhs); }

    [[nodiscard]] auto accept_(TermBinary const &term) const -> std::optional<Term> {
        return rewrite(term, a_lhs, a_rhs);
    }

    // theory

    [[nodiscard]] auto accept_(UnparsedElement const &elem) const -> std::optional<UnparsedElement> {
        return rewrite(elem, a_term);
    }

    [[nodiscard]] auto accept_(TheoryTermUnparsed const &term) const -> std::optional<TheoryTerm> {
        return rewrite(term, a_elems);
    }

    template <class E>
        requires Util::is_among_v<E, TheoryTermSymbol, TheoryTermVariable>
    [[nodiscard]] auto accept_([[maybe_unused]] E const &stm) const -> std::optional<TheoryTerm> {
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(TheoryTermTuple const &term) const -> std::optional<TheoryTerm> {
        return rewrite(term, a_elems);
    }

    [[nodiscard]] auto accept_(TheoryTermFunction const &term) const -> std::optional<TheoryTerm> {
        return rewrite(term, a_args);
    }

    [[nodiscard]] auto accept_(TheoryRGuard const &guard) const -> std::optional<TheoryRGuard> {
        return rewrite(guard, a_term);
    }

    // literal

    [[nodiscard]] auto accept_([[maybe_unused]] LitBool const &lit) const -> std::optional<Lit> { return std::nullopt; }

    [[nodiscard]] auto accept_(LitComparison const &lit) const -> std::optional<Lit> {
        return rewrite(lit, a_lhs, a_rhs);
    }

    [[nodiscard]] auto accept_(LitSymbolic const &lit) const -> std::optional<Lit> { return rewrite(lit, a_term); }

    // conditional literal

    [[nodiscard]] auto accept_(CondLit const &lit) const -> std::optional<CondLit> {
        return rewrite(lit, a_lit, a_cond);
    }

    // set aggregate

    [[nodiscard]] auto accept_(SetAggregateElement const &elem) const -> std::optional<SetAggregateElement> {
        return rewrite(elem, a_lit, a_cond);
    }

    // head literal

    [[nodiscard]] auto accept_(HdLitSimple const &lit) const -> std::optional<HdLit> { return transform(lit.lit()); }

    [[nodiscard]] auto accept_(HdLitDisjunction const &lit) const -> std::optional<HdLit> {
        return rewrite(lit, a_elems);
    }

    [[nodiscard]] auto accept_(HdLitSetAggregate const &lit) const -> std::optional<HdLit> {
        return rewrite(lit, a_lhs, a_elems, a_rhs);
    }

    [[nodiscard]] auto accept_(HdLitAggregateElement const &elem) const -> std::optional<HdLitAggregateElement> {
        return rewrite(elem, a_tuple, a_lit, a_cond);
    }

    [[nodiscard]] auto accept_(HdLitAggregate const &lit) const -> std::optional<HdLit> {
        return rewrite(lit, a_lhs, a_elems, a_rhs);
    }

    [[nodiscard]] auto accept_(TheoryElement const elem) const -> std::optional<TheoryElement> {
        return rewrite(elem, a_tuple, a_cond);
    }

    [[nodiscard]] auto accept_(HdLitTheoryAtom const &lit) const -> std::optional<HdLit> {
        return rewrite(lit, a_name, a_elems, a_rhs);
    }

    // body literal

    [[nodiscard]] auto accept_(BdLitSimple const &lit) const -> std::optional<BdLit> { return transform(lit.lit()); }

    [[nodiscard]] auto accept_(BdLitConjunction const &lit) const -> std::optional<BdLit> {
        return transform(lit.lit());
    }

    [[nodiscard]] auto accept_(BdLitSetAggregate const &lit) const -> std::optional<BdLit> {
        return rewrite(lit, a_lhs, a_elems, a_rhs);
    }

    [[nodiscard]] auto accept_(BdLitAggregateElement const &elem) const -> std::optional<BdLitAggregateElement> {
        return rewrite(elem, a_tuple, a_cond);
    }

    [[nodiscard]] auto accept_(BdLitAggregate const &lit) const -> std::optional<BdLit> {
        return rewrite(lit, a_lhs, a_elems, a_rhs);
    }

    [[nodiscard]] auto accept_(BdLitTheoryAtom const &lit) const -> std::optional<BdLit> {
        return rewrite(lit, a_name, a_elems, a_rhs);
    }

    // statement

    [[nodiscard]] auto accept_(StmRule const &stm) const -> std::optional<Stm> { return rewrite(stm, a_head, a_body); }

    [[nodiscard]] auto accept_(OptimizeTuple const &elem) const -> std::optional<OptimizeTuple> {
        return rewrite(elem, a_weight, a_prio, a_terms);
    }

    [[nodiscard]] auto accept_(OptimizeElement const &elem) const -> std::optional<OptimizeElement> {
        return rewrite(elem, a_tuple, a_cond);
    }

    [[nodiscard]] auto accept_(StmOptimize const &stm) const -> std::optional<Stm> { return rewrite(stm, a_elems); }

    [[nodiscard]] auto accept_(StmWeakConstraint const &stm) const -> std::optional<Stm> {
        return rewrite(stm, a_body, a_tuple);
    }

    [[nodiscard]] auto accept_(StmShow const &stm) const -> std::optional<Stm> { return rewrite(stm, a_term, a_body); }

    [[nodiscard]] auto accept_(StmProject const &stm) const -> std::optional<Stm> {
        return rewrite(stm, a_atom, a_body);
    }

    [[nodiscard]] auto accept_(StmExternal const &stm) const -> std::optional<Stm> {
        return rewrite(stm, a_atom, a_body, a_type);
    }

    [[nodiscard]] auto accept_(Edge const &edge) const -> std::optional<Edge> { return rewrite(edge, a_src, a_dst); }

    [[nodiscard]] auto accept_(StmEdge const &stm) const -> std::optional<Stm> { return rewrite(stm, a_edges, a_body); }

    [[nodiscard]] auto accept_(StmHeuristic const &stm) const -> std::optional<Stm> {
        return rewrite(stm, a_atom, a_body, a_weight, a_prio, a_type);
    }

    template <class E>
        requires Util::is_among_v<E, StmTheory, StmShowNothing, StmShowSig, StmProjectSig, StmDefined, StmScript,
                                  StmInclude, StmProgram, StmConst, StmParts, StmComment>
    [[nodiscard]] auto accept_([[maybe_unused]] E const &stm) const -> std::optional<Stm> {
        return std::nullopt;
    }
};

} // namespace CppClingo::Input
