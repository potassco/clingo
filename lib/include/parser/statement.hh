#pragma once

#include <statement.hh>

#include <parser/base.hh>
#include <parser/body_literal.hh>
#include <parser/head_literal.hh>

namespace grammar {

struct statement {
    struct body {
        static constexpr auto sep = LEXY_LIT(",") / LEXY_LIT(";");
        static constexpr auto rule =
            dsl::opt(dsl::peek_not(LEXY_LIT(".")) >> dsl::list(dsl::p<body_literal>, dsl::sep(sep)));
        static constexpr auto value = lexy::as_list<UBodyLiteralVec>;
    };
    static constexpr auto if_body = LEXY_LIT(":-") >> dsl::p<body> + LEXY_LIT(".");
    static constexpr auto rule = if_body | dsl::else_ >> dsl::p<head_literal> + (LEXY_LIT(".") | if_body);
    // static constexpr auto rule = dsl::p<head_literal> + LEXY_LIT(".");
    static constexpr auto value = lexy::callback<UStatement>(
        lexy::new_<Rule, UStatement>,
        [](UHeadLiteral head) { return std::make_unique<Rule>(std::move(head), UBodyLiteralVec{}); },
        [](UBodyLiteralVec body) {
            return std::make_unique<Rule>(std::make_unique<Disjunction>(Disjunction::ElementVec{}), std::move(body));
        });
};

} // namespace grammar
