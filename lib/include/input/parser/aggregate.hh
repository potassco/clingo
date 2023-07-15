#pragma once

#include <input/aggregate.hh>

#include <input/parser/base.hh>
#include <input/parser/literal.hh>

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
    static constexpr auto value = lexy::as_list<SLiteralVec>;
};

struct opt_condition {
    static constexpr char const *name = "condition";
    static constexpr auto rule = dsl::if_(dsl::p<condition>);
    static constexpr auto value = lexy::construct<SLiteralVec>;
};

struct conditional_literal {
    static constexpr char const *name = "conditional literal";
    static constexpr auto rule = dsl::p<literal> + dsl::p<opt_condition>;
    static constexpr auto value = lexy::construct<std::pair<SLiteral, SLiteralVec>>;
};

template <class E> struct junction_element {
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(":"));
        return dsl::opt(peek >> dsl::list(dsl::p<literal>, dsl::sep(LEXY_LIT(",")))) + dsl::p<opt_condition>;
    }();
    static constexpr auto value = lexy::as_list<SLiteralVec> >> lexy::callback<E>(
                                                                    [](lexy::nullopt, SLiteralVec cond) {
                                                                        return E{{}, std::move(cond)};
                                                                    },
                                                                    lexy::construct<E>);
};

template <class E, class J, class L> struct junction {
    static constexpr auto make_rule = [](auto kw) {
        auto sep = dsl::sep(LEXY_LIT(";"));
        return kw >> dsl::curly_bracketed.opt_list(dsl::p<E>, sep);
    };
    static constexpr auto
        value = lexy::as_list<typename J::ElementVec> >>
                lexy::callback<Util::shared_ptr<L>>(lexy::new_<J, Util::shared_ptr<L>>, [](lexy::nullopt) {
                    return Util::construct_shared<J, L>(typename J::ElementVec{});
                });
};

template <class E>
static constexpr auto rule_junction = [](auto kw) {
    auto sep = dsl::sep(LEXY_LIT(";"));
    return kw >> dsl::curly_bracketed.opt_list(dsl::p<E>, sep);
};

static constexpr auto aggregate_right_guard = []() {
    // Note an aggregate without a guard is terminated by one of the symbols below.
    auto peek = dsl::peek_not(LEXY_ASCII_ONE_OF(":.,;"));
    return dsl::if_(peek >> dsl::if_(dsl::p<relation>) >> dsl::p<term>);
}();

struct set_aggregate_elements {
    static constexpr char const *name = "set aggregate elements";
    static constexpr auto rule =
        dsl::opt(dsl::peek_not(LEXY_LIT("}")) >> dsl::list(dsl::p<conditional_literal>, dsl::sep(LEXY_LIT(";"))));
    static constexpr auto value = lexy::as_list<SetAggregate::ElementVec>;
};

struct set_aggregate {
    static constexpr char const *name = "set aggregate";
    static constexpr auto rule = LEXY_LIT("{") >> dsl::p<set_aggregate_elements> >>
                                 LEXY_LIT("}") + aggregate_right_guard;
    static constexpr auto value =
        lexy::callback<SetAggregate>(lexy::construct<SetAggregate>, [](SetAggregate::ElementVec elems, Term rhs) {
            return SetAggregate{std::move(elems), Relation::less_equal, std::move(rhs)};
        });
};

} // namespace Gringo::Input::Grammar
