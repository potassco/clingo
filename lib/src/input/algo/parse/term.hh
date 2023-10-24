#pragma once

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

    auto operator()(Location loc, auto name) const {
        return TermFunction{std::move(loc), name, PoolVec{TupleVec{}}, external};
    }

    auto operator()(Location loc, auto name, auto args) const {
        return TermFunction{std::move(loc), name, std::move(args), external};
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

struct store_string_ {
    using return_type = String;

    template <typename... Args>
    auto operator()(auto &state, Args &&...args) const
        -> decltype(state.string(Detail::as_string(std::forward<Args>(args)...))) {
        return state.string(Detail::as_string(std::forward<Args>(args)...));
    }

    template <class State> struct sink_callback_ {
        using return_type = String;

        constexpr sink_callback_(State &state) : state{state} {}

        template <typename... Args> auto operator()(Args &&...args) { return sink(std::forward<Args>(args)...); }

        auto finish() && -> String { return state.string(std::move(sink).finish()); }

        State &state;
        decltype(Detail::as_string.sink()) sink = Detail::as_string.sink();
    };

    constexpr auto sink(auto &state) const { return sink_callback_{state}; }
};

static constexpr auto as_stored_string = lexy::bind_sink(store_string_{}, lexy::parse_state) >>
                                         lexy::bind(store_string_{}, lexy::parse_state, lexy::values);

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
    static constexpr auto value = as_stored_string;
};

struct dec_number {
    static constexpr auto rule = dsl::capture(dsl::digits<>.sep(dsl::digit_sep_tick).no_leading_zero());
    static constexpr auto value = lexy::callback([](auto lex) {
        // TODO:
        // - the base must be passed to the constructor because also hex, oct,
        //   and bin numbers have to be parsed.
        // - the string has to be processed to discards the ticks
        return Number{std::string(lex.begin(), lex.end()).c_str()};
    });
};

static constexpr auto simple_number = dsl::integer<int>(dsl::digits<>.sep(dsl::digit_sep_tick).no_leading_zero());

static constexpr auto number = LEXY_LIT("0x") >> dsl::integer<int, dsl::hex> |
                               LEXY_LIT("0o") >> dsl::integer<int, dsl::octal> |
                               LEXY_LIT("0b") >> dsl::integer<int, dsl::binary> | simple_number;

struct term_number : lexy::token_production {
    static constexpr char const *name = "number";
    static constexpr auto rule = Detail::location(number);
    static constexpr auto value = lexy::callback_with_state<Term>(
        [](auto &state, Location loc, auto num) { return TermSymbol(std::move(loc), state.num(num)); });
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
    static constexpr auto rule = Detail::location(string);
    static constexpr auto value = as_stored_string >> lexy::callback<Term>([](Location loc, auto str) {
                                      return TermSymbol{std::move(loc), SymbolStore::str(str)};
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
    static constexpr auto value = lexy::callback_with_state<Term>([](auto &state, auto var) {
        return TermVariable{state.loc(var), state.string(Detail::as_string(var))};
    });
};

enum class Constant { supremum, infimum };

static constexpr auto constants = lexy::symbol_table<Constant> //
                                      .map<LEXY_SYMBOL("#infimum")>(Constant::infimum)
                                      .map<LEXY_SYMBOL("#inf")>(Constant::infimum)
                                      .map<LEXY_SYMBOL("#supremum")>(Constant::supremum)
                                      .map<LEXY_SYMBOL("#sup")>(Constant::supremum);

static constexpr auto constant = dsl::symbol<constants>(keyword_base);

struct term_constant : lexy::token_production {
    static constexpr char const *name = "constant";
    static constexpr auto rule = Detail::location(constant);
    static constexpr auto value = lexy::callback<Term>([](Location loc, auto val) {
        return TermSymbol(std::move(loc), val == Constant::infimum ? SymbolStore::inf() : SymbolStore::sup());
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
    static constexpr auto rule =
        Detail::location(dsl::p<identifier> >> dsl::if_(LEXY_LIT("(") >> dsl::p<term_function_pool> + LEXY_LIT(")")));
    static constexpr auto value = Detail::construct_function<false>{};
};

struct term_external_function {
    static constexpr char const *name = "function";
    static constexpr auto rule = Detail::location(
        LEXY_LIT("@") >> dsl::p<identifier> + dsl::if_(LEXY_LIT("(") >> dsl::p<term_function_pool> + LEXY_LIT(")")));
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
    static constexpr auto rule = Detail::location(
        LEXY_LIT("(") >> dsl::list(dsl::p<term_tuple_element>, dsl::sep(dsl::semicolon)) + LEXY_LIT(")"));
    static constexpr auto value = lexy::as_list<TermTuple::ElementVec> >>
                                  lexy::callback<Term>([](Location loc, TermTuple::ElementVec elem) -> Term {
                                      if (elem.size() == 1 && std::holds_alternative<Term>(elem.front())) {
                                          return std::move(std::get<Term>(elem.front()));
                                      }
                                      return TermTuple{std::move(loc), std::move(elem)};
                                  });
};

struct term_abs {
    static constexpr char const *name = "absolute value";
    static constexpr auto rule =
        Detail::location(dsl::brackets(LEXY_LIT("|"), LEXY_LIT("|")).list(dsl::p<term>, dsl::sep(dsl::semicolon)));
    static constexpr auto value = lexy::as_list<TermVec> >> lexy::callback<Term>([](Location loc, auto pool) {
                                      return TermAbs{std::move(loc), std::move(pool)};
                                  });
};

static constexpr auto anonymous_variable =
    dsl::not_followed_by(LEXY_LIT("_"), dsl::ascii::alpha_digit_underscore / LEXY_LIT("'"));

struct term_anonymous_variable : lexy::token_production {
    static constexpr char const *name = "anonymous variable";
    static constexpr auto rule = Detail::location(anonymous_variable);
    static constexpr auto value = lexy::callback_with_state<Term>([](auto &state, Location loc) {
        return TermVariable{std::move(loc), state.string("_"), true};
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
        template <class State> tag_unary(State &state, auto it) : pos{state.pos(it)} {}
        static constexpr auto op = OP;
        Position pos;
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
    static constexpr auto value = lexy::callback<Term>(
        lexy::forward<Term>,
        [](auto tag, Term rhs) {
            return TermUnary{Location{tag.pos, location(rhs).end}, tag.op, std::move(rhs)};
        },
        [](Term lhs, BinaryOperator op, Term rhs) {
            return TermBinary{Location{location(lhs).begin, location(rhs).end}, std::move(lhs), op, std::move(rhs)};
        });
};

} // namespace Gringo::Input::Grammar
