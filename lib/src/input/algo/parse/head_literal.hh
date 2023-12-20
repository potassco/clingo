#pragma once

#include <input/head_literal.hh>

#include "theory.hh"

namespace Gringo::Input::Grammar {

namespace Detail {

inline auto construct_disj_elem(auto loc, auto lit, auto cond) -> Disjunction::Element {
    if (!cond) {
        return lit;
    }
    return ConditionalLiteral{loc, lit, std::move(cond).value()};
}

inline auto construct_head_aggr(Term term, Relation rel, HeadAggregate aggr) -> HeadAggregate {
    aggr.loc.begin = location(term).begin;
    aggr.lhs = LGuard::value_type{std::move(term), rel};
    return aggr;
}

inline auto construct_head_aggr(Term term, Relation rel, HeadSetAggregate aggr) -> HeadSetAggregate {
    aggr.loc = location(term) + aggr.loc;
    aggr.lhs = LGuard::value_type{std::move(term), rel};
    return aggr;
}

} // namespace Detail

static constexpr auto disjunction_sep = LEXY_ASCII_ONE_OF(",;|");

struct disjunction_element {
    static constexpr char const *name = "conditional literal";
    static constexpr auto rule = Detail::location(dsl::p<literal> + dsl::p<opt_condition>);
    static constexpr auto value = lexy::callback<Disjunction::Element>([](auto &&loc, auto &&lit, auto &&cond) {
        return Detail::construct_disj_elem(GRINGO_FWD(loc), GRINGO_FWD(lit), GRINGO_FWD(cond));
    });
};

struct disjunction_elements {
    static constexpr char const *name = "disjunction element";
    static constexpr auto rule = dsl::opt(dsl::list(disjunction_sep >> dsl::p<disjunction_element>));
    static constexpr auto value = lexy::as_list<Disjunction::ElementVec>;
};

struct disjunction {
    static constexpr char const *name = "disjunction";
    static constexpr auto rule = dsl::list(dsl::p<disjunction_element>, dsl::sep(disjunction_sep));
    static constexpr auto value = lexy::as_list<Disjunction::ElementVec> >>
                                  lexy::callback<HeadLiteral>([](Disjunction::ElementVec elems) -> HeadLiteral {
                                      auto loc = location(elems.front()) + location(elems.back());
                                      if (elems.size() == 1) {
                                          if (auto const *lit = std::get_if<Literal>(&elems.front())) {
                                              return SimpleHeadLiteral{*lit};
                                          }
                                      }
                                      return Disjunction{std::move(loc), std::move(elems)};
                                  });
};

struct head_aggregate_element {
    static constexpr char const *name = "head aggregate element";
    static constexpr auto rule = []() {
        auto tuple = dsl::if_(dsl::peek_not(LEXY_LIT(":")) >> dsl::p<term_list>);
        return Detail::location(tuple + LEXY_LIT(":") + dsl::p<literal> + dsl::p<if_condition>);
    }();
    static constexpr auto value = lexy::callback<HeadAggregate::Element>(
        [](Location loc, Literal lit, LiteralVec cond) {
            return HeadAggregate::Element{std::move(loc), TermVec{}, std::move(lit), std::move(cond)};
        },
        [](Location loc, TermVec tuple, Literal lit, LiteralVec cond) {
            return HeadAggregate::Element{std::move(loc), std::move(tuple), std::move(lit), std::move(cond)};
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
                              Detail::post_position + dsl::p<disjunction_elements>;

        auto with_term =                                                                                       //
            dsl::p<relation> >> with_rel | dsl::p<head_aggregate> | dsl::p<head_set_aggregate> |               //
            is_atom.is_set() >> dsl::p<opt_condition> + Detail::post_position + dsl::p<disjunction_elements> | //
            dsl::else_ >> dsl::error<expected_rel_aggr>;

        auto peek = dsl::peek(kw_not | dsl::symbol<atom_bool::bool_symbols>(keyword_base));

        return peek >> dsl::p<disjunction> |                                                    //
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
        [](Term lhs, Relation rel, Term rhs, std::optional<GuardVec> opt_guards, std::optional<LiteralVec> cond,
           Position end, Disjunction::ElementVec elems) -> HeadLiteral {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            auto rel_lit = LiteralRelation{location(lhs) + location(guards.back().second), Sign::none, std::move(lhs),
                                           std::move(guards)};
            if (elems.empty() && !cond) {
                return SimpleHeadLiteral{std::move(rel_lit)};
            }
            auto loc_lit = location(rel_lit) + std::move(end);
            elems.insert(elems.begin(),
                         Detail::construct_disj_elem(std::move(loc_lit), std::move(rel_lit), std::move(cond)));
            return Disjunction{location(elems.front()) + location(elems.back()), std::move(elems)};
        },
        [](Term term, std::optional<LiteralVec> cond, Position end, Disjunction::ElementVec elems) -> HeadLiteral {
            auto sym_lit = LiteralSymbolic{location(term), Sign::none, std::move(term)};
            if (elems.empty() && !cond) {
                return SimpleHeadLiteral{std::move(sym_lit)};
            }
            auto loc_lit = location(sym_lit) + std::move(end);
            elems.insert(elems.begin(),
                         Detail::construct_disj_elem(std::move(loc_lit), std::move(sym_lit), std::move(cond)));
            return Disjunction{location(elems.front()) + location(elems.back()), std::move(elems)};
        });
};

} // namespace Gringo::Input::Grammar
