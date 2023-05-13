#pragma once

#include <head_literal.hh>

#include <parser/aggregate.hh>
#include <parser/literal.hh>

namespace grammar {

struct head_literal {
    using scan_result = lexy::scan_result<UTerm>;

    struct theory_atom {
        // TODO: proper construction
        static constexpr auto op_class =
            dsl::identifier(dsl::lit_c<'/'> / dsl::lit_c<'<'> / dsl::lit_c<'='> / dsl::lit_c<'>'> / dsl::lit_c<'+'> /
                            dsl::lit_c<'\\'> / dsl::lit_c<'-'> / dsl::lit_c<'*'> / dsl::lit_c<'/'> / dsl::lit_c<'?'> /
                            dsl::lit_c<'&'> / dsl::lit_c<'@'> / dsl::lit_c<'|'> / dsl::lit_c<':'> / dsl::lit_c<';'> /
                            dsl::lit_c<'~'> / dsl::lit_c<'^'> / dsl::lit_c<'.'> / dsl::lit_c<'!'>);
        static constexpr auto kw_semicolon = LEXY_KEYWORD(";", op_class);
        static constexpr auto kw_colon = LEXY_KEYWORD(":", op_class);
        static constexpr auto kw_dot = LEXY_KEYWORD(".", op_class);
        static constexpr auto theory_op = op_class //
                                              .reserve(kw_semicolon)
                                              .reserve(kw_colon)
                                              .reserve(kw_dot) |
                                          dsl::p<kw_not>;
        static constexpr auto theory_ops = dsl::list(theory_op);
        static constexpr auto rec_theory_term = dsl::recurse<struct theory_term>;
        static constexpr auto theory_tuple = dsl::list(rec_theory_term, dsl::sep(dsl::lit_c<','>));
        static constexpr auto theory_name = dsl::p<identifier> + dsl::opt(dsl::p<pool>);
        // TBC
        static constexpr auto rule = LEXY_LIT("&") >> theory_name + LEXY_LIT("{") + LEXY_LIT("}");
        static constexpr auto value =
            lexy::callback<UHeadLiteral>([](auto &&...) { return std::make_unique<HeadTheoryAtom>(); });
    };

    static constexpr auto right_guard = dsl::peek(LEXY_LIT(":") / LEXY_LIT(".")) |
                                        dsl::else_ >> dsl::if_(dsl::p<relation>) + dsl::p<term>;

    struct condition {
        static constexpr auto rule = dsl::opt(dsl::not_followed_by(LEXY_LIT(":"), LEXY_LIT("-")) >>
                                              dsl::list(dsl::p<literal>, dsl::sep(LEXY_LIT(","))));
        static constexpr auto value = lexy::as_list<ULiteralVec>;
    };

    struct conditional_literal {
        static constexpr auto rule = dsl::p<literal> + dsl::p<condition>;
        static constexpr auto value = lexy::construct<std::pair<ULiteral, ULiteralVec>>;
    };

    struct aggregate_element {
        // Note: gringo also accepts
        //   HeadElem ::= Tuple? ':' Literal (':' Condition?)?
        // It is probably not worth the effort to support an empty condition
        // after a colon (but possible with a lookahead of [;}]).
        static constexpr auto rule = dsl::opt(dsl::peek_not(LEXY_LIT(":")) >> dsl::p<tuple>) + LEXY_LIT(":") +
                                     dsl::p<literal> + dsl::p<condition>;
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

    struct aggregate_elements {
        static constexpr auto rule =
            dsl::opt(dsl::peek_not(LEXY_LIT("}")) >> dsl::list(dsl::p<aggregate_element>, dsl::sep(LEXY_LIT(";"))));
        static constexpr auto value = lexy::as_list<HeadAggregate::ElementVec>;
    };

    struct aggregate {
        static constexpr auto rule = dsl::p<aggregate_function> >>
                                     LEXY_LIT("{") + dsl::p<aggregate_elements> + LEXY_LIT("}") + right_guard;
        static constexpr auto value = lexy::callback<UHeadAggregate>(
            lexy::new_<HeadAggregate, UHeadAggregate>,
            [](AggregateFunction fun, HeadAggregate::ElementVec elems, UTerm rhs) {
                return std::make_unique<HeadAggregate>(fun, std::move(elems), Relation::less_equal, std::move(rhs));
            });
    };

    struct set_aggregate_elements {
        static constexpr auto rule =
            dsl::opt(dsl::peek_not(LEXY_LIT("}")) >> dsl::list(dsl::p<conditional_literal>, dsl::sep(LEXY_LIT(";"))));
        static constexpr auto value = lexy::as_list<HeadSetAggregate::ElementVec>;
    };

    struct set_aggregate {
        static constexpr auto rule = LEXY_LIT("{") >> dsl::p<set_aggregate_elements> >> LEXY_LIT("}") + right_guard;
        static constexpr auto value = lexy::callback<UHeadSetAggregate>(
            lexy::new_<HeadSetAggregate, UHeadSetAggregate>, [](HeadSetAggregate::ElementVec elems, UTerm rhs) {
                return std::make_unique<HeadSetAggregate>(std::move(elems), Relation::less_equal, std::move(rhs));
            });
    };

    static constexpr auto sep = LEXY_LIT(",") / LEXY_LIT(";") / LEXY_LIT("|");

    struct conditional_literals {
        static constexpr auto rule = dsl::opt(dsl::list(sep >> dsl::p<conditional_literal>));
        static constexpr auto value = lexy::as_list<Disjunction::ElementVec>;
    };

    struct disjunction {
        static constexpr auto rule = dsl::list(dsl::p<conditional_literal>, dsl::sep(sep));
        static constexpr auto value = lexy::as_list<Disjunction::ElementVec> >> lexy::new_<Disjunction, UHeadLiteral>;
    };

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

    static constexpr auto with_rel =                //
        dsl::p<aggregate> | dsl::p<set_aggregate> | //
        dsl::else_ >> dsl::p<term> + dsl::opt(dsl::p<atom::guards>) + dsl::p<condition> + dsl::p<conditional_literals>;

    static constexpr auto with_term =                                              //
        dsl::p<relation> >> with_rel | dsl::p<aggregate> | dsl::p<set_aggregate> | //
        is_atom.is_set() >> dsl::p<condition> + dsl::p<conditional_literals> |     //
        dsl::else_ >> dsl::error<rel_aggr_expected>;

    static constexpr auto rule =                                          //
        dsl::peek(dsl::p<kw_not>) >> dsl::p<disjunction> |                //
        dsl::p<theory_atom> | dsl::p<aggregate> | dsl::p<set_aggregate> | //
        dsl::else_ >> is_atom.create() + dsl::scan + with_term;

    static constexpr auto value = lexy::callback<UHeadLiteral>(
        lexy::forward<UHeadLiteral>,
        [](UTerm term, auto aggr) {
            aggr->set_left_guard(std::move(term), Relation::less_equal);
            return std::move(aggr);
        },
        [](UTerm term, Relation rel, auto aggr) {
            aggr->set_left_guard(std::move(term), rel);
            return std::move(aggr);
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
