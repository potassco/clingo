#pragma once

//! @file
//! This file contains utilities for visiting generic data structures.

#include <optional>
#include <tuple>
#include <variant>

#include <clingo/util/type_traits.hh>

#include <clingo/input/program.hh>

namespace CppClingo::Input {

namespace Detail {

template <class T, class U, class V = void> struct visitor_has_accept : std::false_type {};

template <class T, class U>
struct visitor_has_accept<T, U, std::void_t<decltype(std::declval<T const *>()->accept(std::declval<U>()))>>
    : std::true_type {};

} // namespace Detail

//! A visitor that eases visiting programs (and standard types).
//!
//! The visitor visits expressions down to terms
//! but does not visit more specific types like theory definitions, etc.
//! Check the implementation.
template <class T> class Visitor {
  public:
    //! Recursively visit the given type.
    template <class U> void visit(U const &x) const {
        if constexpr (Detail::visitor_has_accept<T, U>::value) {
            static_cast<T const *>(this)->accept(x);
        } else {
            accept_(x);
        }
    }

    void operator()(auto const &x) const { visit(x); }

    //! Recursively visit all of the given arguments.
    template <class... U> void visit(U... args) const { (visit(args), ...); }

  private:
    template <class U> void accept_(std::optional<U> const &opt) const {
        if (opt.has_value()) {
            visit(opt.value());
        }
    }

    template <class U> auto accept_(Util::immutable_value<U> const &ptr) const { visit(*ptr); }

    template <class U, class V> auto accept_(std::pair<U, V> const &pair) const {
        visit(pair.first);
        visit(pair.second);
    }

    template <class... U> void accept_(std::tuple<U...> const &tup) const {
        return [&, this]<size_t... I>([[maybe_unused]] std::index_sequence<I...> indices) {
            (this->visit(std::get<I>(tup)), ...);
        }(std::index_sequence_for<U...>{});
    }

    template <class... U> void accept_(std::variant<U...> const &var) const {
        return std::visit([this](auto const &elem) { this->visit(elem); }, var);
    }

    template <class U> void accept_(Util::immutable_array<U> const &vec) const {
        for (auto &elem : vec) {
            visit(elem);
        }
    }

    // igonre

    template <class U>
        requires Util::is_among_v<U, Projection, FormatFieldLiteral, String, SharedString, Relation, TermSymbol,
                                  TermVariable, TheoryTermSymbol, TheoryTermVariable, LitBool, StmTheory,
                                  StmShowNothing, StmShowSig, StmProjectSig, StmDefined, StmScript, StmInclude,
                                  StmProgram, StmParts, StmComment>
    void accept_([[maybe_unused]] U const &x) const {}

    // terms

    void accept_(FormatFieldExpression const &term) const { visit(term.lhs()); }

    void accept_(TermFormatString const &term) const { visit(term.elems()); }

    void accept_(ArgumentTuple const &tuple) const { visit(tuple.elems()); }

    void accept_(TermTuple const &term) const { visit(term.pool()); }

    void accept_(TermFunction const &term) const { visit(term.pool()); }

    void accept_(TermAbs const &term) const { visit(term.pool()); }

    void accept_(TermUnary const &term) const { visit(term.rhs()); }

    void accept_(TermBinary const &term) const { visit(term.lhs(), term.rhs()); }

    // theory terms

    void accept_(UnparsedElement const &elem) const { visit(elem.term()); }

    void accept_(TheoryTermTuple const &term) const { visit(term.elems()); }

    void accept_(TheoryTermFunction const &term) const { visit(term.args()); }

    void accept_(TheoryTermUnparsed const &term) const { visit(term.elems()); }

    // literals

    void accept_(LitComparison const &lit) const { visit(lit.lhs(), lit.rhs()); }

    void accept_(LitSymbolic const &lit) const { visit(lit.term()); }

    // conditional literal

    void accept_(CondLit const &cond_lit) const { visit(cond_lit.lit(), cond_lit.cond()); }

    // aggregate

    void accept_(SetAggregateElement const &elem) const { visit(elem.lit(), elem.cond()); }

    template <bool HasSign> void accept_(SetAggregate<HasSign> const &lit) const {
        visit(lit.elems());
        visit(lit.lhs(), lit.rhs());
    }

    // theory

    void accept_(TheoryRGuard const &guard) const { visit(guard.term()); }

    void accept_(TheoryElement const &elem) const { visit(elem.tuple(), elem.cond()); }

    template <bool HasSign> void accept_(TheoryAtom<HasSign> const &atom) const {
        visit(atom.name(), atom.elems(), atom.rhs());
    }

    // head literal

    void accept_(HdLitSimple const &lit) const { visit(lit.lit()); }

    void accept_(HdLitDisjunction const &lit) const { visit(lit.elems()); }

    void accept_(HdLitAggregateElement const &elem) const { visit(elem.tuple(), elem.lit(), elem.cond()); }

    void accept_(HdLitAggregate const &lit) const { visit(lit.lhs(), lit.elems(), lit.rhs()); }

    // body literal

    void accept_(BdLitSimple const &lit) const { visit(lit.lit()); }

    void accept_(BdLitConjunction const &lit) const { visit(lit.lit()); }

    void accept_(BdLitAggregateElement const &elem) const { visit(elem.tuple(), elem.cond()); }

    void accept_(BdLitAggregate const &lit) const { visit(lit.lhs(), lit.elems(), lit.rhs()); }

    // statement

    void accept_(StmRule const &stm) const { visit(stm.head(), stm.body()); }

    void accept_(OptimizeTuple const &tuple) const { visit(tuple.weight(), tuple.prio(), tuple.terms()); }

    void accept_(OptimizeElement const &stm) const { visit(stm.tuple(), stm.cond()); }

    void accept_(StmOptimize const &stm) const { visit(stm.elems()); }

    void accept_(StmWeakConstraint const &stm) const { visit(stm.body(), stm.tuple()); }

    void accept_(StmShow const &stm) const { visit(stm.term(), stm.body()); }

    void accept_(StmProject const &stm) const { visit(stm.atom(), stm.body()); }

    void accept_(StmExternal const &stm) const { visit(stm.atom(), stm.body(), stm.type()); }

    void accept_(Edge const &edge) const { visit(edge.src(), edge.dst()); }

    void accept_(StmEdge const &stm) const { visit(stm.edges(), stm.body()); }

    void accept_(StmHeuristic const &stm) const { visit(stm.atom(), stm.body(), stm.weight(), stm.prio(), stm.type()); }

    void accept_(StmConst const &stm) const { visit(stm.value()); }
};

} // namespace CppClingo::Input
