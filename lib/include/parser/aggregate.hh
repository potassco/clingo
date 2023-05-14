#pragma once

#include <aggregate.hh>

#include <parser/base.hh>
#include <parser/literal.hh>

namespace grammar {

struct aggregate_function {
    static constexpr auto symbols = lexy::symbol_table<AggregateFunction> //
                                        .map<LEXY_SYMBOL("#count")>(AggregateFunction::count)
                                        .map<LEXY_SYMBOL("#sum")>(AggregateFunction::sum)
                                        .map<LEXY_SYMBOL("#sum+")>(AggregateFunction::sump)
                                        .map<LEXY_SYMBOL("#min")>(AggregateFunction::min)
                                        .map<LEXY_SYMBOL("#max")>(AggregateFunction::max);
    static constexpr auto rule = dsl::symbol<symbols>;
    static constexpr auto value = lexy::forward<AggregateFunction>;
};

struct condition {
    static constexpr auto rule = []() {
        auto colon = dsl::not_followed_by(LEXY_LIT(":"), LEXY_LIT("-"));
        // Note that an empty condition is terminated by one of the symbols
        // listed below in all contexts. In the context of a disjunction,
        // we do not allow it to be terminated with a bar.
        auto peek = dsl::peek_not(LEXY_ASCII_ONE_OF(".;}"));
        return colon >> dsl::opt(peek >> dsl::list(dsl::p<literal>, dsl::sep(LEXY_LIT(","))));
    }();
    static constexpr auto value = lexy::as_list<ULiteralVec>;
};

struct opt_condition {
    static constexpr auto rule = dsl::if_(dsl::p<condition>);
    static constexpr auto value = lexy::construct<ULiteralVec>;
};

struct conditional_literal {
    static constexpr auto rule = dsl::p<literal> + dsl::p<opt_condition>;
    static constexpr auto value = lexy::construct<std::pair<ULiteral, ULiteralVec>>;
};

static constexpr auto aggregate_right_guard = []() {
    // Note an aggregate without a guard is terminated by one of the symbols below.
    auto peek = dsl::peek_not(LEXY_ASCII_ONE_OF(":.,;"));
    return dsl::if_(peek >> dsl::if_(dsl::p<relation>) >> dsl::p<term>);
}();

} // namespace grammar
