#pragma once

#include <head_literal.hh>

#include <parser/aggregate.hh>
#include <parser/literal.hh>
#include <parser/theory.hh>

namespace grammar {

using SHeadAggregate = shared_ptr<HeadAggregate>;
using SHeadSetAggregate = shared_ptr<HeadSetAggregate>;

namespace detail {

inline auto make_head_aggr(SHeadAggregate aggr) -> SHeadAggregate { return std::move(aggr); }

inline auto make_head_aggr(SetAggregate aggr) -> SHeadSetAggregate {
    return construct_shared<HeadSetAggregate>(std::move(aggr));
}

} // namespace detail

static constexpr auto disjunction_sep = LEXY_ASCII_ONE_OF(",;|");

struct disjunction_element {
    static constexpr char const *name = "disjunction element";
    static constexpr auto rule = dsl::opt(dsl::list(disjunction_sep >> dsl::p<conditional_literal>));
    static constexpr auto value = lexy::as_list<Disjunction::ElementVec>;
};

struct disjunction {
    static constexpr char const *name = "disjunction";
    static constexpr auto rule = dsl::list(dsl::p<conditional_literal>, dsl::sep(disjunction_sep));
    static constexpr auto value = lexy::as_list<Disjunction::ElementVec> >> lexy::new_<Disjunction, SHeadLiteral>;
};

struct head_aggregate_element {
    static constexpr char const *name = "head aggregate element";
    static constexpr auto rule = []() {
        auto tuple = dsl::opt(dsl::peek_not(LEXY_LIT(":")) >> dsl::p<term_list>);
        return tuple + LEXY_LIT(":") + dsl::p<literal> + dsl::p<opt_condition>;
    }();
    static constexpr auto value = lexy::callback<HeadAggregate::Element>(
        [](std::optional<STermVec> tuple, SLiteral lit, std::optional<SLiteralVec> cond) {
            auto ret = HeadAggregate::Element{STermVec{}, std::move(lit), SLiteralVec{}};
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
    static constexpr char const *name = "head aggregate elements";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT("}"));
        auto elems = dsl::list(dsl::p<head_aggregate_element>, dsl::sep(LEXY_LIT(";")));
        return LEXY_LIT("{") + dsl::opt(peek >> elems) + LEXY_LIT("}");
    }();
    static constexpr auto value = lexy::as_list<HeadAggregate::ElementVec>;
};

struct head_aggregate {
    static constexpr char const *name = "head aggregate";
    static constexpr auto rule = dsl::p<aggregate_function> >> dsl::p<head_aggregate_elements> + aggregate_right_guard;
    static constexpr auto value = lexy::callback<SHeadAggregate>(
        lexy::new_<HeadAggregate, SHeadAggregate>,
        [](AggregateFunction fun, HeadAggregate::ElementVec elems, STerm rhs) {
            return construct_shared<HeadAggregate>(fun, std::move(elems), Relation::less_equal, std::move(rhs));
        });
};

struct head_literal {
    static constexpr char const *name = "head literal";
    using scan_result = lexy::scan_result<STerm>;

    static constexpr auto is_atom = dsl::context_flag<head_literal>;

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner, auto &&...args) -> scan_result {
        auto res_term = scanner.template parse<STerm>(dsl::p<term>);
        if (res_term.has_value() && res_term.value()->check_type(TermCheckType::atom)) {
            scanner.parse(is_atom.set());
        }
        return res_term;
    }

    STRING_TAG(rel_aggr, "relation or aggregate expected");

    static constexpr auto rule = []() {
        auto with_rel =                                      //
            dsl::p<head_aggregate> | dsl::p<set_aggregate> | //
            dsl::else_ >>
                dsl::p<term> + dsl::opt(dsl::p<right_guards>) + dsl::p<opt_condition> + dsl::p<disjunction_element>;

        auto with_term =                                                                    //
            dsl::p<relation> >> with_rel | dsl::p<head_aggregate> | dsl::p<set_aggregate> | //
            is_atom.is_set() >> dsl::p<opt_condition> + dsl::p<disjunction_element> |       //
            dsl::else_ >> dsl::error<expected_rel_aggr>;

        return dsl::peek(kw_not) >> dsl::p<disjunction> |                             //
               dsl::p<theory_atom> | dsl::p<head_aggregate> | dsl::p<set_aggregate> | //
               dsl::else_ >> is_atom.create() + dsl::scan + with_term;
    }();

    static constexpr auto value = lexy::callback<SHeadLiteral>(
        lexy::forward<SHeadLiteral>, lexy::new_<HeadTheoryAtom, SHeadLiteral>,
        lexy::new_<HeadSetAggregate, SHeadLiteral>,
        [](STerm term, auto aggr) {
            auto ret = detail::make_head_aggr(std::move(aggr));
            ret->set_left_guard(std::move(term), Relation::less_equal);
            return ret;
        },
        [](STerm term, Relation rel, auto aggr) {
            auto ret = detail::make_head_aggr(std::move(aggr));
            ret->set_left_guard(std::move(term), rel);
            return std::move(ret);
        },
        [](STerm lhs, Relation rel, STerm rhs, std::optional<GuardVec> opt_guards, SLiteralVec cond,
           Disjunction::ElementVec elems) {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            elems.insert(elems.begin(), Disjunction::Element{construct_shared<LiteralRelation, Literal>(
                                                                 std::move(lhs), std::move(guards)),
                                                             std::move(cond)});
            return construct_shared<Disjunction, HeadLiteral>(std::move(elems));
        },
        [](STerm term, SLiteralVec cond, Disjunction::ElementVec elems) {
            elems.insert(
                elems.begin(),
                Disjunction::Element{construct_shared<LiteralSymbolic, Literal>(std::move(term)), std::move(cond)});
            return construct_shared<Disjunction, HeadLiteral>(std::move(elems));
        });
};

} // namespace grammar
