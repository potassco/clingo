#pragma once

#include <optional>
#include <variant>
#include <vector>

#include <gringo/util/algorithm.hh>
#include <gringo/util/immutable_value.hh>
#include <gringo/util/optional.hh>

#include <gringo/input/program.hh>

namespace Gringo::Input {

namespace Detail {

template <class T, class U, class V = void> struct transformer_has_accept : std::false_type {};

template <class T, class U>
struct transformer_has_accept<T, U, std::void_t<decltype(std::declval<T const *>()->accept(std::declval<U>()))>>
    : std::true_type {};

} // namespace Detail

template <class U> struct TranslateArgument {
    U const &orig;
    std::optional<U> transformed = std::nullopt;
};

template <class T> class Transformer {
  public:
    template <class U> auto tr(U const &arg) const { return TranslateArgument<U>{arg}; }

    template <class U> auto transform(U const &x) const {
        if constexpr (Detail::transformer_has_accept<T, U>::value) {
            return static_cast<T const *>(this)->accept(x);
        } else {
            return accept_(x);
        }
    }

    template <class U, class... Args> auto transform_construct(Args &&...args) const -> std::optional<U> {
        (apply_(args), ...);
        if ((has_value_(args) || ...)) {
            return {U{get_value_(std::forward<Args>(args))...}};
        }
        return std::nullopt;
    }

    template <class U, class... Attr>
    auto transform_construct2(U const &expr, Attr... attrs) const -> std::optional<U> {
        return [&]<size_t... Indices>(std::index_sequence<Indices...> seq) {
            static_cast<void>(seq);
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

    template <class U> auto accept_(Util::immutable_array<U> const &vec) const -> std::optional<std::vector<U>> {
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

    template <class U> auto apply_(TranslateArgument<U> &arg) const { return arg.transformed = transform(arg.orig); }

    template <class U> auto apply_(U const &arg) const { static_cast<void>(arg); }

    template <class U> auto get_value_(TranslateArgument<U> &&arg) const {
        return std::move(arg.transformed).value_or(arg.orig);
    }

    template <class U> auto get_value_(U const &arg) const { return arg; }

    template <class U> auto has_value_(U const &arg) const -> bool {
        static_cast<void>(arg);
        return false;
    }

    template <class U> auto has_value_(TranslateArgument<U> const &arg) const -> bool {
        return arg.transformed.has_value();
    }

    // ignore

    [[nodiscard]] auto accept_(Projection const &x) const -> std::optional<Projection> {
        static_cast<void>(x);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(String const &x) const -> std::optional<String> {
        static_cast<void>(x);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(Relation const &x) const -> std::optional<Relation> {
        static_cast<void>(x);
        return std::nullopt;
    }

    // term

    [[nodiscard]] auto accept_(ArgumentTuple const &tuple) const -> std::optional<ArgumentTuple> {
        return transform_construct2(tuple, a_elems);
    }

    [[nodiscard]] auto accept_(TermSymbol const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(TermVariable const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(TermFunction const &term) const -> std::optional<Term> {
        return transform_construct2(term, a_pool);
    }

    [[nodiscard]] auto accept_(TermTuple const &term) const -> std::optional<Term> {
        return transform_construct2(term, a_pool);
    }

    [[nodiscard]] auto accept_(TermAbs const &term) const -> std::optional<Term> {
        return transform_construct2(term, a_pool);
    }

    [[nodiscard]] auto accept_(TermUnary const &term) const -> std::optional<Term> {
        return transform_construct2(term, a_rhs);
    }

    [[nodiscard]] auto accept_(TermBinary const &term) const -> std::optional<Term> {
        return transform_construct2(term, a_lhs, a_rhs);
    }

    // theory

    [[nodiscard]] auto accept_(TheoryTermUnparsed const &term) const -> std::optional<TheoryTerm> {
        return transform_construct2(term, a_elems);
    }

    [[nodiscard]] auto accept_(TheoryTermSymbol const &term) const -> std::optional<TheoryTerm> {
        static_cast<void>(term);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(TheoryTermVariable const &term) const -> std::optional<TheoryTerm> {
        static_cast<void>(term);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(TheoryTermTuple const &term) const -> std::optional<TheoryTerm> {
        return transform_construct2(term, a_elems);
    }

    [[nodiscard]] auto accept_(TheoryTermFunction const &term) const -> std::optional<TheoryTerm> {
        return transform_construct2(term, a_args);
    }

    // literal

    [[nodiscard]] auto accept_(LitBool const &lit) const -> std::optional<Lit> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(LitComparison const &lit) const -> std::optional<Lit> {
        return transform_construct2(lit, a_lhs, a_rhs);
    }

    [[nodiscard]] auto accept_(LitSymbolic const &lit) const -> std::optional<Lit> {
        return transform_construct2(lit, a_term);
    }

    // conditional literal

    [[nodiscard]] auto accept_(CondLit const &lit) const -> std::optional<CondLit> {
        return transform_construct2(lit, a_lit, a_cond);
    }

    // set aggregate

    [[nodiscard]] auto accept_(SetAggregateElement const &elem) const -> std::optional<SetAggregateElement> {
        return transform_construct2(elem, a_lit, a_cond);
    }

    // head literal

    [[nodiscard]] auto accept_(HdLitSimple const &lit) const -> std::optional<HdLit> { return transform(lit.lit()); }

    [[nodiscard]] auto accept_(HdLitDisjunction const &lit) const -> std::optional<HdLit> {
        return transform_construct2(lit, a_elems);
    }

    [[nodiscard]] auto accept_(HdLitSetAggregate const &lit) const -> std::optional<HdLit> {
        return transform_construct2(lit, a_lhs, a_elems, a_rhs);
    }

    [[nodiscard]] auto accept_(HdLitAggregateElement const &elem) const -> std::optional<HdLitAggregateElement> {
        return transform_construct2(elem, a_tuple, a_lit, a_cond);
    }

    [[nodiscard]] auto accept_(HdLitAggregate const &lit) const -> std::optional<HdLit> {
        return transform_construct2(lit, a_lhs, a_elems, a_rhs);
    }

    [[nodiscard]] auto accept_(TheoryElement const elem) const -> std::optional<TheoryElement> {
        return transform_construct2(elem, a_tuple, a_cond);
    }

    [[nodiscard]] auto accept_(HdLitTheoryAtom const &lit) const -> std::optional<HdLit> {
        return transform_construct2(lit, a_name, a_elems, a_rhs);
    }

    // body literal

    [[nodiscard]] auto accept_(BdLitSimple const &lit) const -> std::optional<BdLit> { return transform(lit.lit()); }

    [[nodiscard]] auto accept_(BdLitConjunction const &lit) const -> std::optional<BdLit> {
        return transform(lit.lit());
    }

    [[nodiscard]] auto accept_(BdLitSetAggregate const &lit) const -> std::optional<BdLit> {
        return transform_construct2(lit, a_lhs, a_elems, a_rhs);
    }

    [[nodiscard]] auto accept_(BdLitAggregateElement const &elem) const -> std::optional<BdLitAggregateElement> {
        return transform_construct2(elem, a_tuple, a_cond);
    }

    [[nodiscard]] auto accept_(BdLitAggregate const &lit) const -> std::optional<BdLit> {
        return transform_construct2(lit, a_lhs, a_elems, a_rhs);
    }

    [[nodiscard]] auto accept_(BdLitTheoryAtom const &lit) const -> std::optional<BdLit> {
        return transform_construct2(lit, a_name, a_elems, a_rhs);
    }

    // statement

    [[nodiscard]] auto accept_(StmRule const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_head, a_body);
    }

    [[nodiscard]] auto accept_(StmTheory const &stm) const -> std::optional<Stm> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(OptimizeTuple const &elem) const -> std::optional<OptimizeTuple> {
        return transform_construct2(elem, a_weight, a_prio, a_terms);
    }

    [[nodiscard]] auto accept_(StmOptimize const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_elems);
    }

    [[nodiscard]] auto accept_(StmWeakConstraint const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_body, a_tuple);
    }

    [[nodiscard]] auto accept_(StmShow const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_term, a_body);
    }

    [[nodiscard]] auto accept_(StmShowSig const &stm) const -> std::optional<Stm> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(StmProject const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_term, a_body);
    }

    [[nodiscard]] auto accept_(StmProjectSig const &stm) const -> std::optional<Stm> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(StmDefined const &stm) const -> std::optional<Stm> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(StmExternal const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_term, a_body, a_type);
    }

    [[nodiscard]] auto accept_(Edge const &edge) const -> std::optional<Edge> {
        return transform_construct2(edge, a_src, a_dst);
    }

    [[nodiscard]] auto accept_(StmEdge const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_edges, a_body);
    }

    [[nodiscard]] auto accept_(StmHeuristic const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_atom, a_body, a_weight, a_prio, a_type);
    }

    [[nodiscard]] auto accept_(StmScript const &stm) const -> std::optional<Stm> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(StmInclude const &stm) const -> std::optional<Stm> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(StmProgram const &stm) const -> std::optional<Stm> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(StmConst const &stm) const -> std::optional<Stm> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(StmComment const &stm) const -> std::optional<Stm> {
        static_cast<void>(stm);
        return std::nullopt;
    }
};

} // namespace Gringo::Input
