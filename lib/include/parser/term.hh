#pragma once

#include <optional>

#include <lexy/dsl.hpp>

#include <util/lexy_report_error.hh>
#include <util/lexy_stream_input.hh>

#include <parser/base.hh>
#include <term.hh>

namespace grammar {

namespace dsl = lexy::dsl;

namespace detail {

auto empty_args(std::optional<STermVecVec> value) {
    PoolVec ret;
    if (value.has_value()) {
        ret.reserve(value->size());
        for (auto &tuple : value.value()) {
            ret.emplace_back();
            ret.back().reserve(tuple.size());
            for (auto &term : tuple) {
                ret.back().emplace_back(std::move(term));
            }
        }
    } else {
        ret.emplace_back();
    }
    return ret;
};

auto empty_args(std::optional<PoolVec> value) { return std::move(value).value_or(PoolVec{TupleVec{}}); };

class element_trail_vec {
  public:
    void push_back(std::monostate p) { vec_.emplace_back(p); }
    void push_front(std::monostate p) { vec_.emplace(vec_.begin(), p); }
    void push_back(STerm term) { vec_.emplace_back(std::move(term)); }
    template <typename Reader> void push_back(lexy::lexeme<Reader> /* unused */) { trail_ = true; }
    auto to_tuple() -> TermTuple::Element {
        if (vec_.size() == 1 && !trail_ && std::holds_alternative<STerm>(vec_.back())) {
            return std::get<STerm>(std::move(vec_.back()));
        }
        return std::move(vec_);
    }

  private:
    TupleVec vec_;
    bool trail_ = false;
};

struct construct_symbol {
    using return_type = STerm;

    auto operator()(int value) const -> STerm { return construct_shared<TermSymbol, Term>(Symbol{value}); }

    auto operator()(std::string value) const -> STerm {
        return construct_shared<TermSymbol, Term>(Symbol{QuotedString{value}});
    }

    auto operator()(Constant value) const -> STerm { return construct_shared<TermSymbol, Term>(Symbol{value}); }
};

} // namespace detail

static constexpr auto projection_symbol = lexy::symbol_table<std::monostate> //
                                              .map<'*'>(std::monostate{});
static constexpr auto identifier_base = []() {
    auto head = dsl::ascii::lower;
    auto tail = dsl::ascii::alpha_underscore / LEXY_LIT("'");
    return dsl::identifier(head, tail);
}();
static constexpr auto keyword_base = dsl::identifier(LEXY_ASCII_ONE_OF("#"), dsl::ascii::alpha);

static constexpr auto kw_not = LEXY_KEYWORD("not", identifier_base);

struct identifier : lexy::token_production {
    static constexpr char const *name = "identifier";
    static constexpr auto rule = []() {
        auto prefix = dsl::while_one(LEXY_LIT("_") / LEXY_LIT("'"));
        return identifier_base.reserve(kw_not) | dsl::capture(dsl::token(prefix + identifier_base));
    }();
    static constexpr auto value = lexy::as_string<std::string>;
};

static constexpr auto simple_number = dsl::integer<int>(dsl::digits<>.sep(dsl::digit_sep_tick).no_leading_zero());

struct number : lexy::token_production {
    static constexpr char const *name = "number";
    static constexpr auto rule = LEXY_LIT("0x") >> dsl::integer<int, dsl::hex> |
                                 LEXY_LIT("0o") >> dsl::integer<int, dsl::octal> |
                                 LEXY_LIT("0b") >> dsl::integer<int, dsl::binary> | simple_number;
    static constexpr auto value = lexy::forward<int>;
};

struct string : lexy::token_production {
    static constexpr char const *name = "string";
    static constexpr auto escaped_symbols = lexy::symbol_table<char> //
                                                .map<'"'>('"')
                                                .map<'\\'>('\\')
                                                .map<'n'>('\n')
                                                .map<'t'>('\t');

    static constexpr auto rule = [] {
        auto inner = dsl::code_point;
        auto escape = dsl::backslash_escape //
                          .symbol<escaped_symbols>()
                          .rule(dsl::lit_c<'u'> >> dsl::code_point_id<4>);
        return dsl::quoted(inner, escape);
    }();

