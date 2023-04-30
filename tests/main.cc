#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include <lexy/action/parse.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/grammar.hpp>
#include <variant>
#include <vector>

#include "util/lexy_report_error.hh"
#include "util/lexy_stream_input.hh"

namespace {

struct Term {
    virtual ~Term() = default;
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    friend auto operator<<(std::ostream &out, Term const &term) -> std::ostream & {
        term.print(out);
        return out;
    }
};

using UTerm = std::unique_ptr<Term>;

enum class Constant {
    supremum,
    infimum,
};

auto operator<<(std::ostream &out, Constant op) -> std::ostream & {
    switch (op) {
        case Constant::supremum: {
            out << "#sup";
            break;
        }
        case Constant::infimum: {
            out << "#inf";
            break;
        }
    }
    return out;
}

struct TermConstant : Term {
    explicit TermConstant(Constant value) : value(value) {}

    void print(std::ostream &out) const override { out << value; }

    Constant value;
};

struct TermInteger : Term {
    explicit TermInteger(int v) : value(v) {}

    void print(std::ostream &out) const override { out << value; }

    int value;
};

struct TermTuple : Term {
    using value_type = std::variant<std::vector<UTerm>, UTerm>;
    explicit TermTuple(std::vector<value_type> args) : args(std::move(args)) {}

    void print(std::ostream &out) const override {
        if (args.size() == 1 && std::holds_alternative<UTerm>(args.front())) {
            std::get<UTerm>(args.front())->print(out);
        } else {
            out << "(";
            bool sem = false;
            for (const auto &tuple : args) {
                if (sem) {
                    out << ";";
                } else {
                    sem = true;
                }
                std::visit(
                    [&](auto &&arg) {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, UTerm>) {
                            arg->print(out);
                        } else if constexpr (std::is_same_v<T, std::vector<UTerm>>) {
                            bool comma = false;
                            for (const auto &term : arg) {
                                if (comma) {
                                    out << ",";
                                } else {
                                    comma = true;
                                }
                                term->print(out);
                            }
                            if (arg.size() == 1) {
                                out << ",";
                            }
                        }
                    },
                    tuple);
            }
            out << ")";
        }
    }

    std::vector<value_type> args;
};

struct TermString : Term {
    explicit TermString(std::string value) : value(std::move(value)) {}

    void print(std::ostream &out) const override { out << value; }

    std::string value;
};

struct TermVariable : Term {
    explicit TermVariable(std::string name) : name(std::move(name)) {}

    void print(std::ostream &out) const override { out << name; }

    std::string name;
};

struct TermAbs : Term {
    explicit TermAbs(std::vector<UTerm> pool) : pool(std::move(pool)) {}

    void print(std::ostream &out) const override {
        out << "|";
        bool comma = false;
        for (const auto &term : pool) {
            if (comma) {
                out << ";";
            } else {
                comma = true;
            }
            term->print(out);
        }
        out << "|";
    }

    std::vector<UTerm> pool;
};

struct TermFunction : Term {
    explicit TermFunction(std::string name, std::vector<std::vector<UTerm>> args, bool external)
        : name(std::move(name)), args{std::move(args)}, external{external} {}

    void print(std::ostream &out) const override {
        if (external) {
            out << "@";
        }
        out << name;
        if (args.size() != 1 || !args.front().empty()) {
            out << "(";
            bool sem = false;
            for (const auto &tuple : args) {
                if (sem) {
                    out << ";";
                } else {
                    sem = true;
                }
                bool comma = false;
                for (const auto &term : tuple) {
                    if (comma) {
                        out << ",";
                    } else {
                        comma = true;
                    }
                    term->print(out);
                }
            }
            out << ")";
        }
    }

    std::string name;
    std::vector<std::vector<UTerm>> args;
    bool external;
};

enum class UnaryOperator {
    negate,
    invert,
};

auto operator<<(std::ostream &out, UnaryOperator op) -> std::ostream & {
    out << (op == UnaryOperator::negate ? "-" : "~");
    return out;
}

struct TermUnary : Term {
    explicit TermUnary(UnaryOperator op, UTerm e) : op(op), rhs(std::move(e)) {}

    void print(std::ostream &out) const override { out << "(" << op << *rhs << ")"; }

    UnaryOperator op;
    UTerm rhs;
};

enum class BinaryOperator {
    dots,
    xor_,
    or_,
    and_,
    plus,
    minus,
    times,
    div,
    mod,
    pow,
};

