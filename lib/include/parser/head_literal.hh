#pragma once

#include <head_literal.hh>

#include <parser/aggregate.hh>
#include <parser/literal.hh>
#include <parser/theory.hh>

namespace grammar {

namespace detail {

inline auto make_head_aggr(UHeadAggregate aggr) -> UHeadAggregate { return std::move(aggr); }

inline auto make_head_aggr(SetAggregate aggr) -> UHeadSetAggregate {
    return std::make_unique<HeadSetAggregate>(std::move(aggr));
}

} // namespace detail

static constexpr auto disjunction_sep = LEXY_ASCII_ONE_OF(",;|");

struct disjunction_element {
    static constexpr auto rule = dsl::opt(dsl::list(disjunction_sep >> dsl::p<conditional_literal>));
    static constexpr auto value = lexy::as_list<Disjunction::ElementVec>;
};

struct disjunction {
    static constexpr auto rule = dsl::list(dsl::p<conditional_literal>, dsl::sep(disjunction_sep));
    static constexpr auto value = lexy::as_list<Disjunction::ElementVec> >> lexy::new_<Disjunction, UHeadLiteral>;
};

struct head_aggregate_element {
    // TODO: gringo also accepts "tuple:literal:<empty>". This is possible
    // here by using [;}] as lookahead.
    static constexpr auto rule = dsl::opt(dsl::peek_not(LEXY_LIT(":")) >> dsl::p<term_tuple>) + LEXY_LIT(":") +
                                 dsl::p<literal> + dsl::p<opt_condition>;
    static constexpr auto value = lexy::callback<HeadAggregate::Element>(
        [](std::optional<UTermVec> tuple, ULiteral lit, std::optional<ULiteralVec> cond) {
            auto ret = HeadAggregate::Element{UTermVec{}, std::move(lit), ULiteralVec{}};
            if (tuple) {
                std::get<0>(ret) = std::move(tuple).value();
            }
            if (cond) {
                std::get<2>(ret) = std::move(cond).value();
            }
            return ret;
        });
};

struct head_aggregate_elements {
    static constexpr auto rule =
        dsl::opt(dsl::peek_not(LEXY_LIT("}")) >> dsl::list(dsl::p<head_aggregate_element>, dsl::sep(LEXY_LIT(";"))));
    static constexpr auto value = lexy::as_list<HeadAggregate::ElementVec>;
};

struct head_aggregate {
    static constexpr auto rule = dsl::p<aggregate_function> >> LEXY_LIT("{") + dsl::p<head_aggregate_elements> +
                                                                   LEXY_LIT("}") + aggregate_right_guard;
    static constexpr auto value = lexy::callback<UHeadAggregate>(
        lexy::new_<HeadAggregate, UHeadAggregate>,
        [](AggregateFunction fun, HeadAggregate::ElementVec elems, UTerm rhs) {
            return std::make_unique<HeadAggregate>(fun, std::move(elems), Relation::less_equal, std::move(rhs));
        });
};

struct head_literal {
    using scan_result = lexy::scan_result<UTerm>;

    static constexpr auto is_atom = dsl::context_flag<head_literal>;

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner, auto &&...args) -> scan_result {
        auto res_term = scanner.template parse<UTerm>(dsl::p<term>);
        if (res_term.has_value() && res_term.value()->is_atom()) {
            scanner.parse(is_atom.set());
        }
        return res_term;
    }

    struct rel_aggr_expected {
        static constexpr auto name = "relation or aggregate expected";
    };

    static constexpr auto rule = []() {
        auto with_rel =                                      //
            dsl::p<head_aggregate> | dsl::p<set_aggregate> | //
            dsl::else_ >>
                dsl::p<term> + dsl::opt(dsl::p<right_guards>) + dsl::p<opt_condition> + dsl::p<disjunction_element>;

        auto with_term =                                                                    //
            dsl::p<relation> >> with_rel | dsl::p<head_aggregate> | dsl::p<set_aggregate> | //
            is_atom.is_set() >> dsl::p<opt_condition> + dsl::p<disjunction_element> |       //
            dsl::else_ >> dsl::error<rel_aggr_expected>;

        return dsl::peek(kw_not) >> dsl::p<disjunction> |                             //
               dsl::p<theory_atom> | dsl::p<head_aggregate> | dsl::p<set_aggregate> | //
               dsl::else_ >> is_atom.create() + dsl::scan + with_term;
    }();

    static constexpr auto value = lexy::callback<UHeadLiteral>(
        lexy::forward<UHeadLiteral>, lexy::new_<HeadTheoryAtom, UHeadLiteral>,
        lexy::new_<HeadSetAggregate, UHeadLiteral>,
        [](UTerm term, auto aggr) {
            auto ret = detail::make_head_aggr(std::move(aggr));
            ret->set_left_guard(std::move(term), Relation::less_equal);
            return ret;
        },
        [](UTerm term, Relation rel, auto aggr) {
            auto ret = detail::make_head_aggr(std::move(aggr));
            ret->set_left_guard(std::move(term), rel);
            return std::move(ret);
        },
        [](UTerm term, Relation rel, UTerm rhs, std::optional<GuardVec> opt_guards, ULiteralVec cond,
           Disjunction::ElementVec elems) {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            elems.insert(elems.begin(),
                         Disjunction::Element{std::make_unique<LiteralRelation>(std::move(term), std::move(guards)),
                                              std::move(cond)});
            return std::make_unique<Disjunction>(std::move(elems));
        },
        [](UTerm term, ULiteralVec cond, Disjunction::ElementVec elems) {
            elems.insert(elems.begin(),
                         Disjunction::Element{std::make_unique<LiteralSymbolic>(std::move(term)), std::move(cond)});
            return std::make_unique<Disjunction>(std::move(elems));
        });
};

} // namespace grammar
