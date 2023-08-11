#pragma once

#include <input/aggregate.hh>

#include "literal.hh"

namespace Gringo::Input::Grammar {

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

using OptCondition = std::pair<LiteralVec, std::optional<Position>>;

struct opt_condition {
    static constexpr char const *name = "condition";
    static constexpr auto rule = dsl::if_(Detail::position(dsl::p<condition>));
    static constexpr auto value = lexy::callback<OptCondition>(
        [](Position begin, auto cond) {
            auto loc = cond.empty() ? ++begin.column, std::move(begin) : location(cond.back()).end;
            return std::make_pair(std::move(cond), std::move(loc));
        },
        []() { return std::make_pair(LiteralVec{}, std::nullopt); });
};

struct conditional_literal {
    static constexpr char const *name = "conditional literal";
    static constexpr auto rule = dsl::p<literal> + dsl::p<opt_condition>;
    static constexpr auto value = lexy::callback<ConditionalLiteral>([](Literal lit, OptCondition cond) {
        return ConditionalLiteral{location(lit) + std::move(cond.second), LiteralVec{std::move(lit)},
                                  std::move(cond.first)};
    });
};

struct set_aggregate_element {
    static constexpr char const *name = "conditional literal";
    static constexpr auto rule = dsl::p<literal> + dsl::p<opt_condition>;
    static constexpr auto value = lexy::callback<SetAggregate::Element>([](Literal lit, OptCondition cond) {
        return SetAggregate::Element{std::move(lit), std::move(cond.first)};
    });
};

struct junction_element {
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(":"));
        return dsl::opt(peek >> dsl::list(dsl::p<literal>, dsl::sep(LEXY_LIT(",")))) + dsl::p<opt_condition>;
    }();
    static constexpr auto value =
        lexy::as_list<LiteralVec> >>
        lexy::callback<ConditionalLiteral>(
            [](lexy::nullopt, OptCondition cond) {
                assert(cond.second.has_value());
                auto pos = std::move(cond.second).value();
                auto loc = cond.first.empty() ? Location{Position{pos.file, pos.line, pos.column - 1}, pos}
                                              : location(cond.first.front()) + pos;
                return ConditionalLiteral{loc, {}, std::move(cond.first)};
            },
            [](LiteralVec lits, OptCondition cond) {
                assert(!lits.empty());
                return ConditionalLiteral{location(lits.front()) + cond.second, std::move(lits), std::move(cond.first)};
            });
};

template <class E, class J, class L> struct junction {
    static constexpr auto make_rule = [](auto kw) {
        auto sep = dsl::sep(LEXY_LIT(";"));
        auto braces = dsl::brackets(LEXY_LIT("{"), Detail::post_position(LEXY_LIT("}")));
        return Detail::position(kw) >> braces.opt_list(dsl::p<E>, sep);
    };
    static constexpr auto
        value = lexy::as_list<ConditionalLiteralVec> >>
                lexy::callback<L>(
                    [](Position begin, ConditionalLiteralVec elems, Position end) {
                        return J{Location(std::move(begin), std::move(end)), ConditionalLiteralVec{std::move(elems)}};
                    },
                    [](Position begin, lexy::nullopt, Position end) {
                        return J{Location(std::move(begin), std::move(end)), ConditionalLiteralVec{}};
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
    static constexpr auto value = lexy::as_list<SetAggregate::ElementVec>;
};

struct set_aggregate {
    static constexpr char const *name = "set aggregate";
    static constexpr auto rule = Detail::position(LEXY_LIT("{")) >> dsl::p<set_aggregate_elements> >>
                                 Detail::post_position(LEXY_LIT("}")) + aggregate_right_guard;
    static constexpr auto value = lexy::callback<SetAggregate>(
        [](Position begin, SetAggregate::ElementVec elems, Position end) {
            return SetAggregate{Location(std::move(begin), std::move(end)), std::move(elems)};
        },
        [](Position begin, SetAggregate::ElementVec elems, Position end, Term rhs) {
            static_cast<void>(end);
            auto loc = std::move(begin) + location(rhs);
            return SetAggregate{std::move(loc), std::move(elems), Relation::less_equal, std::move(rhs)};
        },
        [](Position begin, SetAggregate::ElementVec elems, Position end, Relation rel, Term rhs) {
            static_cast<void>(end);
            auto loc = std::move(begin) + location(rhs);
            return SetAggregate{std::move(loc), std::move(elems), rel, std::move(rhs)};
        });
};

} // namespace Gringo::Input::Grammar