auto operator<<(std::ostream &out, BinaryOperator op) -> std::ostream & {
    switch (op) {
        case BinaryOperator::dots: {
            out << "^";
            break;
        }
        case BinaryOperator::xor_: {
            out << "^";
            break;
        }
        case BinaryOperator::or_: {
            out << "?";
            break;
        }
        case BinaryOperator::and_: {
            out << "&";
            break;
        }
        case BinaryOperator::plus: {
            out << "+";
            break;
        }
        case BinaryOperator::minus: {
            out << "-";
            break;
        }
        case BinaryOperator::times: {
            out << "*";
            break;
        }
        case BinaryOperator::div: {
            out << "/";
            break;
        }
        case BinaryOperator::mod: {
            out << "\\";
            break;
        }
        case BinaryOperator::pow: {
            out << "**";
            break;
        }
    }
    return out;
}

struct TermBinary : Term {
    explicit TermBinary(UTerm lhs, BinaryOperator op, UTerm rhs) : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    void print(std::ostream &out) const override { out << "(" << *lhs << op << *rhs << ")"; }

    BinaryOperator op;
    UTerm lhs;
    UTerm rhs;
};

namespace grammar {

namespace dsl = lexy::dsl;

using encoding = lexy::utf8_encoding;
using input = StreamInput<encoding>;
using iterator = input::iterator;
using lexeme = lexy::lexeme_for<input>;

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

struct nested_expr : lexy::transparent_production {
    static constexpr auto rule = dsl::recurse<struct expr>;
    static constexpr auto value = lexy::forward<UTerm>;
};

struct tuple {
    // NOTE: the not-followed by is to support trailing commas in tuple terms
    static constexpr auto rule =
        dsl::list(dsl::p<nested_expr>, dsl::sep(dsl::not_followed_by(dsl::comma, dsl::semicolon / LEXY_LIT(")"))));
    static constexpr auto value = lexy::as_list<std::vector<UTerm>>;
};

struct pool {
    static constexpr auto rule = dsl::parenthesized.list(
        dsl::opt(dsl::peek_not(dsl::semicolon / LEXY_LIT(")")) >> dsl::p<tuple>), dsl::sep(dsl::semicolon));
    static constexpr auto value = lexy::collect<std::vector<std::vector<UTerm>>>(lexy::as_list<std::vector<UTerm>>);
};

constexpr auto empty_args_ = [](std::optional<std::vector<std::vector<UTerm>>> value) {
    if (value.has_value()) {
        return std::move(value.value());
    }
    std::vector<std::vector<UTerm>> ret;
    ret.emplace_back();
    return ret;
};

struct function {
    static constexpr auto rule = dsl::p<identifier> >> dsl::opt(dsl::p<pool>);
    static constexpr auto value =
        lexy::bind(lexy::new_<TermFunction, UTerm>, lexy::_1, lexy::_2.map(empty_args_), false);
};

struct external_function {
    static constexpr auto rule = LEXY_LIT("@") >> dsl::p<identifier> + dsl::opt(dsl::p<pool>);
    static constexpr auto value =
        lexy::bind(lexy::new_<TermFunction, UTerm>, lexy::_1, lexy::_2.map(empty_args_), true);
};

struct make_tuple {
    using tuple_type = std::vector<UTerm>;
    using return_type = std::variant<tuple_type, UTerm>;

