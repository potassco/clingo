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

Defs ::= TermDefs ';' AtomDefs | TermDefs | AtomDefs
Theory ::= '#theory' '(' Identifier ')' '{' Defs? '}' '.'
 */

enum class TheoryOpArity {
    unary,
    binary
};

enum class TheoryOpAssociativity {
    left,
    right
};

enum class TheoryAtomType {
    head,
    body,
    any,
    directive
};


static constexpr auto simple_number = dsl::digits<>.sep(dsl::digit_sep_tick).no_leading_zero();

static constexpr auto simple_keyword = dsl::identifier(dsl::ascii::lower);

static constexpr auto sym_unary = lexy::symbol_table<TheoryOpArity>.map<LEXY_SYMBOL("unary")>(TheoryOpArity::unary);
static constexpr auto sym_binary = lexy::symbol_table<TheoryOpArity>.map<LEXY_SYMBOL("binary")>(TheoryOpArity::binary);

static constexpr auto unary = dsl::symbol<sym_unary>(simple_keyword);
static constexpr auto binary = dsl::symbol<sym_unary>(simple_keyword);

static constexpr auto sym_associativity = lexy::symbol_table<TheoryOpAssociativity>
                                         .map<LEXY_SYMBOL("unary")>(TheoryOpAssociativity::left)
                                         .map<LEXY_SYMBOL("binary")>(TheoryOpAssociativity::right);

static constexpr auto associativity = dsl::symbol<sym_associativity>(simple_keyword);

static constexpr auto sym_atom_type = lexy::symbol_table<TheoryAtomType>
    .map<LEXY_SYMBOL("head")>(TheoryAtomType::head)
    .map<LEXY_SYMBOL("body")>(TheoryAtomType::body)
    .map<LEXY_SYMBOL("any")>(TheoryAtomType::any)
    .map<LEXY_SYMBOL("directive")>(TheoryAtomType::directive);

static constexpr auto atom_type = dsl::symbol<sym_atom_type>(simple_keyword);

struct theory_op_definition {
    static constexpr auto rule = dsl::p<theory_op> >> dsl::colon + simple_number + dsl::comma + (unary | binary >> dsl::comma + associativity);
    static constexpr auto value = lexy::noop;
};

struct theory_term_definition {
    static constexpr auto rule = dsl::p<identifier> >> dsl::curly_bracketed.opt_list(dsl::p<theory_op_definition>);
    static constexpr auto value = lexy::noop;
};

struct theory_term_definitions {
    static constexpr auto rule = dsl::list(dsl::p<theory_op_definition>, dsl::sep(dsl::semicolon));
    static constexpr auto value = lexy::noop;
};

struct theory_guard_relations {
    static constexpr auto rule = dsl::list(dsl::p<theory_op>, dsl::sep(dsl::comma));
    static constexpr auto value = lexy::noop;
};

struct theory_guard_definition {
    static constexpr auto rule = dsl::curly_bracketed.opt(dsl::p<theory_guard_relations>) >> dsl::comma + dsl::p<identifier>;
    static constexpr auto value = lexy::noop;
};

// TODO: will have to be combined with theory_term_definitions
struct theory_atom_definition {
    static constexpr auto rule = dsl::p<identifier> >> dsl::slash + simple_number + dsl::colon + dsl::p<identifier> + dsl::comma + dsl::opt(dsl::p<theory_guard_definition> >> dsl::comma) + atom_type;
    static constexpr auto value = lexy::noop;
};

struct theory_atom_definitions {
    static constexpr auto rule = dsl::list(dsl::p<theory_atom_definition>, dsl::sep(dsl::semicolon));
    static constexpr auto value = lexy::noop;
};

struct statement_body {
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT("."));
        auto sep = dsl::sep(LEXY_LIT(",") / LEXY_LIT(";"));
        return dsl::opt(peek >> dsl::list(dsl::p<body_literal>, sep));
    }();
    static constexpr auto value = lexy::as_list<UBodyLiteralVec>;
};

struct statement_rule {
    static constexpr auto rule = []() {
        auto if_body = LEXY_LIT(":-") >> dsl::p<statement_body> + LEXY_LIT(".");
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
    static constexpr auto rule = dsl::p<statement_rule>;
    static constexpr auto value = lexy::forward<UStatement>;
};

} // namespace grammar