    static constexpr auto value = lexy::as_string<std::string, encoding>;
};

struct variable {
    static constexpr char const *name = "variable";
    static constexpr auto rule = []() {
        auto prefix = dsl::while_(LEXY_LIT("_") / LEXY_LIT("'"));
        auto suffix = dsl::while_(dsl::ascii::alpha_underscore / LEXY_LIT("'"));
        return dsl::capture(dsl::token(prefix + dsl::ascii::upper + suffix));
    }();
    static constexpr auto value = lexy::as_string<std::string, encoding>;
};

struct term_variable : lexy::token_production {
    static constexpr char const *name = "variable";
    static constexpr auto rule = dsl::p<variable>;
    static constexpr auto value = lexy::new_<TermVariable, STerm>;
};

static constexpr auto constants = lexy::symbol_table<Constant> //
                                      .map<LEXY_SYMBOL("#infimum")>(Constant::infimum)
                                      .map<LEXY_SYMBOL("#inf")>(Constant::infimum)
                                      .map<LEXY_SYMBOL("#supremum")>(Constant::supremum)
                                      .map<LEXY_SYMBOL("#sup")>(Constant::supremum);

static constexpr auto constant = dsl::symbol<constants>(keyword_base);

struct term {
    static constexpr char const *name = "term";
    static constexpr auto rule = dsl::recurse<struct term_rec>;
    static constexpr auto value = lexy::forward<STerm>;
};

struct term_list {
    static constexpr char const *name = "list of terms";
    static constexpr auto rule = dsl::list(dsl::p<term>, dsl::sep(dsl::comma));
    static constexpr auto value = lexy::as_list<STermVec>;
};

struct term_function_tuple {
    static constexpr char const *name = "list of terms";
    static constexpr auto rule =
        dsl::list(dsl::symbol<projection_symbol> | dsl::else_ >> dsl::p<term>, dsl::sep(dsl::comma));
    static constexpr auto value = lexy::as_list<TupleVec>;
};

struct term_function_pool {
    static constexpr char const *name = "pool of terms";
    // Note: dsl::parenthesized.list tries to be too clever.
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(dsl::semicolon / LEXY_LIT(")"));
        auto item = dsl::opt(peek >> dsl::p<term_function_tuple>);
        auto sep = dsl::sep(dsl::semicolon);
        return LEXY_LIT("(") >> dsl::list(item, sep) + LEXY_LIT(")");
    }();
    static constexpr auto value = lexy::collect<PoolVec>(lexy::as_list<TupleVec>);
};

struct term_function {
    static constexpr char const *name = "function";
    static constexpr auto rule = dsl::p<identifier> >> dsl::opt(dsl::p<term_function_pool>);
    static constexpr auto value = lexy::callback<STerm>([](std::string name, std::optional<PoolVec> value) {
        return construct_shared<TermFunction, Term>(std::move(name), detail::empty_args(std::move(value)), false);
    });
};

struct term_external_function {
    static constexpr char const *name = "function";
    static constexpr auto rule = LEXY_LIT("@") >> dsl::p<identifier> + dsl::opt(dsl::p<term_function_pool>);
    static constexpr auto value = lexy::callback<STerm>([](std::string name, std::optional<PoolVec> value) {
        return construct_shared<TermFunction, Term>(std::move(name), detail::empty_args(std::move(value)), true);
    });
};

struct term_tuple_element {
    static constexpr char const *name = "term pool";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(dsl::semicolon / LEXY_LIT(")") / dsl::comma);
        auto sep = dsl::trailing_sep(dsl::capture(LEXY_LIT(",")));
        auto ps = dsl::symbol<projection_symbol>;
        return dsl::if_(ps >> dsl::comma) + dsl::if_(dsl::list(ps | peek >> dsl::p<term>, sep));
    }();
    static constexpr auto
        value = lexy::as_list<detail::element_trail_vec> >>
                lexy::callback<TermTuple::Element>([]() -> TupleVec { return {}; },
                                                   [](std::monostate p) -> TupleVec { return {p}; },
                                                   [](std::monostate p, detail::element_trail_vec elem) {
                                                       elem.push_front(p);
                                                       return elem.to_tuple();
                                                   },
                                                   [](detail::element_trail_vec elem) { return elem.to_tuple(); });
};

