#pragma once

#include <body_literal.hh>

#include <parser/aggregate.hh>
#include <parser/base.hh>
#include <parser/literal.hh>

namespace grammar {

struct body_atom : lexy::transparent_production {
    using scan_result = lexy::scan_result<UTerm>;

    struct theory_atom {
        // TODO: proper construction
        static constexpr auto rule = LEXY_LIT("&") >> dsl::p<identifier> + LEXY_LIT("{") + LEXY_LIT("}");
        static constexpr auto value =
            lexy::callback<UBodyLiteral>([](auto &&...) { return std::make_unique<BodyTheoryAtom>(); });
    };

    static constexpr auto right_guard = dsl::peek(LEXY_LIT(":") / LEXY_LIT(".")) |
                                        dsl::else_ >> dsl::if_(dsl::p<relation>) + dsl::p<term>;

    struct condition {
        static constexpr auto rule = dsl::opt(LEXY_LIT(":") >> dsl::list(dsl::p<literal>, dsl::sep(LEXY_LIT(","))));
        static constexpr auto value = lexy::as_list<ULiteralVec>;
    };

    struct conditional_literal {
        static constexpr auto rule = dsl::p<literal> + dsl::p<condition>;
        static constexpr auto value = lexy::new_<ConditionalLiteral, UBodyLiteral>;
    };

    struct aggregate_element {
        // TODO: this allows either an empty tuple or condition but not both.
        // See not at head_literal.
        static constexpr auto rule = dsl::opt(dsl::peek_not(LEXY_LIT(":")) >> dsl::p<term_tuple>) + dsl::p<condition>;
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

    struct aggregate_elements {
        static constexpr auto rule =
            dsl::opt(dsl::peek_not(LEXY_LIT("}")) >> dsl::list(dsl::p<aggregate_element>, dsl::sep(LEXY_LIT(";"))));
        static constexpr auto value = lexy::as_list<BodyAggregate::ElementVec>;
    };

    struct aggregate {
        static constexpr auto rule = dsl::p<aggregate_function> >>
                                     LEXY_LIT("{") + dsl::p<aggregate_elements> + LEXY_LIT("}") + right_guard;
        static constexpr auto value = lexy::callback<UBodyAggregate>(
            lexy::new_<BodyAggregate, UBodyAggregate>,
            [](AggregateFunction fun, BodyAggregate::ElementVec elems, UTerm rhs) {
                return std::make_unique<BodyAggregate>(fun, std::move(elems), Relation::less_equal, std::move(rhs));
            });
    };

    struct set_aggregate_element {
        static constexpr auto rule = dsl::p<literal> + dsl::p<condition>;
        static constexpr auto value = lexy::construct<BodySetAggregate::Element>;
    };

    struct set_aggregate_elements {
        static constexpr auto rule =
            dsl::opt(dsl::peek_not(LEXY_LIT("}")) >> dsl::list(dsl::p<set_aggregate_element>, dsl::sep(LEXY_LIT(";"))));
        static constexpr auto value = lexy::as_list<BodySetAggregate::ElementVec>;
    };

    struct set_aggregate {
        static constexpr auto rule = LEXY_LIT("{") >> dsl::p<set_aggregate_elements> >> LEXY_LIT("}") + right_guard;
        static constexpr auto value = lexy::callback<UBodySetAggregate>(
            lexy::new_<BodySetAggregate, UBodySetAggregate>, [](BodySetAggregate::ElementVec elems, UTerm rhs) {
                return std::make_unique<BodySetAggregate>(std::move(elems), Relation::less_equal, std::move(rhs));
            });
    };

    static constexpr auto is_atom = dsl::context_flag<body_atom>;

    struct rel_aggr_expected {
        static constexpr auto name = "relation or aggregate expected";
    };

    static constexpr auto with_rel = dsl::p<aggregate> | dsl::p<set_aggregate> |
                                     dsl::else_ >> dsl::p<term> + dsl::opt(dsl::p<right_guards>) + dsl::p<condition>;

    static constexpr auto with_term =               //
        dsl::p<relation> >> with_rel |              //
        dsl::p<aggregate> | dsl::p<set_aggregate> | //
        is_atom.is_set() >> dsl::p<condition> |     //
        dsl::else_ >> dsl::error<rel_aggr_expected>;

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto res_term = scanner.template parse<UTerm>(dsl::p<term>);
        if (res_term.has_value() && res_term.value()->is_atom()) {
            scanner.parse(is_atom.set());
        }
        return res_term;
    }

    static constexpr auto rule = dsl::p<theory_atom> | dsl::p<aggregate> | dsl::p<set_aggregate> | //
                                 dsl::else_ >> is_atom.create() + dsl::scan + with_term;
    static constexpr auto value = lexy::callback<UBodyLiteral>(
        lexy::forward<UBodyLiteral>,
        [](UTerm term, auto aggr) {
            aggr->set_left_guard(std::move(term), Relation::less_equal);
            return std::move(aggr);
        },
        [](UTerm term, Relation rel, auto aggr) {
            aggr->set_left_guard(std::move(term), rel);
            return std::move(aggr);
        },
        [](UTerm term, Relation rel, UTerm rhs, std::optional<GuardVec> opt_guards, ULiteralVec cond) {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            auto lit = std::make_unique<LiteralRelation>(std::move(term), std::move(guards));
            return std::make_unique<ConditionalLiteral>(std::move(lit), std::move(cond));
        },
        [](UTerm term, ULiteralVec cond) {
            auto lit = std::make_unique<LiteralSymbolic>(std::move(term));
            return std::make_unique<ConditionalLiteral>(std::move(lit), std::move(cond));
        });
};

struct body_literal {
    struct sign {
        static auto constexpr rule = dsl::opt(kw_not) + dsl::opt(kw_not);
        static auto constexpr value = lexy::callback<Sign>([](lexy::nullopt, lexy::nullopt) { return Sign::none; }, //
                                                           [](lexy::nullopt) { return Sign::once; },                //
                                                           []() { return Sign::twice; });
    };

    static constexpr auto rule = dsl::p<sign> + dsl::p<body_atom>;
    static constexpr auto value = lexy::callback<UBodyLiteral>([](Sign sign, UBodyLiteral literal) {
        literal->add_sign(sign);
        return std::move(literal);
    });
};

} // namespace grammar
