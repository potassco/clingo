#pragma once

#include <util/algorithm.hh>

#include <input/body_literal.hh>

#include "theory.hh"

namespace Gringo::Input::Grammar {

namespace Detail {

inline auto construct_body_aggr(Term term, Relation rel, BodyAggregate aggr) -> BodyAggregate {
    aggr.lhs = LGuard::value_type{term, rel};
    return aggr;
}

inline auto construct_body_aggr(Term term, Relation rel, SetAggregate aggr) -> BodySetAggregate {
    aggr.lhs = LGuard::value_type{term, rel};
    return BodySetAggregate{std::move(aggr)};
}

auto construct_conjunction(Literal lit, OptCondition cond) {
    return Conjunction{ConditionalLiteralVec{
        ConditionalLiteral{location(lit) + std::move(cond.second), LiteralVec{std::move(lit)}, std::move(cond.first)}}};
}

} // namespace Detail

struct body_aggregate_element {
    static constexpr char const *name = "body aggregate element";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(":"));
        return dsl::if_(peek >> dsl::p<term_list>) + dsl::p<opt_condition>;
    }();
    static constexpr auto value = lexy::callback<BodyAggregate::Element>(
        [](OptCondition cond) {
            return BodyAggregate::Element{TermVec{}, std::move(cond.first)};
        },
        [](TermVec tuple, OptCondition cond) {
            return BodyAggregate::Element{std::move(tuple), std::move(cond.first)};
        });
};

struct body_aggregate_elements {
    static constexpr char const *name = "body aggregate elements";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT("}"));
        auto elems = dsl::list(dsl::p<body_aggregate_element>, dsl::sep(LEXY_LIT(";")));
        return LEXY_LIT("{") + dsl::opt(peek >> elems) + LEXY_LIT("}");
    }();
    static constexpr auto value = lexy::as_list<BodyAggregate::ElementVec>;
};

struct body_aggregate {
    static constexpr char const *name = "body aggregate";
    static constexpr auto rule = dsl::p<aggregate_function> >> dsl::p<body_aggregate_elements> + aggregate_right_guard;
    static constexpr auto value = lexy::callback<BodyAggregate>(
        lexy::construct<BodyAggregate>, [](AggregateFunction fun, BodyAggregate::ElementVec elems, Term rhs) {
            return BodyAggregate{fun, std::move(elems), Relation::less_equal, std::move(rhs)};
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
        auto with_rel = dsl::p<body_aggregate> | dsl::p<set_aggregate> |
                        dsl::else_ >> dsl::p<term> + dsl::opt(dsl::p<right_guards>) + dsl::p<opt_condition>;

        auto with_term =                                     //
            dsl::p<relation> >> with_rel |                   //
            dsl::p<body_aggregate> | dsl::p<set_aggregate> | //
            is_atom.is_set() >> dsl::p<opt_condition> |      //
            dsl::else_ >> dsl::error<expected_rel_aggr>;

        return dsl::p<theory_atom> | dsl::p<body_aggregate> | dsl::p<set_aggregate> | //
               dsl::p<atom_bool> >> dsl::p<opt_condition> |                           //
               dsl::else_ >> is_atom.create() + dsl::scan + with_term;
    }();
    static constexpr auto value = lexy::callback<BodyLiteral>(
        lexy::forward<BodyLiteral>, lexy::construct<BodySetAggregate>, lexy::construct<BodyTheoryAtom>,
        [](Term term, auto aggr) {
            return Detail::construct_body_aggr(std::move(term), Relation::less_equal, std::move(aggr));
        },
        [](Term term, Relation rel, auto aggr) {
            return Detail::construct_body_aggr(std::move(term), rel, std::move(aggr));
        },
        [](Term lhs, Relation rel, Term rhs, std::optional<GuardVec> opt_guards, OptCondition cond) {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            auto loc = location(lhs) + location(guards.back().second);
            auto lit = LiteralRelation{loc, std::move(lhs), std::move(guards)};
            return Detail::construct_conjunction(std::move(lit), std::move(cond));
        },
        [](Literal lit, OptCondition cond) { return Detail::construct_conjunction(std::move(lit), std::move(cond)); },
        [](Term term, OptCondition cond) {
            auto loc = location(term);
            auto lit = LiteralSymbolic{loc, std::move(term)};
            return Detail::construct_conjunction(std::move(lit), std::move(cond));
        });
};

struct conjunction_element : private junction_element {
    using junction_element::rule;
    using junction_element::value;
};

struct conjunction : private junction<conjunction_element, Conjunction, BodyLiteral> {
    static constexpr auto rule = junction::make_rule(LEXY_KEYWORD("#and", keyword_base));
    using junction::value;
};

struct body_literal {
    static constexpr char const *name = "body literal";
    static constexpr auto rule = dsl::p<conjunction> | dsl::else_ >> dsl::p<naf_sign> + dsl::p<body_atom>;
    static constexpr auto value =
        lexy::callback<BodyLiteral>(lexy::forward<BodyLiteral>, [](auto sign, BodyLiteral literal) {
            add_sign(literal, sign.second);
            return literal;
        });
};

} // namespace Gringo::Input::Grammar