    [[nodiscard]] static auto make(std::optional<tuple_type> tuple, bool force_tuple) -> return_type {
        if (tuple.has_value()) {
            if (!force_tuple && tuple->size() == 1) {
                return std::move(tuple->front());
            }
            return std::move(tuple.value());
        }
        auto ret = std::vector<UTerm>{};
        ret.emplace_back();
        return ret;
    }
    auto operator()(std::optional<tuple_type> tuple, lexy::nullopt /*unused*/) const -> return_type {
        return make(std::move(tuple), false);
    }
    auto operator()(std::optional<tuple_type> tuple) const -> return_type { return make(std::move(tuple), true); }
};

struct make_pool {
    using return_type = UTerm;
    auto operator()(std::vector<make_tuple::return_type> pool) const -> UTerm {
        if (pool.size() == 1 && std::holds_alternative<UTerm>(pool.front())) {
            return std::move(std::get<UTerm>(pool.front()));
        }
        return std::make_unique<TermTuple>(std::move(pool));
    }
};

struct term_tuple {
    static constexpr auto rule = []() {
        auto opt_tuple = dsl::opt(dsl::peek_not(dsl::semicolon / LEXY_LIT(")") / LEXY_LIT(",")) >> dsl::p<tuple>);
        auto opt_comma = dsl::opt(LEXY_LIT(","));
        return dsl::parenthesized.list(opt_tuple + opt_comma, dsl::sep(dsl::semicolon));
    }();
    static constexpr auto value = lexy::collect<std::vector<make_tuple::return_type>>(make_tuple{}) >> make_pool();
};

struct math_abs {
    static constexpr auto rule =
        dsl::brackets(LEXY_LIT("|"), LEXY_LIT("|")).list(dsl::p<nested_expr>, dsl::sep(dsl::semicolon));
    static constexpr auto value = lexy::as_list<std::vector<UTerm>> >> lexy::new_<TermAbs, UTerm>;
};

struct anonymous_variable {
    static constexpr auto rule =
        dsl::capture(dsl::not_followed_by(LEXY_LIT("_"), dsl::ascii::alpha_digit_underscore / LEXY_LIT("'")));
    static constexpr auto value = lexy::as_string<std::string> | lexy::new_<TermVariable, UTerm>;
};

/*
struct upper {
    static constexpr auto rule = dsl::ascii::upper;
    static constexpr auto value = lexy::forward<void>;
};

struct atom_expr : lexy::scan_production<UTerm> {
    struct expected_operand {
        static constexpr auto name = "expected operand";
    };

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> lexy::scan_result<UTerm> {
        // parse a term in parenthesis
        scan_result res;
        if (scanner.branch(res, dsl::parenthesized(dsl::p<nested_expr>))) {
            return res;
        }
        // parse a number
        lexy::scan_result<int> num_res;
        if (scanner.branch(num_res, dsl::p<number>)) {
            res = std::make_unique<TermInteger>(num_res.value());
            return res;
        }
        // parse a variable
        auto begin = scanner.position();

        while (scanner.branch(LEXY_LIT("_") / LEXY_LIT("'"))) {
        }

        auto p_res = lexy::match<upper>(scanner.remaining_input());
        if (!p_res) {
            scanner.fatal_error("expected ASCII.alpha", scanner.position());
            return lexy::scan_failed;
        }

        bool var = false;
        if (scanner.peek(dsl::ascii::upper)) {
            var = true;
        }
        else if (scanner.peek(dsl::ascii::lower)) {
            var = false;
        }
        else {
            scanner.fatal_error("expected ASCII.alpha", scanner.position());
            return lexy::scan_failed;
        }

        while (scanner.branch(dsl::ascii::alpha_underscore / LEXY_LIT("'"))) {
        }

        if (var) {
            return std::make_unique<TermVariable>(std::string{begin, scanner.position()});
        }
        // TODO: parse the arguments of the function
        return std::make_unique<TermFunction>(std::string{begin, scanner.position()});
    }
};
*/

struct expr : lexy::expression_production {
    struct expected_term {
        static constexpr auto name = "expected term";
    };

    static constexpr auto atom = dsl::p<number> | dsl::p<term_tuple> | dsl::p<variable> | dsl::p<math_abs> |
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

struct statement {
    static constexpr auto whitespace = dsl::ascii::space | dsl::newline;
    static constexpr auto rule = dsl::p<nested_expr> + dsl::lit_c<';'> + dsl::position;
    static constexpr auto value = lexy::construct<std::pair<UTerm, iterator>>;
};

} // namespace grammar

auto parse(std::string str) -> std::string {
    std::istringstream in;
    str.append(";");
    in.str(std::move(str));
    auto input = grammar::input{in};
    auto stm = lexy::parse<grammar::statement>(input, report_error);
    REQUIRE(stm.has_value());
    return stm.value().first->to_string();
}

} // namespace

TEST_CASE("statements") {
    REQUIRE(parse("42") == "42");
    REQUIRE(parse("f") == "f");
    REQUIRE(parse("f(  )+5") == "(f+5)");
    REQUIRE(parse("f(1)") == "f(1)");
    REQUIRE(parse("f ( 1 , 2 ; 4 )") == "f(1,2;4)");
    REQUIRE(parse("1 + f") == "(1+f)");
    REQUIRE(parse("@f(1,2)") == "@f(1,2)");
    REQUIRE(parse("|42|") == "|42|");
    REQUIRE(parse("||42||") == "||42||");
    REQUIRE(parse("f(_,X)") == "f(_,X)");
    REQUIRE(parse("(a)") == "a");
    REQUIRE(parse("(a;a,b;a,b,c)") == "(a;a,b;a,b,c)");
    REQUIRE(parse("(a,;a,b,;a,b,c,)") == "(a,;a,b;a,b,c)");
}

TEST_CASE("term-test-working") {
    std::istringstream in;
    in.str("42  *-\n2-32**3+'_Xa_'-_xA;\n43+'_$;");
    auto input = grammar::input{in};
    auto stm = lexy::parse<grammar::statement>(input, report_error);
    REQUIRE(stm.has_value());
    REQUIRE(stm.value().first->to_string() == "((((42*(-2))-(32**3))+'_Xa_')-_xA)");
    input.discard_before(stm.value().second);
    stm = lexy::parse<grammar::statement>(input, report_error);
    REQUIRE(!stm.has_value());
}
