#pragma once

#include <any>
#include <optional>

#include <lexy/dsl.hpp>

#include <input/term.hh>

#include "base.hh"

namespace Gringo::Input::Grammar {

namespace Detail {

auto empty_args(std::optional<TermVecVec> value) {
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

template <bool external> struct construct_function {
    using return_type = TermFunction;

    auto operator()(Position begin, auto name, Position end) const {
        return TermFunction{Location{std::move(begin), std::move(end)}, std::move(name), PoolVec{TupleVec{}}, external};
    }

    auto operator()(Position begin, auto name, auto args, Position end) const {
        return TermFunction{Location{std::move(begin), std::move(end)}, std::move(name), std::move(args), external};
    }
};

auto empty_args(std::optional<PoolVec> value) { return std::move(value).value_or(PoolVec{TupleVec{}}); };

class tuple_trail {
  public:
    void push_back(std::monostate p) { vec_.emplace_back(p); }
    void push_front(std::monostate p) { vec_.emplace(vec_.begin(), p); }
    void push_back(Term term) { vec_.emplace_back(std::move(term)); }
    template <typename Reader> void push_back(lexy::lexeme<Reader> /* unused */) { trail_ = true; }
    auto to_tuple() -> TermTuple::Element {
        if (vec_.size() == 1 && !trail_ && std::holds_alternative<Term>(vec_.back())) {
            return std::get<Term>(std::move(vec_.back()));
        }
        return std::move(vec_);
    }

  private:
    TupleVec vec_;
    bool trail_ = false;
};

} // namespace Detail

static constexpr auto projection_symbol = lexy::symbol_table<std::monostate> //
                                              .map<'*'>(std::monostate{});
static constexpr auto identifier_base = []() {
    auto head = dsl::ascii::lower;
    auto tail = dsl::ascii::alpha_digit_underscore / LEXY_LIT("'");
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
    static constexpr auto value = Detail::as_string;
};

static constexpr auto simple_number = dsl::integer<int>(dsl::digits<>.sep(dsl::digit_sep_tick).no_leading_zero());

static constexpr auto number = LEXY_LIT("0x") >> dsl::integer<int, dsl::hex> |
                               LEXY_LIT("0o") >> dsl::integer<int, dsl::octal> |
                               LEXY_LIT("0b") >> dsl::integer<int, dsl::binary> | simple_number;

struct term_number : lexy::token_production {
    static constexpr char const *name = "number";
    static constexpr auto rule = Detail::position(number >> Detail::position);
    static constexpr auto value = lexy::callback<Term>([](Position begin, auto num, Position end) {
        return TermSymbol(Location{std::move(begin), std::move(end)}, num);
    });
};

static constexpr auto escaped_symbols = lexy::symbol_table<char> //
                                            .map<'"'>('"')
                                            .map<'\\'>('\\')
                                            .map<'n'>('\n')
                                            .map<'t'>('\t');
static constexpr auto string = [] {
    auto inner = dsl::code_point;
    auto escape = dsl::backslash_escape //
                      .symbol<escaped_symbols>()
                      .rule(dsl::lit_c<'u'> >> dsl::code_point_id<4>);
    return dsl::quoted(inner, escape);
}();

struct term_string : lexy::token_production {
    static constexpr char const *name = "string";
    static constexpr auto rule = Detail::position(string >> Detail::position);
    static constexpr auto value = Detail::as_string >> lexy::callback<Term>([](Position begin, auto str, Position end) {
                                      return TermSymbol{Location(std::move(begin), std::move(end)), QuotedString{str}};
                                  });
};

static constexpr auto variable = []() {
    auto prefix = dsl::while_(LEXY_LIT("_") / LEXY_LIT("'"));
    auto suffix = dsl::while_(dsl::ascii::alpha_digit_underscore / LEXY_LIT("'"));
    return prefix + dsl::ascii::upper + suffix;
}();

struct term_variable : lexy::token_production {
    static constexpr char const *name = "variable";
    static constexpr auto rule = dsl::capture(dsl::token(variable));
    static constexpr auto value = Detail::with_state<Term>([](auto &state, auto var) {
        return TermVariable{state.loc(var), Detail::as_string(var)};
    });
};

static constexpr auto constants = lexy::symbol_table<Constant> //
                                      .map<LEXY_SYMBOL("#infimum")>(Constant::infimum)
                                      .map<LEXY_SYMBOL("#inf")>(Constant::infimum)
                                      .map<LEXY_SYMBOL("#supremum")>(Constant::supremum)
                                      .map<LEXY_SYMBOL("#sup")>(Constant::supremum);

static constexpr auto constant = dsl::symbol<constants>(keyword_base);

struct term_constant : lexy::token_production {
    static constexpr char const *name = "constant";
    static constexpr auto rule = Detail::position(constant >> Detail::position);
    static constexpr auto value = lexy::callback<Term>([](Position begin, auto val, Position end) {
        return TermSymbol(Location(std::move(begin), std::move(end)), val);
    });
};

struct term {
    static constexpr char const *name = "term";
    static constexpr auto rule = dsl::recurse<struct term_rec>;
    static constexpr auto value = lexy::forward<Term>;
};

struct term_list {
    static constexpr char const *name = "list of terms";
    static constexpr auto rule = dsl::list(dsl::p<term>, dsl::sep(dsl::comma));
    static constexpr auto value = lexy::as_list<TermVec>;
};

struct term_function_tuple {
    static constexpr char const *name = "list of terms";
    static constexpr auto rule =
        dsl::list(dsl::symbol<projection_symbol> | dsl::else_ >> dsl::p<term>, dsl::sep(dsl::comma));
    static constexpr auto value = lexy::as_list<TupleVec>;
};

struct term_function_pool {
    static constexpr char const *name = "pool of terms";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(dsl::semicolon / LEXY_LIT(")"));
        auto item = dsl::opt(peek >> dsl::p<term_function_tuple>);
        auto sep = dsl::sep(dsl::semicolon);
        return dsl::list(item, sep);
    }();
    static constexpr auto value = lexy::collect<PoolVec>(lexy::as_list<TupleVec>);
};

struct term_function {
    static constexpr char const *name = "function";
    static constexpr auto
        rule = Detail::position(dsl::p<identifier>) >>
               dsl::if_(LEXY_LIT("(") >> dsl::p<term_function_pool> + LEXY_LIT(")")) + Detail::post_position;
    static constexpr auto value = Detail::construct_function<false>{};
};

struct term_external_function {
    static constexpr char const *name = "function";
    static constexpr auto
        rule = Detail::position(LEXY_LIT("@") >> dsl::p<identifier>) >>
               dsl::if_(LEXY_LIT("(") >> dsl::p<term_function_pool> + LEXY_LIT(")")) + Detail::post_position;
    static constexpr auto value = Detail::construct_function<true>{};
};

struct term_tuple_element {
    static constexpr char const *name = "term pool";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(dsl::semicolon / LEXY_LIT(")") / dsl::comma);
        auto sep = dsl::trailing_sep(dsl::capture(LEXY_LIT(",")));
        auto ps = dsl::symbol<projection_symbol>;
        return dsl::if_(ps >> dsl::comma) + dsl::if_(dsl::list(ps | peek >> dsl::p<term>, sep));
    }();
    static constexpr auto value = lexy::as_list<Detail::tuple_trail> >>
                                  lexy::callback<TermTuple::Element>([]() -> TupleVec { return {}; },
                                                                     [](std::monostate p) -> TupleVec { return {p}; },
                                                                     [](std::monostate p, Detail::tuple_trail elem) {
                                                                         elem.push_front(p);
                                                                         return elem.to_tuple();
                                                                     },
                                                                     [](Detail::tuple_trail elem) {
                                                                         return elem.to_tuple();
                                                                     });
};

