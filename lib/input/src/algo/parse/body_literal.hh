#pragma once

#include "theory.hh"

#include <gringo/input/body_literal.hh>

#include <gringo/util/algorithm.hh>

namespace Gringo::Input::Grammar {

namespace Detail {

inline auto construct_body_aggr(Term term, Relation rel, BdLitAggregate const &aggr) -> BdLitAggregate {
    return BdLitAggregate{location(term).begin + aggr.loc(),
                          aggr.sign(),
                          LGuard::value_type{std::move(term), rel},
                          aggr.fun(),
                          aggr.elems(),
                          aggr.rhs()};
}

inline auto construct_body_aggr(Term term, Relation rel, BdLitSetAggregate const &aggr) -> BdLitSetAggregate {
    return BdLitSetAggregate{location(term).begin + aggr.loc(), aggr.sign(), LGuard::value_type{std::move(term), rel},
                             aggr.elems(), aggr.rhs()};
}

auto construct_conjunction(Lit lit, std::optional<std::vector<Lit>> cond, Position end) -> BdLit {
    if (!cond) {
        return lit;
    }
    return BdLitConjunction{CondLit{location(lit) + end, std::move(lit), std::move(cond).value()}};
}

} // namespace Detail

struct body_aggregate_element {
    static constexpr char const *name = "body aggregate element";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(":"));
        return Detail::location(dsl::if_(peek >> dsl::p<term_list>) + dsl::p<if_condition>);
    }();
    static constexpr auto value = lexy::callback<BdLitAggregateElement>(
        [](Location loc, std::vector<Lit> cond) {
            return BdLitAggregateElement{loc, TermArray{}, std::move(cond)};
        },
        [](Location loc, TermArray tuple, std::vector<Lit> cond) {
            return BdLitAggregateElement{loc, std::move(tuple), std::move(cond)};
        });
};

struct body_aggregate_elements {
    static constexpr char const *name = "body aggregate elements";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT("}"));
        auto elems = dsl::list(dsl::p<body_aggregate_element>, dsl::sep(LEXY_LIT(";")));
        return LEXY_LIT("{") + dsl::opt(peek >> elems) + LEXY_LIT("}");
    }();
    static constexpr auto value = lexy::as_list<std::vector<BdLitAggregateElement>>;
};

struct body_aggregate {
    static constexpr char const *name = "body aggregate";
    static constexpr auto rule =
        Detail::location(dsl::p<aggregate_function> >> dsl::p<body_aggregate_elements> + aggregate_right_guard);
    static constexpr auto value = lexy::callback<BdLitAggregate>(
        [](Location loc, AggregateFunction fun, BdLitAggregateElementArray elems) {
            return BdLitAggregate{loc, Sign::none, std::nullopt, fun, std::move(elems), std::nullopt};
        },
        [](Location loc, AggregateFunction fun, BdLitAggregateElementArray elems, Relation rel, Term rhs) {
            return BdLitAggregate{loc, Sign::none,       std::nullopt,
                                  fun, std::move(elems), RGuard::value_type{rel, std::move(rhs)}};
        },
        [](Location loc, AggregateFunction fun, BdLitAggregateElementArray elems, Term rhs) {
            return BdLitAggregate{loc, Sign::none,       std::nullopt,
                                  fun, std::move(elems), RGuard::value_type{Relation::less_equal, std::move(rhs)}};
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
    static constexpr auto value = lexy::callback<BdLit>(
        lexy::construct<BdLit>,
        [](Term term, auto aggr) {
            return Detail::construct_body_aggr(std::move(term), Relation::less_equal, std::move(aggr));
        },
        [](Term term, Relation rel, auto aggr) {
            return Detail::construct_body_aggr(std::move(term), rel, std::move(aggr));
        },
        [](Term lhs, Relation rel, Term rhs, std::optional<GuardArray> opt_guards, std::optional<std::vector<Lit>> cond,
           Position end) {
            std::vector<Guard> guards;
            if (opt_guards.has_value()) {
                guards.assign(opt_guards->begin(), opt_guards->end());
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            auto loc = location(lhs) + location(guards.back().second);
            auto lit = LitComparison{loc, Sign::none, std::move(lhs), std::move(guards)};
            return Detail::construct_conjunction(std::move(lit), std::move(cond), end);
        },
        [](Lit lit, std::optional<std::vector<Lit>> cond, Position end) -> BdLit {
            return Detail::construct_conjunction(std::move(lit), std::move(cond), end);
        },
        [](Term term, std::optional<std::vector<Lit>> cond, Position end) {
            auto loc = location(term);
            auto lit = LitSymbolic{loc, Sign::none, std::move(term)};
            return Detail::construct_conjunction(std::move(lit), std::move(cond), end);
        });
};

struct body_literal {
    static constexpr char const *name = "body literal";
    static constexpr auto rule = dsl::p<naf_sign> + dsl::p<body_atom>;
    static constexpr auto value = lexy::callback<BdLit>(lexy::forward<BdLit>, [](auto sign, BdLit lit) {
        auto res = add_sign(lit, sign.second);
        return std::move(res).value_or(std::move(lit));
    });
};

} // namespace Gringo::Input::Grammar
