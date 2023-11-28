#pragma once

#include <util/algorithm.hh>

#include <input/body_literal.hh>

#include "theory.hh"

namespace Gringo::Input::Grammar {

namespace Detail {

inline auto construct_body_aggr(Term term, Relation rel, BodyAggregate aggr) -> BodyAggregate {
    aggr.loc.begin = location(term).begin;
    aggr.lhs = LGuard::value_type{std::move(term), rel};
    return aggr;
}

inline auto construct_body_aggr(Term term, Relation rel, BodySetAggregate aggr) -> BodySetAggregate {
    aggr.loc.begin = location(term).begin;
    aggr.lhs = LGuard::value_type{term, rel};
    return aggr;
}

auto construct_conjunction(Literal lit, LiteralVec cond, Position end) -> BodyLiteral {
    if (cond.empty()) {
        return lit;
    }
    auto loc = location(lit) + std::move(end);
    auto loc_elem = loc;
    return Conjunction{std::move(loc), ConditionalLiteralVec{ConditionalLiteral{
                                           std::move(loc_elem), LiteralVec{std::move(lit)}, std::move(cond)}}};
}

} // namespace Detail

struct body_aggregate_element {
    static constexpr char const *name = "body aggregate element";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(":"));
        return Detail::location(dsl::if_(peek >> dsl::p<term_list>) + dsl::p<opt_condition>);
    }();
    static constexpr auto value = lexy::callback<BodyAggregate::Element>(
        [](Location loc, LiteralVec cond) {
            return BodyAggregate::Element{std::move(loc), TermVec{}, std::move(cond)};
        },
        [](Location loc, TermVec tuple, LiteralVec cond) {
            return BodyAggregate::Element{std::move(loc), std::move(tuple), std::move(cond)};
        });
};

struct body_aggregate_elements {
    static constexpr char const *name = "body aggregate elements";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT("}"));
        auto elems = dsl::list(dsl::p<body_aggregate_element>, dsl::sep(LEXY_LIT(";")));
        return LEXY_LIT("{") + dsl::opt(peek >> elems) + LEXY_LIT("}");
    }();
    static constexpr auto value = lexy::as_list<BodyAggregate::ElementVec>;
};

struct body_aggregate {
    static constexpr char const *name = "body aggregate";
    static constexpr auto rule =
        Detail::location(dsl::p<aggregate_function> >> dsl::p<body_aggregate_elements> + aggregate_right_guard);
    static constexpr auto value = lexy::callback<BodyAggregate>(
        [](Location loc, AggregateFunction fun, BodyAggregate::ElementVec elems) {
            return BodyAggregate{std::move(loc), Sign::none, std::nullopt, fun, std::move(elems), std::nullopt};
        },
        [](Location loc, AggregateFunction fun, BodyAggregate::ElementVec elems, Relation rel, Term rhs) {
            return BodyAggregate{std::move(loc),   Sign::none,
                                 std::nullopt,     fun,
                                 std::move(elems), RGuard::value_type{rel, std::move(rhs)}};
        },
        [](Location loc, AggregateFunction fun, BodyAggregate::ElementVec elems, Term rhs) {
            return BodyAggregate{std::move(loc),   Sign::none,
                                 std::nullopt,     fun,
                                 std::move(elems), RGuard::value_type{Relation::less_equal, std::move(rhs)}};
        });
};

struct body_atom : lexy::transparent_production {
    static constexpr char const *name = "body atom";
    using scan_result = lexy::scan_result<Term>;

    static constexpr auto is_atom = dsl::context_flag<body_atom>;

    STRING_TAG(rel_aggr, "relation or aggregate expected");

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto res_term = scanner.template parse<Term>(dsl::p<term>);
        if (res_term.has_value() && check_type(res_term.value(), TermCheckType::atom)) {
            scanner.parse(is_atom.set());
        }
        return res_term;
    }

    static constexpr auto rule = []() {
        auto with_rel =
            dsl::p<body_aggregate> | dsl::p<body_set_aggregate> |
            dsl::else_ >> dsl::p<term> + dsl::opt(dsl::p<right_guards>) + dsl::p<opt_condition> + Detail::post_position;

        auto with_term =                                                        //
            dsl::p<relation> >> with_rel |                                      //
            dsl::p<body_aggregate> | dsl::p<body_set_aggregate> |               //
            is_atom.is_set() >> dsl::p<opt_condition> + Detail::post_position | //
            dsl::else_ >> dsl::error<expected_rel_aggr>;

        return dsl::p<body_theory_atom> | dsl::p<body_aggregate> | dsl::p<body_set_aggregate> | //
               dsl::p<atom_bool> >> dsl::p<opt_condition> + Detail::post_position |             //
               dsl::else_ >> is_atom.create() + dsl::scan + with_term;
    }();
    static constexpr auto value = lexy::callback<BodyLiteral>(
        lexy::construct<BodyLiteral>,
        [](Term term, auto aggr) {
            return Detail::construct_body_aggr(std::move(term), Relation::less_equal, std::move(aggr));
        },
        [](Term term, Relation rel, auto aggr) {
            return Detail::construct_body_aggr(std::move(term), rel, std::move(aggr));
        },
        [](Term lhs, Relation rel, Term rhs, std::optional<GuardVec> opt_guards, LiteralVec cond, Position end) {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            auto loc = location(lhs) + location(guards.back().second);
            auto lit = LiteralRelation{loc, Sign::none, std::move(lhs), std::move(guards)};
            return Detail::construct_conjunction(std::move(lit), std::move(cond), std::move(end));
        },
        [](Literal lit, LiteralVec cond, Position end) -> BodyLiteral {
            return Detail::construct_conjunction(std::move(lit), std::move(cond), std::move(end));
        },
        [](Term term, LiteralVec cond, Position end) {
            auto loc = location(term);
            auto lit = LiteralSymbolic{loc, Sign::none, std::move(term)};
            return Detail::construct_conjunction(std::move(lit), std::move(cond), std::move(end));
        });
};

struct conjunction : private junction<SimpleBodyLiteral, Conjunction, BodyLiteral> {
    static constexpr auto rule = junction::make_rule(LEXY_KEYWORD("#and", keyword_base));
    using junction::value;
};

struct body_literal {
    static constexpr char const *name = "body literal";
    static constexpr auto rule = dsl::p<conjunction> | dsl::else_ >> dsl::p<naf_sign> + dsl::p<body_atom>;
    static constexpr auto value =
        lexy::callback<BodyLiteral>(lexy::forward<BodyLiteral>, [](auto sign, BodyLiteral literal) {
            add_sign(literal, sign.second);
            return literal;
        });
};

} // namespace Gringo::Input::Grammar
