#pragma once

#include <input/aggregate.hh>

#include "literal.hh"

namespace Gringo::Input::Grammar {

namespace Detail {
template <bool HasSign> static auto construct_set_aggregate(Location loc, SetAggregateElementVec elems, RGuard rhs) {
    if constexpr (HasSign) {
        return SetAggregate<HasSign>{std::move(loc), Sign::none, std::nullopt, std::move(elems), std::move(rhs)};
    } else {
        return SetAggregate<HasSign>{std::move(loc), std::nullopt, std::move(elems), std::move(rhs)};
    }
}
} // namespace Detail

struct aggregate_function {
    static constexpr char const *name = "aggregate function";
    static constexpr auto symbols = lexy::symbol_table<AggregateFunction> //
                                        .map<LEXY_SYMBOL("#count")>(AggregateFunction::count)
                                        .map<LEXY_SYMBOL("#sum")>(AggregateFunction::sum)
                                        .map<LEXY_SYMBOL("#sum+")>(AggregateFunction::sump)
                                        .map<LEXY_SYMBOL("#min")>(AggregateFunction::min)
                                        .map<LEXY_SYMBOL("#max")>(AggregateFunction::max);
    static constexpr auto rule = dsl::symbol<symbols>(keyword_base);
    static constexpr auto value = lexy::forward<AggregateFunction>;
};

struct condition {
    static constexpr char const *name = "condition";
    static constexpr auto rule = []() {
        auto colon = dsl::not_followed_by(LEXY_LIT(":"), LEXY_LIT("-"));
        // Note that an empty condition is terminated by one of the symbols
        // listed below in all contexts. In the context of a disjunction,
        // we do not allow it to be terminated with a bar.
        auto peek = dsl::peek_not(LEXY_ASCII_ONE_OF(".;}"));
        return colon >> dsl::opt(peek >> dsl::list(dsl::p<literal>, dsl::sep(LEXY_LIT(","))));
    }();
    static constexpr auto value = lexy::as_list<LiteralVec>;
};

struct opt_condition {
    static constexpr char const *name = "condition";
    static constexpr auto rule = dsl::if_(dsl::p<condition>);
    static constexpr auto value = lexy::construct<LiteralVec>;
};

struct conditional_literal {
    static constexpr char const *name = "conditional literal";
    static constexpr auto rule = dsl::p<literal> + dsl::p<opt_condition> + Detail::post_position;
    static constexpr auto value = lexy::callback<ConditionalLiteral>([](Literal lit, LiteralVec cond, Position end) {
        auto loc = location(lit) + std::move(end);
        return ConditionalLiteral{std::move(loc), LiteralVec{std::move(lit)}, std::move(cond)};
    });
};

struct set_aggregate_element {
    static constexpr char const *name = "conditional literal";
    static constexpr auto rule = dsl::p<literal> + dsl::p<opt_condition>;
    static constexpr auto value = lexy::callback<SetAggregateElement>([](Literal lit, LiteralVec cond) {
        return SetAggregateElement{std::move(lit), std::move(cond)};
    });
};

struct junction_element {
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(":"));
        return Detail::location(dsl::if_(peek >> dsl::list(dsl::p<literal>, dsl::sep(LEXY_LIT(",")))) +
                                dsl::p<opt_condition>);
    }();
    static constexpr auto value = lexy::as_list<LiteralVec> >>
                                  lexy::callback<ConditionalLiteral>(
                                      [](Location loc, LiteralVec cond) {
                                          return ConditionalLiteral{std::move(loc), {}, std::move(cond)};
                                      },
                                      [](Location loc, LiteralVec lits, LiteralVec cond) {
                                          return ConditionalLiteral{std::move(loc), std::move(lits), std::move(cond)};
                                      });
};

template <class E, class J, class L> struct junction {
    static constexpr auto make_rule = [](auto kw) {
        auto sep = dsl::sep(LEXY_LIT(";"));
        auto elems = dsl::curly_bracketed.opt_list(dsl::p<E>, sep);
        return Detail::location(kw >> elems);
    };
    static constexpr auto value = lexy::as_list<ConditionalLiteralVec> >>
                                  lexy::callback<L>(
                                      [](Location loc, ConditionalLiteralVec elems) {
                                          return J{std::move(loc), ConditionalLiteralVec{std::move(elems)}};
                                      },
                                      [](Location loc, lexy::nullopt) {
                                          return J{std::move(loc), ConditionalLiteralVec{}};
                                      });
};

template <class E>
static constexpr auto rule_junction = [](auto kw) {
    auto sep = dsl::sep(LEXY_LIT(";"));
    return kw >> dsl::curly_bracketed.opt_list(dsl::p<E>, sep);
};

static constexpr auto aggregate_right_guard = []() {
    // Note an aggregate without a guard is terminated by one of the symbols below.
    auto peek = dsl::peek_not(LEXY_ASCII_ONE_OF(":.,;") | dsl::eof);
    return dsl::if_(peek >> dsl::if_(dsl::p<relation>) >> dsl::p<term>);
}();

struct set_aggregate_elements {
    static constexpr char const *name = "set aggregate elements";
    static constexpr auto rule =
        dsl::opt(dsl::peek_not(LEXY_LIT("}")) >> dsl::list(dsl::p<set_aggregate_element>, dsl::sep(LEXY_LIT(";"))));
    static constexpr auto value = lexy::as_list<SetAggregateElementVec>;
};

template <bool HasSign> struct set_aggregate {
    static constexpr char const *name = "set aggregate";
    static constexpr auto rule =
        Detail::location(LEXY_LIT("{") >> dsl::p<set_aggregate_elements> >> LEXY_LIT("}") + aggregate_right_guard);
    static constexpr auto value = lexy::callback<SetAggregate<HasSign>>(
        [](Location loc, SetAggregateElementVec elems) {
            return Detail::construct_set_aggregate<HasSign>(std::move(loc), std::move(elems), std::nullopt);
        },
        [](Location loc, SetAggregateElementVec elems, Term rhs) {
            return Detail::construct_set_aggregate<HasSign>(std::move(loc), std::move(elems),
                                                            RGuard::value_type{Relation::less_equal, std::move(rhs)});
        },
        [](Location loc, SetAggregateElementVec elems, Relation rel, Term rhs) {
            return Detail::construct_set_aggregate<HasSign>(std::move(loc), std::move(elems),
                                                            RGuard::value_type{rel, std::move(rhs)});
        });
};

using body_set_aggregate = set_aggregate<true>;
using head_set_aggregate = set_aggregate<false>;

} // namespace Gringo::Input::Grammar
