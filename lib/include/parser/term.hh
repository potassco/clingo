#pragma once

#include <optional>

#include <lexy/dsl.hpp>

#include <util/lexy_report_error.hh>
#include <util/lexy_stream_input.hh>

#include <parser/base.hh>
#include <term.hh>

namespace grammar {

namespace dsl = lexy::dsl;

struct variable : lexy::token_production {
    static constexpr auto rule = []() {
        auto prefix = dsl::while_(LEXY_LIT("_") / LEXY_LIT("'"));
        auto suffix = dsl::while_(dsl::ascii::alpha_underscore / LEXY_LIT("'"));
        return dsl::capture(dsl::token(prefix + dsl::ascii::upper + suffix));
    }();
    static constexpr auto value = lexy::callback<UTerm>(
        [](lexeme lex) { return std::make_unique<TermVariable>(std::string(lex.begin(), lex.end())); });
};

struct identifier : lexy::token_production {
    static constexpr auto rule = []() {
        auto prefix = dsl::while_one(LEXY_LIT("_") / LEXY_LIT("'"));
        auto head = dsl::ascii::lower;
        auto tail = dsl::ascii::alpha_underscore / LEXY_LIT("'");
        auto id = dsl::identifier(head, tail);
        auto kw_not = LEXY_KEYWORD("not", id);

        return id.reserve(kw_not) | dsl::capture(dsl::token(prefix + id));
    }();
    static constexpr auto value = lexy::as_string<std::string>;
};

struct number : lexy::token_production {
    static constexpr auto rule = []() {
        auto digits = dsl::digits<>.sep(dsl::digit_sep_tick).no_leading_zero();
        return LEXY_LIT("0x") >> dsl::integer<int, dsl::hex> | LEXY_LIT("0o") >> dsl::integer<int, dsl::octal> |
               LEXY_LIT("0b") >> dsl::integer<int, dsl::binary> | dsl::integer<int>(digits);
    }();
    static constexpr auto value = lexy::forward<int>;
};

struct string : lexy::token_production {
    static constexpr auto escaped_symbols = lexy::symbol_table<char> //
                                                .map<'"'>('"')
                                                .map<'\\'>('\\')
                                                .map<'n'>('\n')
                                                .map<'t'>('\t');

    static constexpr auto rule = [] {
        auto inner = -dsl::ascii::control;
        auto escape = dsl::backslash_escape //
                          .symbol<escaped_symbols>()
                          .rule(dsl::lit_c<'u'> >> dsl::code_point_id<4>);
        return dsl::quoted(inner, escape);
    }();

    static constexpr auto value = lexy::as_string<std::string, lexy::utf8_encoding> >> lexy::new_<TermString, UTerm>;
};

struct constant : lexy::token_production {
    static constexpr auto constants = lexy::symbol_table<Constant> //
                                          .map<LEXY_SYMBOL("infimum")>(Constant::infimum)
                                          .map<LEXY_SYMBOL("inf")>(Constant::infimum)
                                          .map<LEXY_SYMBOL("supremum")>(Constant::supremum)
                                          .map<LEXY_SYMBOL("sup")>(Constant::supremum);

    static constexpr auto rule = [] {
        auto name = dsl::identifier(dsl::ascii::alpha);
        auto reference = dsl::symbol<constants>(name);
        return dsl::lit_c<'#'> >> reference;
    }();

    static constexpr auto value = lexy::new_<TermConstant, UTerm>;
};

struct term {
    static constexpr auto rule = dsl::recurse<struct expr>;
    static constexpr auto value = lexy::forward<UTerm>;
};

struct term_tuple {
    static constexpr auto rule = dsl::list(dsl::p<term>, dsl::sep(dsl::comma));
    static constexpr auto value = lexy::as_list<UTermVec>;
};

struct term_pool {
    // Note: dsl::parenthesized.list tries to be too clever.
    static constexpr auto rule = LEXY_LIT("(") >> dsl::list(dsl::opt(dsl::peek_not(dsl::semicolon / LEXY_LIT(")")) >>
                                                                     dsl::p<term_tuple>),
                                                            dsl::sep(dsl::semicolon)) +
                                                      LEXY_LIT(")");
    static constexpr auto value = lexy::collect<UTermVecVec>(lexy::as_list<UTermVec>);
};

constexpr auto empty_args_ = [](std::optional<UTermVecVec> value) {
    if (value.has_value()) {
        return std::move(value.value());
    }
    UTermVecVec ret;
    ret.emplace_back();
    return ret;
};

struct function {
    static constexpr auto rule = dsl::p<identifier> >> dsl::opt(dsl::p<term_pool>);
    static constexpr auto value =
        lexy::bind(lexy::new_<TermFunction, UTerm>, lexy::_1, lexy::_2.map(empty_args_), false);
};

struct external_function {
    static constexpr auto rule = LEXY_LIT("@") >> dsl::p<identifier> + dsl::opt(dsl::p<term_pool>);
    static constexpr auto value =
        lexy::bind(lexy::new_<TermFunction, UTerm>, lexy::_1, lexy::_2.map(empty_args_), true);
};

struct tuple_term_element {
    // Note: the std::optional is for C++17 compatibility
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(dsl::semicolon / LEXY_LIT(")") / dsl::comma);
        auto sep = dsl::capture(LEXY_LIT(","));
        auto tuple = dsl::list(peek >> dsl::p<term>, dsl::trailing_sep(sep));
        return dsl::comma | dsl::else_ >> dsl::if_(tuple);
    }();
    static constexpr auto value = []() {
        auto sink = lexy::fold_inplace<std::optional<TermTuple::Element>>(
            std::nullopt,
            [](std::optional<TermTuple::Element> &sink, UTerm term) {
                if (!sink.has_value()) {
                    sink = std::move(term);
                } else {
                    std::get<UTermVec>(sink.value()).emplace_back(std::move(term));
                }
            },
            [](std::optional<TermTuple::Element> &sink, lexeme sep) {
                if (!sink.has_value()) {
                    sink = UTermVec{};
                } else if (std::holds_alternative<UTerm>(sink.value())) {
                    auto vec = UTermVec{};
                    vec.emplace_back(std::move(std::get<UTerm>(sink.value())));
                    sink = std::move(vec);
                }
            });
        auto callback =
            lexy::callback<TermTuple::Element>(lexy::construct<UTermVec>, [](std::optional<TermTuple::Element> tuple) {
                return std::move(tuple).value();
            });

        return sink >> callback;
    }();
};