struct term_tuple {
    static constexpr char const *name = "term tuple";
    // Note: dsl::parenthesized.list tries to be too clever.
    static constexpr auto
        rule = Detail::position(LEXY_LIT("(")) >>
               dsl::list(dsl::p<term_tuple_element>, dsl::sep(dsl::semicolon)) + Detail::post_position(LEXY_LIT(")"));
    static constexpr auto value = lexy::as_list<TermTuple::ElementVec> >>
                                  lexy::callback<Term>([](Position begin, TermTuple::ElementVec elem,
                                                          Position end) -> Term {
                                      if (elem.size() == 1 && std::holds_alternative<Term>(elem.front())) {
                                          return std::move(std::get<Term>(elem.front()));
                                      }
                                      return TermTuple{Location(std::move(begin), std::move(end)), std::move(elem)};
                                  });
};

struct term_abs {
    static constexpr char const *name = "absolute value";
    static constexpr auto rule = dsl::brackets(Detail::position(LEXY_LIT("|")), Detail::post_position(LEXY_LIT("|")))
                                     .list(dsl::p<term>, dsl::sep(dsl::semicolon));
    static constexpr auto value = lexy::as_list<TermVec> >>
                                  lexy::callback<Term>([](Position begin, auto pool, Position end) {
                                      return TermAbs{Location(std::move(begin), std::move(end)), std::move(pool)};
                                  });
};

