#pragma once

#include <body_literal.hh>

#include <parser/aggregate.hh>
#include <parser/base.hh>
#include <parser/literal.hh>
#include <parser/theory.hh>

namespace grammar {

namespace detail {

inline auto make_body_aggr(UBodyAggregate aggr) -> UBodyAggregate { return std::move(aggr); }

inline auto make_body_aggr(SetAggregate aggr) -> UBodySetAggregate {
    return std::make_unique<BodySetAggregate>(std::move(aggr));
}

} // namespace detail

struct body_aggregate_element {
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(":"));
        return dsl::opt(peek >> dsl::p<term_list>) + dsl::p<opt_condition>;
    }();
    static constexpr auto value =
        lexy::callback<BodyAggregate::Element>([](std::optional<UTermVec> tuple, std::optional<ULiteralVec> cond) {
            auto ret = BodyAggregate::Element{UTermVec{}, ULiteralVec{}};
            if (tuple) {
                std::get<0>(ret) = std::move(tuple).value();
            }
            if (cond) {
                std::get<1>(ret) = std::move(cond).value();
            }
            return ret;
        });
};

struct body_aggregate_elements {
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT("}"));
        auto elems = dsl::list(dsl::p<body_aggregate_element>, dsl::sep(LEXY_LIT(";")));
        return LEXY_LIT("{") + dsl::opt(peek >> elems) + LEXY_LIT("}");
    }();
    static constexpr auto value = lexy::as_list<BodyAggregate::ElementVec>;
};

struct body_aggregate {
    static constexpr auto rule = dsl::p<aggregate_function> >> dsl::p<body_aggregate_elements> + aggregate_right_guard;
    static constexpr auto value = lexy::callback<UBodyAggregate>(
        lexy::new_<BodyAggregate, UBodyAggregate>,
        [](AggregateFunction fun, BodyAggregate::ElementVec elems, UTerm rhs) {
            return std::make_unique<BodyAggregate>(fun, std::move(elems), Relation::less_equal, std::move(rhs));
        });
};

struct body_atom : lexy::transparent_production {
    using scan_result = lexy::scan_result<UTerm>;

    static constexpr auto is_atom = dsl::context_flag<body_atom>;

    struct rel_aggr_expected {
        static constexpr auto name = "relation or aggregate expected";
    };

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto res_term = scanner.template parse<UTerm>(dsl::p<term>);
        if (res_term.has_value() && res_term.value()->is_atom()) {
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
            dsl::else_ >> dsl::error<rel_aggr_expected>;

        return dsl::p<theory_atom> | dsl::p<body_aggregate> | dsl::p<set_aggregate> | //
               dsl::else_ >> is_atom.create() + dsl::scan + with_term;
    }();
    static constexpr auto value = lexy::callback<UBodyLiteral>(
        lexy::forward<UBodyLiteral>, lexy::new_<BodySetAggregate, UBodyLiteral>,
        lexy::new_<BodyTheoryAtom, UBodyLiteral>,
        [](UTerm term, auto aggr) {
            auto ret = detail::make_body_aggr(std::move(aggr));
            ret->set_left_guard(std::move(term), Relation::less_equal);
            return ret;
        },
        [](UTerm term, Relation rel, auto aggr) {
            auto ret = detail::make_body_aggr(std::move(aggr));
            ret->set_left_guard(std::move(term), rel);
            return std::move(ret);
        },
        [](UTerm lhs, Relation rel, UTerm rhs, std::optional<GuardVec> opt_guards, ULiteralVec cond) {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            auto lit = std::make_unique<LiteralRelation>(std::move(lhs), std::move(guards));
            return std::make_unique<ConditionalLiteral>(std::move(lit), std::move(cond));
        },
        [](UTerm term, ULiteralVec cond) {
            auto lit = std::make_unique<LiteralSymbolic>(std::move(term));
            return std::make_unique<ConditionalLiteral>(std::move(lit), std::move(cond));
        });
};

struct body_literal {
    static constexpr auto rule = dsl::p<naf_sign> + dsl::p<body_atom>;
    static constexpr auto value = lexy::callback<UBodyLiteral>([](Sign sign, UBodyLiteral literal) {
        literal->add_sign(sign);
        return std::move(literal);
    });
};

} // namespace grammar
