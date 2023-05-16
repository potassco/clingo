#pragma once

#include <statement.hh>

#include <parser/base.hh>
#include <parser/body_literal.hh>
#include <parser/head_literal.hh>

namespace grammar {

// TODO: implement theory operator definitions
/*
OpDef ::= Operator ':' Number ',' 'unary'
        | Operator ':' Number ',' 'binary' ',' 'left'
        | Operator ':' Number ',' 'binary' ',' 'right'
TermDef ::= Identifier '{' (OpDef (';' OpDef)*)? '}'
TermDefs ::= TermDef (';' TermDef)*

GuardRels ::= Operator (',' Operator)*
GuardDef ::= '{' GuardRels? '}' ',' Identifier
AtomType ::= 'head' | 'body' | 'any' | 'directive'
AtomDef ::= Identifier '/' Number ':' Identifier
            (',' GuardDef)? ',' AtomType
AtomDefs ::= AtomDef (';' AtomDef)*

Defs ::= TermDefs ';' AtomDefs
       | TermDefs | AtomDefs
Theory ::= '#theory' '(' Identifier ')' '{' Defs? '}' '.'
 */

struct body {
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT("."));
        auto sep = dsl::sep(LEXY_LIT(",") / LEXY_LIT(";"));
        return dsl::opt(peek >> dsl::list(dsl::p<body_literal>, sep));
    }();
    static constexpr auto value = lexy::as_list<UBodyLiteralVec>;
};

struct rule_ {
    static constexpr auto rule = []() {
        auto if_body = LEXY_LIT(":-") >> dsl::p<body> + LEXY_LIT(".");
        return if_body | dsl::else_ >> dsl::p<head_literal> + (LEXY_LIT(".") | if_body);
    }();
    static constexpr auto value = lexy::callback<UStatement>(
        lexy::new_<Rule, UStatement>,
        [](UHeadLiteral head) { return std::make_unique<Rule>(std::move(head), UBodyLiteralVec{}); },
        [](UBodyLiteralVec body) {
            return std::make_unique<Rule>(std::make_unique<Disjunction>(Disjunction::ElementVec{}), std::move(body));
        });
};

struct statement {
    static constexpr auto rule = dsl::p<rule_>;
    static constexpr auto value = lexy::forward<UStatement>;
};

} // namespace grammar
