#pragma once

#include <input/head_literal.hh>

#include "theory.hh"

namespace Gringo::Input::Grammar {

namespace Detail {

inline auto construct_head_aggr(Term term, Relation rel, HeadAggregate aggr) -> HeadAggregate {
    aggr.loc.begin = location(term).begin;
    aggr.lhs = LGuard::value_type{std::move(term), rel};
    return aggr;
}

inline auto construct_head_aggr(Term term, Relation rel, HeadSetAggregate aggr) -> HeadSetAggregate {
    auto loc = location(term) + aggr.loc;
    aggr.lhs = LGuard::value_type{std::move(term), rel};
    return aggr;
}

} // namespace Detail

static constexpr auto disjunction_sep = LEXY_ASCII_ONE_OF(",;|");

struct simple_disjunction_element {
    static constexpr char const *name = "disjunction element";
    static constexpr auto rule = dsl::opt(dsl::list(disjunction_sep >> dsl::p<conditional_literal>));
    static constexpr auto value = lexy::as_list<ConditionalLiteralVec>;
};

struct simple_disjunction {
    static constexpr char const *name = "disjunction";
    static constexpr auto rule = dsl::list(dsl::p<conditional_literal>, dsl::sep(disjunction_sep));
    static constexpr auto value = lexy::as_list<ConditionalLiteralVec> >>
                                  lexy::callback<HeadLiteral>([](ConditionalLiteralVec elems) -> HeadLiteral {
                                      auto loc = location(elems.front()) + location(elems.back());
                                      if (elems.size() == 1 && elems.front().lits.size() == 1 &&
                                          elems.front().cond.empty()) {
                                          return SimpleHeadLiteral{std::move(elems).front().lits.front()};
                                      }
                                      return Disjunction{std::move(loc), std::move(elems)};
                                  });
};

struct disjunction_element : private junction_element {
    using junction_element::rule;
    using junction_element::value;
};

struct disjunction : private junction<disjunction_element, Disjunction, HeadLiteral> {
    static constexpr auto rule = make_rule(LEXY_KEYWORD("#or", keyword_base));
    using junction::value;
};

struct head_aggregate_element {
    static constexpr char const *name = "head aggregate element";
    static constexpr auto rule = []() {
        auto tuple = dsl::if_(dsl::peek_not(LEXY_LIT(":")) >> dsl::p<term_list>);
        return tuple + LEXY_LIT(":") + dsl::p<literal> + dsl::p<opt_condition>;
    }();
    static constexpr auto value = lexy::callback<HeadAggregate::Element>(
        [](Literal lit, LiteralVec cond) {
            return HeadAggregate::Element{TermVec{}, std::move(lit), std::move(cond)};
        },
        [](TermVec tuple, Literal lit, LiteralVec cond) {
            return HeadAggregate::Element{std::move(tuple), std::move(lit), std::move(cond)};
        });
};

struct head_aggregate_elements {
    static constexpr char const *name = "head aggregate elements";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT("}"));
        auto elems = dsl::list(dsl::p<head_aggregate_element>, dsl::sep(LEXY_LIT(";")));
        return LEXY_LIT("{") + dsl::opt(peek >> elems) + LEXY_LIT("}");
    }();
    static constexpr auto value = lexy::as_list<HeadAggregate::ElementVec>;
};

struct head_aggregate {
    static constexpr char const *name = "head aggregate";
    static constexpr auto rule =
        Detail::location(dsl::p<aggregate_function> >> dsl::p<head_aggregate_elements> + aggregate_right_guard);
    static constexpr auto value = lexy::callback<HeadAggregate>(
        [](Location loc, AggregateFunction fun, HeadAggregate::ElementVec elems) {
            return HeadAggregate(std::move(loc), std::nullopt, fun, std::move(elems), std::nullopt);
        },
        [](Location loc, AggregateFunction fun, HeadAggregate::ElementVec elems, Relation rel, Term rhs) {
            return HeadAggregate(std::move(loc), std::nullopt, fun, std::move(elems),
                                 RGuard::value_type{rel, std::move(rhs)});
        },
        [](Location loc, AggregateFunction fun, HeadAggregate::ElementVec elems, Term rhs) {
            return HeadAggregate(std::move(loc), std::nullopt, fun, std::move(elems),
                                 RGuard::value_type{Relation::less_equal, std::move(rhs)});
        });
};

