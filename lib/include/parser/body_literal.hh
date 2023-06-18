#pragma once

#include <body_literal.hh>

#include <parser/aggregate.hh>
#include <parser/base.hh>
#include <parser/literal.hh>
#include <parser/theory.hh>

namespace grammar {

using SBodyAggregate = shared_ptr<BodyAggregate>;
using SBodySetAggregate = shared_ptr<BodySetAggregate>;

namespace detail {

inline auto make_body_aggr(SBodyAggregate aggr) -> SBodyAggregate { return std::move(aggr); }

inline auto make_body_aggr(SetAggregate aggr) -> SBodySetAggregate {
    return construct_shared<BodySetAggregate>(std::move(aggr));
}

} // namespace detail

struct body_aggregate_element {
    static constexpr char const *name = "body aggregate element";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(":"));
        return dsl::opt(peek >> dsl::p<term_list>) + dsl::p<opt_condition>;
    }();
    static constexpr auto value =
        lexy::callback<BodyAggregate::Element>([](std::optional<STermVec> tuple, std::optional<SLiteralVec> cond) {
            auto ret = BodyAggregate::Element{STermVec{}, SLiteralVec{}};
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
    static constexpr auto value = lexy::callback<SBodyAggregate>(
        lexy::new_<BodyAggregate, SBodyAggregate>,
        [](AggregateFunction fun, BodyAggregate::ElementVec elems, STerm rhs) {
            return construct_shared<BodyAggregate>(fun, std::move(elems), Relation::less_equal, std::move(rhs));
        });
};

namespace detail {

auto construct_conjunction(SLiteral lit, SLiteralVec cond) {
    VariableSet global;
    lit->variables(global, VariableSelectMode::add);
    for (auto &lit : cond) {
        lit->variables(global, VariableSelectMode::del);
    }
    auto vars = std::vector<std::string>{global.begin(), global.end()};
    std::sort(vars.begin(), vars.end());
    return construct_shared<Conjunction, BodyLiteral>(
        std::move(vars), Conjunction::ElementVec{Conjunction::Element{SLiteralVec{std::move(lit)}, std::move(cond)}});
}

} // namespace detail

struct body_atom : lexy::transparent_production {
    static constexpr char const *name = "body atom";
    using scan_result = lexy::scan_result<STerm>;

    static constexpr auto is_atom = dsl::context_flag<body_atom>;

    STRING_TAG(rel_aggr, "relation or aggregate expected");

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto res_term = scanner.template parse<STerm>(dsl::p<term>);
        if (res_term.has_value() && res_term.value()->check_type(TermCheckType::atom)) {
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
               dsl::else_ >> is_atom.create() + dsl::scan + with_term;
    }();
    static constexpr auto value = lexy::callback<SBodyLiteral>(
        lexy::forward<SBodyLiteral>, lexy::new_<BodySetAggregate, SBodyLiteral>,
        lexy::new_<BodyTheoryAtom, SBodyLiteral>,
        [](STerm term, auto aggr) {
            auto ret = detail::make_body_aggr(std::move(aggr));
            ret->set_left_guard(std::move(term), Relation::less_equal);
            return ret;
        },
        [](STerm term, Relation rel, auto aggr) {
            auto ret = detail::make_body_aggr(std::move(aggr));
            ret->set_left_guard(std::move(term), rel);
            return std::move(ret);
        },
        [](STerm lhs, Relation rel, STerm rhs, std::optional<GuardVec> opt_guards, SLiteralVec cond) {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            auto lit = construct_shared<LiteralRelation, Literal>(std::move(lhs), std::move(guards));
            return detail::construct_conjunction(std::move(lit), std::move(cond));
        },
        [](STerm term, SLiteralVec cond) {
            auto lit = construct_shared<LiteralSymbolic, Literal>(std::move(term));
            return detail::construct_conjunction(std::move(lit), std::move(cond));
        });
};

struct conjunction_element {
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(":"));
        return dsl::opt(peek >> dsl::list(dsl::p<literal>, dsl::sep(LEXY_LIT(",")))) + dsl::p<opt_condition>;
    }();
    static constexpr auto value = lexy::as_list<SLiteralVec> >>
                                  lexy::callback<Conjunction::Element>(
                                      [](lexy::nullopt, SLiteralVec cond) {
                                          return Conjunction::Element{{}, std::move(cond)};
                                      },
                                      lexy::construct<Conjunction::Element>);
};

//  p(X;Y) : q(A;B)
//    p(X): q(A); p(X): q(B)
//    p(Y): q(A); p(Y): q(B)
//  p(X,X..Y) : q(Y)
//    p(X,Z),Z=X..Y: q(Y)
//  #and(X,Y) { p(X;Y) : q(A;B) }.
//    #and(X) { p(X) : q(A;B) }.
//    #and(Y) { p(Y) : q(A;B) }.
//    % unsafe
//  #and() { p(X;Y) : q(X;Y) }.
//    #and(X) { p(X) : q(X;Y) }.
//    #and(Y) { p(Y) : q(X;Y) }.
//    % safe
//  #and(X,Y) { p(X;Y) : q(X;Y) }.
//    #and(X,Y) { p(X) : q(X;Y) }.
//    #and(X,Y) { p(Y) : q(X;Y) }.
//    % unsafe

struct variable_list {
    static constexpr auto rule = []() {
        return dsl::opt(dsl::parenthesized.opt_list(dsl::p<variable>, dsl::sep(dsl::comma)));
    }();
    static constexpr auto value = lexy::as_list<std::vector<std::string>> >>
                                  lexy::callback<std::vector<std::string>>(lexy::forward<std::vector<std::string>>,
                                                                           [](lexy::nullopt) {
                                                                               return std::vector<std::string>{};
                                                                           });
};

struct conjunction {
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#and", keyword_base);
        auto sep = dsl::sep(LEXY_LIT(";"));
        return kw >> dsl::p<variable_list> + dsl::curly_bracketed.opt_list(dsl::p<conjunction_element>, sep);
    }();
    static constexpr auto value = lexy::as_list<Conjunction::ElementVec> >>
                                  lexy::callback<SBodyLiteral>(lexy::new_<Conjunction, SBodyLiteral>,
                                                               [](std::vector<std::string> global, lexy::nullopt) {
                                                                   return construct_shared<Conjunction, BodyLiteral>(
                                                                       std::move(global), Conjunction::ElementVec{});
                                                               });
};

struct body_literal {
    static constexpr char const *name = "body literal";
    static constexpr auto rule = dsl::p<conjunction> | dsl::else_ >> dsl::p<naf_sign> + dsl::p<body_atom>;
    static constexpr auto value =
        lexy::callback<SBodyLiteral>(lexy::forward<SBodyLiteral>, [](Sign sign, SBodyLiteral literal) {
            literal->add_sign(sign);
            return std::move(literal);
        });
};

} // namespace grammar
