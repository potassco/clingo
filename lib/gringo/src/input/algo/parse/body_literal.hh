#pragma once

#include <gringo/util/algorithm.hh>

#include <gringo/input/body_literal.hh>

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

auto construct_conjunction(Literal lit, std::optional<LiteralVec> cond, Position end) -> BodyLiteral {
    if (!cond) {
        return lit;
    }
    return ConditionalLiteral{location(lit) + std::move(end), std::move(lit), std::move(cond).value()};
}

} // namespace Detail

struct body_aggregate_element {
    static constexpr char const *name = "body aggregate element";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(":"));
        return Detail::location(dsl::if_(peek >> dsl::p<term_list>) + dsl::p<if_condition>);
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
    static constexpr auto value = lexy::as_list<std::vector<BodyAggregate::Element>>;
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
        [](Term lhs, Relation rel, Term rhs, std::optional<GuardVec> opt_guards, std::optional<LiteralVec> cond,
           Position end) {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            auto loc = location(lhs) + location(guards.back().second);
            auto lit = LiteralRelation{loc, Sign::none, std::move(lhs), std::move(guards)};
            return Detail::construct_conjunction(std::move(lit), std::move(cond), std::move(end));
        },
        [](Literal lit, std::optional<LiteralVec> cond, Position end) -> BodyLiteral {
            return Detail::construct_conjunction(std::move(lit), std::move(cond), std::move(end));
        },
        [](Term term, std::optional<LiteralVec> cond, Position end) {
            auto loc = location(term);
            auto lit = LiteralSymbolic{loc, Sign::none, std::move(term)};
            return Detail::construct_conjunction(std::move(lit), std::move(cond), std::move(end));
        });
};

struct body_literal {
    static constexpr char const *name = "body literal";
    static constexpr auto rule = dsl::p<naf_sign> + dsl::p<body_atom>;
    static constexpr auto value =
        lexy::callback<BodyLiteral>(lexy::forward<BodyLiteral>, [](auto sign, BodyLiteral lit) {
            auto res = add_sign(lit, sign.second);
            return std::move(res).value_or(std::move(lit));
        });
};

} // namespace Gringo::Input::Grammar
