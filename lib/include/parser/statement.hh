#pragma once

#include <statement.hh>

#include <parser/base.hh>
#include <parser/body_literal.hh>
#include <parser/head_literal.hh>

namespace grammar {

static constexpr auto simple_number = dsl::digits<>.sep(dsl::digit_sep_tick).no_leading_zero();

static constexpr auto simple_keyword = dsl::identifier(dsl::ascii::lower);

static constexpr auto unary = LEXY_KEYWORD("unary", simple_keyword);
static constexpr auto binary = LEXY_KEYWORD("binary", simple_keyword);

static constexpr auto sym_theory_op_types = lexy::symbol_table<TheoryOpType> //
                                                .map<LEXY_SYMBOL("left")>(TheoryOpType::binary_left)
                                                .map<LEXY_SYMBOL("right")>(TheoryOpType::binary_right);

static constexpr auto associativity = dsl::symbol<sym_theory_op_types>(simple_keyword);

static constexpr auto sym_atom_type = lexy::symbol_table<TheoryAtomType>
    .map<LEXY_SYMBOL("head")>(TheoryAtomType::head)
    .map<LEXY_SYMBOL("body")>(TheoryAtomType::body)
    .map<LEXY_SYMBOL("any")>(TheoryAtomType::any)
    .map<LEXY_SYMBOL("directive")>(TheoryAtomType::directive);

static constexpr auto atom_type = dsl::symbol<sym_atom_type>(simple_keyword);

struct theory_op_definition {
    static constexpr auto rule = dsl::p<theory_op> >> dsl::colon + simple_number + dsl::comma +
                                                          (unary | binary >> dsl::comma + associativity);
    static constexpr auto value = lexy::noop;
};

struct theory_guard_relations {
    static constexpr auto rule = dsl::list(dsl::p<theory_op>, dsl::sep(dsl::comma));
    static constexpr auto value = lexy::noop;
};

struct theory_guard_definition {
    static constexpr auto rule =
        dsl::curly_bracketed.opt(dsl::p<theory_guard_relations>) >> dsl::comma + dsl::p<identifier>;
    static constexpr auto value = lexy::noop;
};

struct theory_definitions {
    static constexpr auto is_atom_def = dsl::context_flag<theory_definitions>;
    static constexpr auto rule = []() {
        auto term_def = dsl::curly_bracketed.opt_list(dsl::p<theory_op_definition>);
        auto atom_def =
            dsl::slash >> is_atom_def.set() >> simple_number + dsl::colon + dsl::p<identifier> + dsl::comma +
                                                   dsl::opt(dsl::p<theory_guard_definition> >> dsl::comma) + atom_type;
        auto def = dsl::p<identifier> >> (atom_def | is_atom_def.is_reset() >> term_def);
        auto sep = dsl::sep(dsl::semicolon);
        return is_atom_def.create() + dsl::list(def, sep);
    }();
    static constexpr auto value = lexy::noop;
};

struct statement_theory {
    static constexpr auto rule = []() {
        auto kw_theory = LEXY_KEYWORD("#theory", keyword_base);
        return kw_theory >>
               dsl::p<identifier> + dsl::curly_bracketed.opt_list(dsl::p<theory_definitions>) + dsl::period;
    }();
    static constexpr auto value = lexy::noop >> lexy::callback<UStatement>([](std::string name, auto &&...args) {
                                      return std::make_unique<TheoryDefinition>(std::move(name));
                                  });
};

struct statement_body {
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(dsl::period);
        auto sep = dsl::sep(dsl::comma / dsl::semicolon);
        return dsl::opt(peek >> dsl::list(dsl::p<body_literal>, sep));
    }();
    static constexpr auto value = lexy::as_list<UBodyLiteralVec>;
};

struct statement_rule {
    static constexpr auto rule = []() {
        auto if_body = LEXY_LIT(":-") >> dsl::p<statement_body> + dsl::period;
        return if_body | dsl::else_ >> dsl::p<head_literal> + (dsl::period | if_body);
    }();
    static constexpr auto value = lexy::callback<UStatement>(
        lexy::new_<Rule, UStatement>,
        [](UHeadLiteral head) { return std::make_unique<Rule>(std::move(head), UBodyLiteralVec{}); },
        [](UBodyLiteralVec body) {
            return std::make_unique<Rule>(std::make_unique<Disjunction>(Disjunction::ElementVec{}), std::move(body));
        });
};

struct statement {
    static constexpr auto rule = dsl::p<statement_theory> | dsl::else_ >> dsl::p<statement_rule>;
    static constexpr auto value = lexy::forward<UStatement>;
};

} // namespace grammar
