#pragma once

//! @file
//! This file contains utilities for visiting generic data structures.

#include <optional>
#include <tuple>
#include <variant>
#include <vector>

#include <gringo/input/program.hh>

namespace Gringo::Input {

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

    template <class... U, size_t... I>
    void accept_(std::tuple<U...> const &tup, std::index_sequence<I...> indices) const {
        static_cast<void>(indices);
        (visit(std::get<I>(tup)), ...);
    }

    template <class... U> void accept_(std::tuple<U...> const &tup) const {
        return accept_(tup, std::index_sequence_for<U...>{});
    }

    template <class... U> void accept_(std::variant<U...> const &var) const {
        return std::visit([this](auto const &elem) { this->visit(elem); }, var);
    }

    template <class U> void accept_(tcb::span<U> const &span) const {
        for (auto &elem : span) {
            visit(elem);
        }
    }

    template <class U> void accept_(std::vector<U> const &vec) const { visit(tcb::make_span(vec)); }

    template <class U> void accept_(Util::immutable_array<U> const &vec) const { visit(tcb::make_span(vec)); }

    // igonre

    void accept_(Projection const &x) const { static_cast<void>(x); }

    void accept_(String const &x) const { static_cast<void>(x); }

    void accept_(Relation const &x) const { static_cast<void>(x); }

    // terms

    void accept_(ArgumentTuple const &tuple) const { visit(tuple.elems()); }

    void accept_(TermSymbol const &term) const { static_cast<void>(term); }

    void accept_(TermVariable const &term) const { static_cast<void>(term); }

    void accept_(TermTuple const &term) const { visit(term.pool()); }

    void accept_(TermFunction const &term) const { visit(term.pool()); }

    void accept_(TermAbs const &term) const { visit(term.pool()); }

    void accept_(TermUnary const &term) const { visit(term.rhs()); }

    void accept_(TermBinary const &term) const { visit(term.lhs(), term.rhs()); }

    // theory terms

    void accept_(TheoryTermSymbol const &term) const { static_cast<void>(term); }

    void accept_(TheoryTermVariable const &term) const { static_cast<void>(term); }

    void accept_(TheoryTermTuple const &term) const { visit(term.elems_); }

    void accept_(TheoryTermFunction const &term) const { visit(term.args_); }

    void accept_(TheoryTermUnparsed const &term) const { visit(term.elems_); }

    // literals

    void accept_(LiteralBoolean const &lit) const { static_cast<void>(lit); }

    void accept_(LiteralRelation const &lit) const { visit(lit.lhs_, lit.rhs_); }

    void accept_(LiteralSymbolic const &lit) const { visit(lit.term_); }

    // conditional literal

    void accept_(ConditionalLiteral const &cond_lit) const { visit(cond_lit.lit_, cond_lit.cond_); }

    // aggregate

    void accept_(SetAggregateElement const &elem) const { visit(elem.lit_, elem.cond_); }

    template <bool HasSign> void accept_(SetAggregate<HasSign> const &lit) const {
        visit(lit.elems_);
        visit(lit.lhs_, lit.rhs_);
    }

    // theory

    void accept_(TheoryElement const &elem) const { visit(elem.tuple_, elem.cond_); }

    template <bool HasSign> void accept_(TheoryAtom<HasSign> const &atom) const {
        visit(atom.name_, atom.elems_, atom.rhs_);
    }

    // head literal

    void accept_(SimpleHeadLiteral const &lit) const { visit(lit.lit_); }

    void accept_(Disjunction const &lit) const { visit(lit.elems_); }

    void accept_(HeadAggregate::Element const &elem) const { visit(elem.tuple_, elem.lit_, elem.cond_); }

    void accept_(HeadAggregate const &lit) const { visit(lit.lhs_, lit.elems_, lit.rhs_); }

    // body literal

    void accept_(SimpleBodyLiteral const &lit) const { visit(lit.lit_); }

    void accept_(Conjunction const &lit) const { visit(lit.lit_); }

    void accept_(BodyAggregate::Element const &elem) const { visit(elem.tuple_, elem.cond_); }

    void accept_(BodyAggregate const &lit) const { visit(lit.lhs_, lit.elems_, lit.rhs_); }

    // statement

    void accept_(Rule const &stm) const { visit(stm.head_, stm.body_); }

    void accept_(TheoryDefinition const &stm) const { static_cast<void>(stm); }

    void accept_(StatementOptimize::Tuple const &tuple) const { visit(tuple.weight_, tuple.priority_, tuple.terms_); }

    void accept_(StatementOptimize const &stm) const { visit(stm.elems_); }

    void accept_(StatementWeakConstraint const &stm) const { visit(stm.body_, stm.tuple_); }

    void accept_(StatementShow const &stm) const { visit(stm.term_, stm.body_); }

    void accept_(StatementShowSig const &stm) const { static_cast<void>(stm); }

    void accept_(StatementProject const &stm) const { visit(stm.term_, stm.body_); }

    void accept_(StatementProjectSig const &stm) const { static_cast<void>(stm); }

    void accept_(StatementDefined const &stm) const { static_cast<void>(stm); }

    void accept_(StatementExternal const &stm) const { visit(stm.term_, stm.body_, stm.type_); }

    void accept_(StatementEdge::Edge const &edge) const { visit(edge.u_, edge.v_); }

    void accept_(StatementEdge const &stm) const { visit(stm.edges_, stm.body_); }

    void accept_(StatementHeuristic const &stm) const { visit(stm.atom_, stm.body_, stm.type_, stm.prio_, stm.mod_); }

    void accept_(StatementScript const &stm) const { static_cast<void>(stm); }

    void accept_(StatementInclude const &stm) const { static_cast<void>(stm); }

    void accept_(StatementProgram const &stm) const { static_cast<void>(stm); }

    void accept_(StatementConst const &stm) const { visit(stm.value_); }

    void accept_(Comment const &stm) const { static_cast<void>(stm); }
};

} // namespace Gringo::Input