struct head_literal {
    static constexpr char const *name = "head literal";
    using scan_result = lexy::scan_result<Term>;

    static constexpr auto is_atom = dsl::context_flag<head_literal>;

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto res_term = scanner.template parse<Term>(dsl::p<term>);
        if (res_term.has_value() && check_type(res_term.value(), TermCheckType::atom)) {
            scanner.parse(is_atom.set());
        }
        return res_term;
    }

    STRING_TAG(rel_aggr, "relation or aggregate expected");

    static constexpr auto rule = []() {
        auto with_rel =                                           //
            dsl::p<head_aggregate> | dsl::p<head_set_aggregate> | //
            dsl::else_ >> dsl::p<term> + dsl::opt(dsl::p<right_guards>) + dsl::p<opt_condition> +
                              Detail::post_position + dsl::p<simple_disjunction_element>;

        auto with_term =                                                                                             //
            dsl::p<relation> >> with_rel | dsl::p<head_aggregate> | dsl::p<head_set_aggregate> |                     //
            is_atom.is_set() >> dsl::p<opt_condition> + Detail::post_position + dsl::p<simple_disjunction_element> | //
            dsl::else_ >> dsl::error<expected_rel_aggr>;

        auto peek = dsl::peek(kw_not | dsl::symbol<atom_bool::bool_symbols>(keyword_base));

        return peek >> dsl::p<simple_disjunction> | dsl::p<disjunction> |                       //
               dsl::p<head_theory_atom> | dsl::p<head_aggregate> | dsl::p<head_set_aggregate> | //
               dsl::else_ >> is_atom.create() + dsl::scan + with_term;
    }();

    static constexpr auto value = lexy::callback<HeadLiteral>(
        lexy::construct<HeadLiteral>,
        [](Term term, auto aggr) -> HeadLiteral {
            return Detail::construct_head_aggr(std::move(term), Relation::less_equal, std::move(aggr));
        },
        [](Term term, Relation rel, auto aggr) -> HeadLiteral {
            return Detail::construct_head_aggr(std::move(term), rel, std::move(aggr));
        },
        [](Term lhs, Relation rel, Term rhs, std::optional<GuardVec> opt_guards, LiteralVec cond, Position end,
           ConditionalLiteralVec elems) -> HeadLiteral {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            auto rel_lit = LiteralRelation{location(lhs) + location(guards.back().second), Sign::none, std::move(lhs),
                                           std::move(guards)};
            if (elems.empty() && cond.empty()) {
                return SimpleHeadLiteral{std::move(rel_lit)};
            }
            auto loc_lit = location(rel_lit) + std::move(end);
            elems.insert(elems.begin(),
                         ConditionalLiteral{std::move(loc_lit), LiteralVec{std::move(rel_lit)}, std::move(cond)});
            return Disjunction{location(elems.front()) + location(elems.back()), std::move(elems)};
        },
        [](Term term, LiteralVec cond, Position end, ConditionalLiteralVec elems) -> HeadLiteral {
            auto sym_lit = LiteralSymbolic{location(term), Sign::none, std::move(term)};
            if (elems.empty() && cond.empty()) {
                return SimpleHeadLiteral{std::move(sym_lit)};
            }
            auto loc_lit = location(sym_lit) + std::move(end);
            elems.insert(elems.begin(),
                         ConditionalLiteral{std::move(loc_lit), LiteralVec{std::move(sym_lit)}, std::move(cond)});
            return Disjunction{location(elems.front()) + location(elems.back()), std::move(elems)};
        });
};

} // namespace Gringo::Input::Grammar
