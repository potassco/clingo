#pragma once

#include <gringo/input/algo/evaluate.hh>

#include <gringo/core/symbol.hh>

#include <lexy/callback.hpp>
#include <lexy/callback/string.hpp>
#include <lexy/dsl.hpp>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define STRING_TAG(n, v)                                                                                               \
    struct expected_##n {                                                                                              \
        static constexpr char const *name = v;                                                                         \
    }

namespace Gringo::Input::SymbolGrammar {

namespace dsl = lexy::dsl;

using encoding = lexy::utf8_encoding;

struct control {
    static constexpr auto whitespace = dsl::ascii::newline / dsl::ascii::space;
};

template <Base B, class LB> struct bigint {
    static constexpr auto rule = dsl::capture(dsl::digits<LB>.sep(dsl::digit_sep_tick).no_leading_zero());
    static constexpr auto value = lexy::callback<Number>([](auto lex) {
        auto rep = std::string(lex.begin(), lex.end());
        rep.erase(std::remove(rep.begin(), rep.end(), '\''), rep.end());
        return Number{rep.c_str(), B};
    });
};

static constexpr auto
    number = LEXY_LIT("0x") >> dsl::p<bigint<Base::hex, dsl::hex>> |
             LEXY_LIT("0o") >> dsl::p<bigint<Base::oct, dsl::octal>> |
             LEXY_LIT("0b") >> dsl::p<bigint<Base::bin, dsl::binary>> | dsl::p<bigint<Base::dec, dsl::decimal>>;

static constexpr auto escaped_symbols = lexy::symbol_table<char> //
                                            .map<'"'>('"')
                                            .map<'\\'>('\\')
                                            .map<'n'>('\n')
                                            .map<'t'>('\t');

enum class Constant : uint8_t { supremum, infimum };

static constexpr auto constants = lexy::symbol_table<Constant> //
                                      .map<LEXY_SYMBOL("#infimum")>(Constant::infimum)
                                      .map<LEXY_SYMBOL("#inf")>(Constant::infimum)
                                      .map<LEXY_SYMBOL("#supremum")>(Constant::supremum)
                                      .map<LEXY_SYMBOL("#sup")>(Constant::supremum);

static constexpr auto keyword_base = dsl::identifier(LEXY_ASCII_ONE_OF("#"), dsl::ascii::alpha);

static constexpr auto constant = dsl::symbol<constants>(keyword_base);

static constexpr auto identifier_base = []() {
    auto head = dsl::ascii::lower;
    auto tail = dsl::ascii::alpha_digit_underscore / LEXY_LIT("'");
    return dsl::identifier(head, tail);
}();

static constexpr auto kw_not = LEXY_KEYWORD("not", identifier_base);

struct identifier : lexy::token_production {
    static constexpr char const *name = "identifier";
    static constexpr auto rule = []() {
        auto prefix = dsl::while_one(LEXY_LIT("_") / LEXY_LIT("'"));
        return identifier_base.reserve(kw_not) | dsl::capture(dsl::token(prefix + identifier_base));
    }();
    static constexpr auto value = lexy::callback_with_state<String>(
        [](SymbolStore &store, auto str) { return store.string_ref(lexy::as_string<std::string, encoding>(str)); });
};

struct symbol {
    static constexpr char const *name = "symbol";
    static constexpr auto rule = dsl::recurse<struct symbol_rec>;
    static constexpr auto value = lexy::forward<Symbol>;
};

struct symbol_number : lexy::token_production {
    static constexpr char const *name = "number";
    static constexpr auto rule = number;
    static constexpr auto value = lexy::callback_with_state<Symbol>(
        [](SymbolStore &store, auto num) { return store.num_ref(Number{std::move(num)}); });
};

struct symbol_string : lexy::token_production {
    static constexpr char const *name = "string";
    static constexpr auto rule = [] {
        auto inner = dsl::code_point;
        auto escape = dsl::backslash_escape //
                          .symbol<escaped_symbols>()
                          .rule(dsl::lit_c<'u'> >> dsl::code_point_id<4>);
        return dsl::quoted(inner, escape);
    }();

    static constexpr auto value = lexy::as_string<std::string, encoding> >>
                                  lexy::callback_with_state<Symbol>([](SymbolStore &store, auto str) {
                                      return SymbolStore::str_ref(store.string_ref(str));
                                  });
};

struct symbol_constant : lexy::token_production {
    static constexpr char const *name = "constant";
    static constexpr auto rule = constant;
    static constexpr auto value = lexy::callback<Symbol>(
        [](auto val) { return val == Constant::infimum ? SymbolStore::inf() : SymbolStore::sup(); });
};

class tuple_trail {
  public:
    void push_back(Symbol sym) { vec_.emplace_back(sym); }
    template <typename Reader> void push_back([[maybe_unused]] lexy::lexeme<Reader> x) { trail_ = true; }
    auto get(SymbolStore &store) const -> Symbol {
        if (vec_.size() == 1 && !trail_) {
            return vec_.back();
        }
        return store.tup_ref(vec_);
    }