static constexpr auto anonymous_variable =
    dsl::not_followed_by(LEXY_LIT("_"), dsl::ascii::alpha_digit_underscore / LEXY_LIT("'"));

struct term_anonymous_variable : lexy::token_production {
    static constexpr char const *name = "anonymous variable";
    static constexpr auto rule = Detail::position(anonymous_variable);
    static constexpr auto value = lexy::callback<Term>([](Position begin) {
        auto end = begin;
        end.column += 1;
        return TermVariable{Location(std::move(begin), std::move(end)), "_", true};
    });
};

struct term_rec : lexy::expression_production {
    static constexpr char const *name = "term";
    STRING_TAG(term, "expected term");

    static constexpr auto atom = dsl::p<term_number> | dsl::p<term_tuple> | dsl::p<term_variable> | dsl::p<term_abs> |
                                 dsl::p<term_external_function> | dsl::p<term_function> | dsl::p<term_string> |
                                 dsl::p<term_constant> | dsl::p<term_anonymous_variable> | dsl::error<expected_term>;

    struct op_power : dsl::infix_op_right {
        static constexpr char const *name = "exponentiation";
        static constexpr auto op = dsl::op<BinaryOperator::pow>(LEXY_LIT("**"));
        using operand = dsl::atom;
    };

    template <UnaryOperator OP> struct tag_unary {
        // TODO:
        // - The std::prev works around a potential bug in lexy, which should be reported.
        // - Ideally, the constructor would also be called with the state.
        //   Then, the actual position could be calculated here.
        //   Maybe I can ask for such an extension to avoid the ugly any.
        tag_unary(auto it) : it{std::prev(it)} {}
        static constexpr auto op = OP;
        std::any it;
    };

    struct op_unary : dsl::prefix_op {
        static constexpr char const *name = "unary";
        static constexpr auto op = dsl::op<tag_unary<UnaryOperator::negate>>(LEXY_LIT("-")) /
                                   dsl::op<tag_unary<UnaryOperator::invert>>(LEXY_LIT("~"));
        using operand = op_power;
    };

    struct op_product : dsl::infix_op_left {
        static constexpr char const *name = "product";
        static constexpr auto op = [] {
            auto star = dsl::not_followed_by(LEXY_LIT("*"), dsl::lit_c<'*'>);
            return dsl::op<BinaryOperator::times>(star) / dsl::op<BinaryOperator::div>(LEXY_LIT("/")) /
                   dsl::op<BinaryOperator::mod>(LEXY_LIT("\\"));
        }();
        using operand = op_unary;
    };

    struct op_sum : dsl::infix_op_left {
        static constexpr char const *name = "sum";
        static constexpr auto op =
            dsl::op<BinaryOperator::plus>(LEXY_LIT("+")) / dsl::op<BinaryOperator::minus>(LEXY_LIT("-"));
        using operand = op_product;
    };

    struct op_and : dsl::infix_op_left {
        static constexpr char const *name = "binary and";
        static constexpr auto op = dsl::op<BinaryOperator::and_>(LEXY_LIT("&"));
        using operand = op_sum;
    };

    struct op_or : dsl::infix_op_left {
        static constexpr char const *name = "binary or";
        static constexpr auto op = dsl::op<BinaryOperator::or_>(LEXY_LIT("?"));
        using operand = op_and;
    };

    struct op_xor : dsl::infix_op_left {
        static constexpr char const *name = "binary xor";
        static constexpr auto op = dsl::op<BinaryOperator::xor_>(LEXY_LIT("^"));
        using operand = op_or;
    };

    struct op_dots : dsl::infix_op_left {
        static constexpr char const *name = "interval";
        static constexpr auto op = dsl::op<BinaryOperator::dots>(LEXY_LIT(".."));
        using operand = op_xor;
    };

    using operation = op_dots;
    static constexpr auto value = Detail::with_state<Term>(
        [](auto &state, Term term) {
            static_cast<void>(state);
            return term;
        },
        [](auto &state, auto tag, Term rhs) {
            auto begin = state.pos(std::any_cast<typename std::remove_reference_t<decltype(state)>::iterator>(tag.it));
            return TermUnary{Location{begin, location(rhs).end}, tag.op, std::move(rhs)};
        },
        [](auto &state, Term lhs, BinaryOperator op, Term rhs) {
            static_cast<void>(state);
            return TermBinary{Location{location(lhs).begin, location(rhs).end}, std::move(lhs), op, std::move(rhs)};
        });
};

} // namespace Gringo::Input::Grammar
