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

  private:
    template <class... U, size_t... I>
    auto accept_(std::tuple<U...> const &tup, std::index_sequence<I...> indices, std::optional<U>... transfomed) const
        -> std::optional<std::tuple<U...>> {
        static_cast<void>(indices);
        if ((transfomed.has_value() || ...)) {
            return {std::move(transfomed).value_or(std::get<I>(tup))...};
        }
        return std::nullopt;
    }

    template <size_t i, class... U, class... Args>
    auto accept_(std::tuple<U...> const &tup, Args &&...args) const -> std::optional<std::tuple<U...>> {
        // Note: we have to use the complicated recursive version because the
        // C++ standard leaves the order of argument evaluation unspecified.
        if constexpr (i == sizeof...(U)) {
            return accept_(tup, std::forward<Args>(args)..., std::index_sequence_for<U...>{});
        } else {
            return accept_(tup, std::forward<Args>(args)..., transform(std::get<i>(tup)));
        }
    }

    template <class... U> auto accept_(std::tuple<U...> const &tup) const -> std::optional<std::tuple<U...>> {
        return accept_<0>(tup);
    }

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
        return transform_construct<ArgumentTuple>(tr(tuple.elems()));
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
        return transform_construct<TermFunction>(term.loc(), term.name(), tr(term.pool()), term.external());
    }

    [[nodiscard]] auto accept_(TermTuple const &term) const -> std::optional<Term> {
        return transform_construct<TermTuple>(term.loc(), tr(term.pool()));
    }

    [[nodiscard]] auto accept_(TermAbs const &term) const -> std::optional<Term> {
        return transform_construct<TermAbs>(term.loc(), tr(term.pool()));
    }

    [[nodiscard]] auto accept_(TermUnary const &term) const -> std::optional<Term> {
        return transform_construct<TermUnary>(term.loc(), term.op(), tr(term.rhs()));
    }

    [[nodiscard]] auto accept_(TermBinary const &term) const -> std::optional<Term> {
        return transform_construct<TermBinary>(term.loc(), tr(term.lhs()), term.op(), tr(term.rhs()));
    }

    // theory

    [[nodiscard]] auto accept_(TheoryTermUnparsed const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermUnparsed>(term.loc(), tr(term.elems()));
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
        return transform_construct<TheoryTermTuple>(term.loc(), term.type(), tr(term.elems()));
    }

    [[nodiscard]] auto accept_(TheoryTermFunction const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermFunction>(term.loc(), term.name(), tr(term.args()));
    }

    // literal

    [[nodiscard]] auto accept_(LitBool const &lit) const -> std::optional<Lit> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(LitComparison const &lit) const -> std::optional<Lit> {
        return transform_construct<LitComparison>(lit.loc(), lit.sign(), tr(lit.lhs()), tr(lit.rhs()));
    }

    [[nodiscard]] auto accept_(LitSymbolic const &lit) const -> std::optional<Lit> {
        return transform_construct<LitSymbolic>(lit.loc(), lit.sign(), tr(lit.term()));
    }

    // conditional literal

    [[nodiscard]] auto accept_(CondLit const &lit) const -> std::optional<CondLit> {
        return transform_construct<CondLit>(lit.loc(), tr(lit.lit()), tr(lit.cond()));
    }

    // set aggregate

    [[nodiscard]] auto accept_(SetAggregateElement const &elem) const -> std::optional<SetAggregateElement> {
        return transform_construct<SetAggregateElement>(elem.loc(), tr(elem.lit()), tr(elem.cond()));
    }

    // head literal

    [[nodiscard]] auto accept_(HdLitSimple const &lit) const -> std::optional<HdLit> { return transform(lit.lit()); }

    [[nodiscard]] auto accept_(HdLitDisjunctionElement const &elem) const -> std::optional<HdLitDisjunctionElement> {
        return std::visit(
            [this](auto const &elem) -> std::optional<HdLitDisjunctionElement> { return this->transform(elem); }, elem);
    }

    [[nodiscard]] auto accept_(HdLitDisjunction const &lit) const -> std::optional<HdLit> {
        return transform_construct<HdLitDisjunction>(lit.loc(), tr(lit.elems()));
    }

    [[nodiscard]] auto accept_(HdLitSetAggregate const &lit) const -> std::optional<HdLit> {
        return transform_construct<HdLitSetAggregate>(lit.loc(), tr(lit.lhs()), tr(lit.elems()), tr(lit.rhs()));
    }

    [[nodiscard]] auto accept_(HdLitAggregateElement const &elem) const -> std::optional<HdLitAggregateElement> {
        return transform_construct<HdLitAggregateElement>(elem.loc(), tr(elem.tuple()), tr(elem.lit()),
                                                          tr(elem.cond()));
    }

    [[nodiscard]] auto accept_(HdLitAggregate const &lit) const -> std::optional<HdLit> {
        return transform_construct<HdLitAggregate>(lit.loc(), tr(lit.lhs()), lit.fun(), tr(lit.elems()), tr(lit.rhs()));
    }

    [[nodiscard]] auto accept_(TheoryElement const elem) const -> std::optional<TheoryElement> {
        return transform_construct<TheoryElement>(elem.loc(), tr(elem.tuple()), tr(elem.cond()));
    }

    [[nodiscard]] auto accept_(HdLitTheoryAtom const &lit) const -> std::optional<HdLit> {
        return transform_construct<HdLitTheoryAtom>(lit.loc(), tr(lit.name()), tr(lit.elems()), tr(lit.rhs()));
    }

    // body literal

    [[nodiscard]] auto accept_(BdLitSimple const &lit) const -> std::optional<BdLit> { return transform(lit.lit()); }

    [[nodiscard]] auto accept_(BdLitConjunction const &lit) const -> std::optional<BdLit> {
        return transform(lit.lit());
    }

    [[nodiscard]] auto accept_(BdLitSetAggregate const &lit) const -> std::optional<BdLit> {
        return transform_construct<BdLitSetAggregate>(lit.loc(), lit.sign(), tr(lit.lhs()), tr(lit.elems()),
                                                      tr(lit.rhs()));
    }

    [[nodiscard]] auto accept_(BdLitAggregateElement const &elem) const -> std::optional<BdLitAggregateElement> {
        return transform_construct<BdLitAggregateElement>(elem.loc(), tr(elem.tuple()), tr(elem.cond()));
    }

    [[nodiscard]] auto accept_(BdLitAggregate const &lit) const -> std::optional<BdLit> {
        return transform_construct<BdLitAggregate>(lit.loc(), lit.sign(), tr(lit.lhs()), lit.fun(), tr(lit.elems()),
                                                   tr(lit.rhs()));
    }

    [[nodiscard]] auto accept_(BdLitTheoryAtom const &lit) const -> std::optional<BdLit> {
        return transform_construct<BdLitTheoryAtom>(lit.loc(), lit.sign(), tr(lit.name()), tr(lit.elems()),
                                                    tr(lit.rhs()));
    }

    // statement

    [[nodiscard]] auto accept_(StmRule const &stm) const -> std::optional<Stm> {
        return transform_construct<StmRule>(stm.loc(), tr(stm.head()), tr(stm.body()));
    }

    [[nodiscard]] auto accept_(StmTheory const &stm) const -> std::optional<Stm> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(OptimizeTuple const &elem) const -> std::optional<OptimizeTuple> {
        return transform_construct<OptimizeTuple>(tr(elem.weight()), tr(elem.prio()), tr(elem.terms()));
    }

    [[nodiscard]] auto accept_(OptimizeElement const &elem) const -> std::optional<OptimizeElement> {
        return transform_construct<OptimizeElement>(tr(elem.first), tr(elem.second));
    }

    [[nodiscard]] auto accept_(StmOptimize const &stm) const -> std::optional<Stm> {
        return transform_construct<StmOptimize>(stm.loc(), stm.type(), tr(stm.elems()));
    }

    [[nodiscard]] auto accept_(StmWeakConstraint const &stm) const -> std::optional<Stm> {
        return transform_construct<StmWeakConstraint>(stm.loc(), tr(stm.body()), tr(stm.tuple()));
    }

    [[nodiscard]] auto accept_(StmShow const &stm) const -> std::optional<Stm> {
        return transform_construct<StmShow>(stm.loc(), tr(stm.term()), tr(stm.body()));
    }

    [[nodiscard]] auto accept_(StmShowSig const &stm) const -> std::optional<Stm> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    [[nodiscard]] auto accept_(StmProject const &stm) const -> std::optional<Stm> {
        return transform_construct<StmProject>(stm.loc(), tr(stm.term()), tr(stm.body()));
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
        return transform_construct<StmExternal>(stm.loc(), tr(stm.term()), tr(stm.body()), tr(stm.type()));
    }

    [[nodiscard]] auto accept_(Edge const &edge) const -> std::optional<Edge> {
        return transform_construct<Edge>(tr(edge.src()), tr(edge.dst()));
    }

    [[nodiscard]] auto accept_(StmEdge const &stm) const -> std::optional<Stm> {
        return transform_construct<StmEdge>(stm.loc(), tr(stm.edges()), tr(stm.body()));
    }

    [[nodiscard]] auto accept_(StmHeuristic const &stm) const -> std::optional<Stm> {
        return transform_construct<StmHeuristic>(stm.loc(), tr(stm.atom()), tr(stm.body()), tr(stm.weight()),
                                                 tr(stm.prio()), tr(stm.type()));
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
