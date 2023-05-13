#pragma once

#include <head_literal.hh>

#include <parser/aggregate.hh>
#include <parser/literal.hh>

namespace grammar {

struct head_literal {
    using scan_result = lexy::scan_result<UTerm>;

    struct condition {
        static constexpr auto rule = dsl::not_followed_by(LEXY_LIT(":"), LEXY_LIT("-")) >>
                                     dsl::list(dsl::p<literal>, dsl::sep(LEXY_LIT(",")));
        static constexpr auto value = lexy::as_list<ULiteralVec>;
    };

    struct opt_condition {
        static constexpr auto rule = dsl::opt(dsl::not_followed_by(LEXY_LIT(":"), LEXY_LIT("-")) >>
                                              dsl::list(dsl::p<literal>, dsl::sep(LEXY_LIT(","))));
        static constexpr auto value = lexy::as_list<ULiteralVec>;
    };

    struct theory_atom {
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
                                          dsl::inline_<kw_not>;
        static constexpr auto theory_ops = dsl::list(theory_op);
        struct rec_theory_term;
        struct theory_term {
            static constexpr auto rule = dsl::recurse<rec_theory_term>;
            static constexpr auto value = lexy::noop;
        };
        struct theory_root {
            static constexpr auto rule =
                dsl::parenthesized.opt_list(dsl::p<theory_term>, dsl::trailing_sep(dsl::lit_c<','>)) |
                dsl::angle_bracketed.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>)) |
                dsl::curly_bracketed.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>)) |
                dsl::p<identifier> >>
                    dsl::opt(dsl::parenthesized.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>))) |
                dsl::p<constant> | dsl::p<number> | dsl::p<string> | dsl::p<variable> | dsl::p<anonymous_variable>;
            static constexpr auto value = lexy::noop;
        };
        struct rec_theory_term {
            static constexpr auto rule =
                dsl::opt(theory_ops) + dsl::p<theory_root> + dsl::while_(theory_ops >> dsl::p<theory_root>);
            static constexpr auto value = lexy::noop;
        };
        static constexpr auto theory_guard = theory_op >> dsl::p<theory_term>;
        static constexpr auto
            theory_elem = dsl::p<condition> |
                          dsl::else_ >>
                              dsl::list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>)) + dsl::p<opt_condition>;

        static constexpr auto theory_name = dsl::p<identifier> + dsl::opt(dsl::p<term_pool>);
        static constexpr auto rule = LEXY_LIT("&") >>
                                     theory_name + dsl::if_(dsl::curly_bracketed.opt_list(theory_elem,
                                                                                          dsl::sep(dsl::lit_c<';'>)) >>
                                                            dsl::if_(theory_guard));
        static constexpr auto value = lexy::noop >> lexy::callback<UHeadLiteral>([](auto &&...) {
                                          return std::make_unique<HeadTheoryAtom>();
                                      });
    };

    static constexpr auto right_guard = dsl::peek(LEXY_LIT(":") / LEXY_LIT(".")) |
                                        dsl::else_ >> dsl::if_(dsl::p<relation>) + dsl::p<term>;

    struct conditional_literal {
        static constexpr auto rule = dsl::p<literal> + dsl::p<opt_condition>;
        static constexpr auto value = lexy::construct<std::pair<ULiteral, ULiteralVec>>;
    };

    struct aggregate_element {
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
        dsl::else_ >>
            dsl::p<term> + dsl::opt(dsl::p<atom::guards>) + dsl::p<opt_condition> + dsl::p<conditional_literals>;

    static constexpr auto with_term =                                              //
        dsl::p<relation> >> with_rel | dsl::p<aggregate> | dsl::p<set_aggregate> | //
        is_atom.is_set() >> dsl::p<opt_condition> + dsl::p<conditional_literals> | //
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