  private:
    std::vector<Symbol> vec_;
    bool trail_ = false;
};

struct symbol_tuple {
    static constexpr char const *name = "symbol tuple";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(")") / dsl::comma);
        auto sep = dsl::trailing_sep(dsl::capture(LEXY_LIT(",")));
        return LEXY_LIT("(") >> dsl::if_(dsl::list(peek >> dsl::p<symbol>, sep)) + LEXY_LIT(")");
    }();
    static constexpr auto value =
        lexy::as_list<tuple_trail> >>
        lexy::callback_with_state<Symbol>([](SymbolStore &store) -> Symbol { return store.tup_ref(SymbolSpan{}); },
                                          [](SymbolStore &store, tuple_trail const &trail) -> Symbol {
                                              return trail.get(store);
                                          });
};

struct symbol_function {
    static constexpr char const *name = "function symbol";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(")") / dsl::comma);
        auto sep = dsl::sep(LEXY_LIT(","));
        return dsl::p<identifier> >>
               dsl::if_(LEXY_LIT("(") >> dsl::if_(dsl::list(peek >> dsl::p<symbol>, sep)) + LEXY_LIT(")"));
    }();
    static constexpr auto
        value = lexy::as_list<SymbolVec> >>
                lexy::callback_with_state<Symbol>([](SymbolStore &store,
                                                     String name) -> Symbol { return store.fun_ref(name, {}, false); },
                                                  [](SymbolStore &store, String name, SymbolVec const &vec) -> Symbol {
                                                      return store.fun_ref(name, vec, false);
                                                  });
};

struct symbol_rec : lexy::expression_production {
    static constexpr char const *name = "symbol";
    STRING_TAG(symbol, "expected symbol");

    static constexpr auto atom = dsl::p<symbol_number> | dsl::p<symbol_tuple> | dsl::p<symbol_function> |
                                 dsl::p<symbol_string> | dsl::p<symbol_constant> | dsl::error<expected_symbol>;

    struct op_power : dsl::infix_op_right {
        static constexpr char const *name = "exponentiation";
        static constexpr auto op = dsl::op<BinaryOperator::pow>(LEXY_LIT("**"));
        using operand = dsl::atom;
    };

    struct op_unary : dsl::prefix_op {
        static constexpr char const *name = "unary";
        static constexpr auto op =
            dsl::op<UnaryOperator::negate>(LEXY_LIT("-")) / dsl::op<UnaryOperator::invert>(LEXY_LIT("~"));
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

    using operation = op_xor;
    static constexpr auto value =
        lexy::callback_with_state<Symbol>([]([[maybe_unused]] SymbolStore &store, Symbol sym) { return sym; },
                                          [](SymbolStore &store, UnaryOperator op, Symbol rhs) {
                                              if (auto res = evaluate(store, op, rhs); res) {
                                                  return *res;
                                              }
                                              throw std::runtime_error("could not parse symbol");
                                          },
                                          [](SymbolStore &store, Symbol lhs, BinaryOperator op, Symbol rhs) {
                                              if (auto res = evaluate(store, lhs, op, rhs); res) {
                                                  return *res;
                                              }
                                              throw std::runtime_error("could not parse symbol");
                                          });
};

struct program_param {
    static constexpr char const *name = "param vector";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(")") / dsl::comma);
        auto sep = dsl::sep(LEXY_LIT(","));
        return dsl::p<identifier> >>
               dsl::if_(LEXY_LIT("(") >> dsl::if_(dsl::list(peek >> dsl::p<symbol>, sep)) + LEXY_LIT(")"));
    }();
    static constexpr auto
        value = lexy::as_list<SymbolVec> >>
                lexy::callback<ProgramParam>([](String name) -> ProgramParam { return {SharedString{name}, {}}; },
                                             [](String name, SymbolVec const &vec) -> ProgramParam {
                                                 auto span = as_shared_symbol_span(vec);
                                                 return {SharedString{name}, {span.begin(), span.end()}};
                                             });
};

struct program_param_vec {
    static constexpr char const *name = "parameter list";
    static constexpr auto rule = []() {
        auto sep = dsl::sep(LEXY_LIT(","));
        return dsl::list(dsl::p<program_param>, sep);
    }();
    static constexpr auto value = lexy::as_list<ProgramParamVec>;
};

struct program_param_vec_vec {
    static constexpr char const *name = "list of parameter lists";
    static constexpr auto rule = []() {
        auto sep = dsl::sep(dsl::semicolon);
        return dsl::list(dsl::p<program_param_vec>, sep);
    }();
    static constexpr auto value = lexy::as_list<std::vector<ProgramParamVec>>;
};

} // namespace Gringo::Input::SymbolGrammar
