#pragma once

#include <input/head_literal.hh>

#include "theory.hh"

namespace Gringo::Input::Grammar {

namespace Detail {

inline auto construct_head_aggr(Term term, Relation rel, HeadAggregate aggr) -> HeadAggregate {
    aggr.lhs = LGuard::value_type{std::move(term), rel};
    return aggr;
}

inline auto construct_head_aggr(Term term, Relation rel, SetAggregate aggr) -> HeadSetAggregate {
    aggr.lhs = LGuard::value_type{std::move(term), rel};
    return HeadSetAggregate{std::move(aggr)};
}

struct construct_disjunction_element {
    using return_type = ConditionalLiteral;
    auto operator()(std::pair<Literal, LiteralVec> elem) const -> return_type {
        return {LiteralVec{std::move(elem.first)}, std::move(elem.second)};
    }
};

} // namespace Detail

static constexpr auto disjunction_sep = LEXY_ASCII_ONE_OF(",;|");

struct simple_disjunction_element {
    static constexpr char const *name = "disjunction element";
    static constexpr auto rule = dsl::opt(dsl::list(disjunction_sep >> dsl::p<conditional_literal>));
    static constexpr auto value = lexy::as_list<ConditionalLiteralVec>;
};

struct simple_disjunction {
    static constexpr char const *name = "disjunction";
    static constexpr auto rule = dsl::list(dsl::p<conditional_literal>, dsl::sep(disjunction_sep));
    static constexpr auto value = lexy::as_list<ConditionalLiteralVec> >> Detail::construct_v<Disjunction, HeadLiteral>;
};

struct disjunction_element : private junction_element<ConditionalLiteral> {
    using junction_element::rule;
    using junction_element::value;
};

struct disjunction : private junction<disjunction_element, Disjunction, HeadLiteral> {
    static constexpr auto rule = make_rule(LEXY_KEYWORD("#or", keyword_base));
    using junction::value;
};

struct head_aggregate_element {
    static constexpr char const *name = "head aggregate element";
    static constexpr auto rule = []() {
        auto tuple = dsl::opt(dsl::peek_not(LEXY_LIT(":")) >> dsl::p<term_list>);
        return tuple + LEXY_LIT(":") + dsl::p<literal> + dsl::p<opt_condition>;
    }();
    static constexpr auto value = lexy::callback<HeadAggregate::Element>(
        [](std::optional<TermVec> tuple, Literal lit, std::optional<LiteralVec> cond) {
            auto ret = HeadAggregate::Element{TermVec{}, std::move(lit), LiteralVec{}};
            if (tuple) {
                ret.tuple = std::move(tuple).value();
            }
            if (cond) {
                ret.cond = std::move(cond).value();
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
    static constexpr auto value = lexy::callback<HeadAggregate>(
        lexy::construct<HeadAggregate>, [](AggregateFunction fun, HeadAggregate::ElementVec elems, Term rhs) {
            return HeadAggregate(fun, std::move(elems), Relation::less_equal, std::move(rhs));
        });
};

struct head_literal {
    static constexpr char const *name = "head literal";
    using scan_result = lexy::scan_result<Term>;

    static constexpr auto is_atom = dsl::context_flag<head_literal>;

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto res_term = scanner.template parse<Term>(dsl::p<term>);
        if (res_term.has_value() && check_type(res_term.value(), TermCheckType::atom)) {
            scanner.parse(is_atom.set());
        }
        return res_term;
    }

    STRING_TAG(rel_aggr, "relation or aggregate expected");

    static constexpr auto rule = []() {
        auto with_rel =                                      //
            dsl::p<head_aggregate> | dsl::p<set_aggregate> | //
            dsl::else_ >> dsl::p<term> + dsl::opt(dsl::p<right_guards>) + dsl::p<opt_condition> +
                              dsl::p<simple_disjunction_element>;

        auto with_term =                                                                     //
            dsl::p<relation> >> with_rel | dsl::p<head_aggregate> | dsl::p<set_aggregate> |  //
            is_atom.is_set() >> dsl::p<opt_condition> + dsl::p<simple_disjunction_element> | //
            dsl::else_ >> dsl::error<expected_rel_aggr>;

        auto peek = dsl::peek(kw_not | dsl::symbol<atom_bool::bool_symbols>(keyword_base));

        return peek >> dsl::p<simple_disjunction> | dsl::p<disjunction> |             //
               dsl::p<theory_atom> | dsl::p<head_aggregate> | dsl::p<set_aggregate> | //
               dsl::else_ >> is_atom.create() + dsl::scan + with_term;
    }();

    static constexpr auto value = lexy::callback<HeadLiteral>(
        lexy::forward<HeadLiteral>, lexy::construct<HeadSetAggregate>, lexy::construct<HeadTheoryAtom>,
        [](Term term, auto aggr) -> HeadLiteral {
            return Detail::construct_head_aggr(std::move(term), Relation::less_equal, std::move(aggr));
        },
        [](Term term, Relation rel, auto aggr) -> HeadLiteral {
            return Detail::construct_head_aggr(std::move(term), rel, std::move(aggr));
        },
        [](Term lhs, Relation rel, Term rhs, std::optional<GuardVec> opt_guards, LiteralVec cond,
           ConditionalLiteralVec elems) -> HeadLiteral {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            elems.insert(
                elems.begin(),
                ConditionalLiteral{LiteralVec{LiteralRelation{std::move(lhs), std::move(guards)}}, std::move(cond)});
            return Disjunction{std::move(elems)};
        },
        [](Term term, LiteralVec cond, ConditionalLiteralVec elems) {
            elems.insert(elems.begin(),
                         ConditionalLiteral{LiteralVec{LiteralSymbolic{std::move(term)}}, std::move(cond)});
            return Disjunction{std::move(elems)};
        });
};

} // namespace Gringo::Input::Grammar
