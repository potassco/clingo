#pragma once

#include "literal.hh"

#include <gringo/input/aggregate.hh>

#include <utility>

namespace Gringo::Input::Grammar {

namespace Detail {
template <bool HasSign> static auto construct_set_aggregate(Location loc, SetAggregateElementArray elems, RGuard rhs) {
    if constexpr (HasSign) {
        return SetAggregate<HasSign>{loc, Sign::none, std::nullopt, std::move(elems), std::move(rhs)};
    } else {
        return SetAggregate<HasSign>{loc, std::nullopt, std::move(elems), std::move(rhs)};
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
    static constexpr auto value = lexy::as_list<std::vector<Lit>>;
};

struct if_condition {
    static constexpr char const *name = "condition";
    static constexpr auto rule = dsl::if_(dsl::p<condition>);
    static constexpr auto value = lexy::construct<std::vector<Lit>>;
};

struct opt_condition {
    static constexpr char const *name = "condition";
    static constexpr auto rule = dsl::if_(dsl::p<condition>);
    static constexpr auto value = lexy::construct<std::optional<std::vector<Lit>>>;
};

struct set_aggregate_element {
    static constexpr char const *name = "conditional literal";
    static constexpr auto rule = Detail::location(dsl::p<literal> + dsl::p<if_condition>);
    static constexpr auto value = lexy::callback<SetAggregateElement>([](Location loc, Lit lit, std::vector<Lit> cond) {
        return SetAggregateElement{std::move(loc), std::move(lit), std::move(cond)};
    });
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
    static constexpr auto value = lexy::as_list<std::vector<SetAggregateElement>>;
};

template <bool HasSign> struct set_aggregate {
    static constexpr char const *name = "set aggregate";
    static constexpr auto rule =
        Detail::location(LEXY_LIT("{") >> dsl::p<set_aggregate_elements> >> LEXY_LIT("}") + aggregate_right_guard);
    static constexpr auto value = lexy::callback<SetAggregate<HasSign>>(
        [](Location loc, SetAggregateElementArray elems) {
            return Detail::construct_set_aggregate<HasSign>(loc, std::move(elems), std::nullopt);
        },
        [](Location loc, SetAggregateElementArray elems, Term rhs) {
            return Detail::construct_set_aggregate<HasSign>(loc, std::move(elems),
                                                            RGuard::value_type{Relation::less_equal, std::move(rhs)});
        },
        [](Location loc, SetAggregateElementArray elems, Relation rel, Term rhs) {
            return Detail::construct_set_aggregate<HasSign>(loc, std::move(elems),
                                                            RGuard::value_type{rel, std::move(rhs)});
        });
};

using body_set_aggregate = set_aggregate<true>;
using head_set_aggregate = set_aggregate<false>;

} // namespace Gringo::Input::Grammar