struct tuple_term {
    // Note: dsl::parenthesized.list tries to be too clever.
    static constexpr auto rule = LEXY_LIT("(") >>
                                 dsl::list(dsl::p<tuple_term_element>, dsl::sep(dsl::semicolon)) + LEXY_LIT(")");
    static constexpr auto value = lexy::as_list<TermTuple::ElementVec> >>
                                  lexy::callback<UTerm>([](TermTuple::ElementVec elem) -> UTerm {
                                      if (elem.size() == 1 && std::holds_alternative<UTerm>(elem.front())) {
                                          return std::move(std::get<UTerm>(elem.front()));
                                      }
                                      return std::make_unique<TermTuple>(std::move(elem));
                                  });
};

struct math_abs {
    static constexpr auto rule =
        dsl::brackets(LEXY_LIT("|"), LEXY_LIT("|")).list(dsl::p<term>, dsl::sep(dsl::semicolon));
    static constexpr auto value = lexy::as_list<UTermVec> >> lexy::new_<TermAbs, UTerm>;
};

struct anonymous_variable {
    static constexpr auto rule =
        dsl::capture(dsl::not_followed_by(LEXY_LIT("_"), dsl::ascii::alpha_digit_underscore / LEXY_LIT("'")));
    static constexpr auto value = lexy::as_string<std::string> | lexy::new_<TermVariable, UTerm>;
};

struct expr : lexy::expression_production {
    struct expected_term {
        static constexpr auto name = "expected term";
    };

    static constexpr auto atom = dsl::p<number> | dsl::p<tuple_term> | dsl::p<variable> | dsl::p<math_abs> |
                                 dsl::p<external_function> | dsl::p<function> | dsl::p<string> | dsl::p<constant> |
                                 dsl::p<anonymous_variable> | dsl::error<expected_term>;

    struct math_power : dsl::infix_op_right {
        static constexpr auto op = dsl::op<BinaryOperator::pow>(LEXY_LIT("**"));
        using operand = dsl::atom;
    };

    struct math_prefix : dsl::prefix_op {
        static constexpr auto op =
            dsl::op<UnaryOperator::negate>(LEXY_LIT("-")) / dsl::op<UnaryOperator::invert>(LEXY_LIT("~"));
        using operand = math_power;
    };

    struct math_product : dsl::infix_op_left {
        static constexpr auto op = [] {
            auto star = dsl::not_followed_by(LEXY_LIT("*"), dsl::lit_c<'*'>);
            return dsl::op<BinaryOperator::times>(star) / dsl::op<BinaryOperator::div>(LEXY_LIT("/")) /
                   dsl::op<BinaryOperator::mod>(LEXY_LIT("\\"));
        }();
        using operand = math_prefix;
    };

    struct math_sum : dsl::infix_op_left {
        static constexpr auto op =
            dsl::op<BinaryOperator::plus>(LEXY_LIT("+")) / dsl::op<BinaryOperator::minus>(LEXY_LIT("-"));
        using operand = math_product;
    };

    struct math_and : dsl::infix_op_left {
        static constexpr auto op = dsl::op<BinaryOperator::and_>(LEXY_LIT("&"));
        using operand = math_sum;
    };

    struct math_or : dsl::infix_op_left {
        static constexpr auto op = dsl::op<BinaryOperator::and_>(LEXY_LIT("?"));
        using operand = math_and;
    };

    struct math_xor : dsl::infix_op_left {
        static constexpr auto op = dsl::op<BinaryOperator::xor_>(LEXY_LIT("^"));
        using operand = math_or;
    };

    struct math_dots : dsl::infix_op_left {
        static constexpr auto op = dsl::op<BinaryOperator::xor_>(LEXY_LIT(".."));
        using operand = math_xor;
    };

    using operation = math_dots;
    static constexpr auto value = lexy::callback(lexy::forward<UTerm>, lexy::new_<TermInteger, UTerm>,
                                                 lexy::new_<TermUnary, UTerm>, lexy::new_<TermBinary, UTerm>);
};

} // namespace grammar
