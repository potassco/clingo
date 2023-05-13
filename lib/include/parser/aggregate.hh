#pragma once

#include <aggregate.hh>

#include <parser/base.hh>

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

} // namespace grammar