struct term_tuple {
    static constexpr char const *name = "term tuple";
    // Note: dsl::parenthesized.list tries to be too clever.
    static constexpr auto rule = LEXY_LIT("(") >>
                                 dsl::list(dsl::p<term_tuple_element>, dsl::sep(dsl::semicolon)) + LEXY_LIT(")");
    static constexpr auto value = lexy::as_list<TermTuple::ElementVec> >>
                                  lexy::callback<STerm>([](TermTuple::ElementVec elem) -> STerm {
                                      if (elem.size() == 1 && std::holds_alternative<STerm>(elem.front())) {
                                          return std::move(std::get<STerm>(elem.front()));
                                      }
                                      return construct_shared<TermTuple, Term>(std::move(elem));
                                  });
};

struct term_abs {
    static constexpr char const *name = "absolute value";
    static constexpr auto rule =
        dsl::brackets(LEXY_LIT("|"), LEXY_LIT("|")).list(dsl::p<term>, dsl::sep(dsl::semicolon));
    static constexpr auto value = lexy::as_list<STermVec> >> lexy::new_<TermAbs, STerm>;
};

static constexpr auto anonymous_variable =
    dsl::not_followed_by(LEXY_LIT("_"), dsl::ascii::alpha_digit_underscore / LEXY_LIT("'"));

struct term_anonymous_variable {
    static constexpr char const *name = "anonymous variable";
    static constexpr auto rule = anonymous_variable;
    static constexpr auto value = lexy::callback<STerm>([]() { return construct_shared<TermVariable, Term>("_"); });
};

struct term_rec : lexy::expression_production {
    static constexpr char const *name = "term";
    STRING_TAG(term, "expected term");

    static constexpr auto atom = dsl::p<number> | dsl::p<term_tuple> | dsl::p<term_variable> | dsl::p<term_abs> |
                                 dsl::p<term_external_function> | dsl::p<term_function> | dsl::p<string> | constant |
                                 dsl::p<term_anonymous_variable> | dsl::error<expected_term>;

    struct term_power : dsl::infix_op_right {
        static constexpr char const *name = "exponentiation";
        static constexpr auto op = dsl::op<BinaryOperator::pow>(LEXY_LIT("**"));
        using operand = dsl::atom;
    };

    struct term_unary : dsl::prefix_op {
        static constexpr char const *name = "inverse";
        static constexpr auto op =
            dsl::op<UnaryOperator::negate>(LEXY_LIT("-")) / dsl::op<UnaryOperator::invert>(LEXY_LIT("~"));
        using operand = term_power;
    };

    struct term_product : dsl::infix_op_left {
        static constexpr char const *name = "product";
        static constexpr auto op = [] {
            auto star = dsl::not_followed_by(LEXY_LIT("*"), dsl::lit_c<'*'>);
            return dsl::op<BinaryOperator::times>(star) / dsl::op<BinaryOperator::div>(LEXY_LIT("/")) /
                   dsl::op<BinaryOperator::mod>(LEXY_LIT("\\"));
        }();
        using operand = term_unary;
    };

    struct term_sum : dsl::infix_op_left {
        static constexpr char const *name = "sum";
        static constexpr auto op =
            dsl::op<BinaryOperator::plus>(LEXY_LIT("+")) / dsl::op<BinaryOperator::minus>(LEXY_LIT("-"));
        using operand = term_product;
    };

    struct term_and : dsl::infix_op_left {
        static constexpr char const *name = "binary and";
        static constexpr auto op = dsl::op<BinaryOperator::and_>(LEXY_LIT("&"));
        using operand = term_sum;
    };

    struct term_or : dsl::infix_op_left {
        static constexpr char const *name = "binary or";
        static constexpr auto op = dsl::op<BinaryOperator::and_>(LEXY_LIT("?"));
        using operand = term_and;
    };

    struct term_xor : dsl::infix_op_left {
        static constexpr char const *name = "binary xor";
        static constexpr auto op = dsl::op<BinaryOperator::xor_>(LEXY_LIT("^"));
        using operand = term_or;
    };

    struct term_dots : dsl::infix_op_left {
        static constexpr char const *name = "interval";
        static constexpr auto op = dsl::op<BinaryOperator::xor_>(LEXY_LIT(".."));
        using operand = term_xor;
    };

    using operation = term_dots;
    static constexpr auto value = lexy::callback(lexy::forward<STerm>, detail::construct_symbol{},
                                                 lexy::new_<TermUnary, STerm>, lexy::new_<TermBinary, STerm>);
};

} // namespace grammar
