#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <lexy/action/parse.hpp>
#include <lexy/action/scan.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/grammar.hpp>

#include "util/lexy_report_error.hh"
#include "util/lexy_stream_input.hh"

namespace {

struct Term {
    virtual ~Term() = default;
    [[nodiscard]] virtual auto is_atom() const -> bool { return false; }
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

    [[nodiscard]] auto is_atom() const -> bool override { return !external; }

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

    [[nodiscard]] auto is_atom() const -> bool override { return op == UnaryOperator::negate && rhs->is_atom(); }

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

enum class Sign {
    none,
    once,
    twice,
};

auto operator-(Sign a) {
    switch (a) {
        case Sign::none: {
            return Sign::once;
        }
        case Sign::once: {
            return Sign::twice;
        }
        case Sign::twice: {
            break;
        }
    }
    return Sign::once;
}

auto operator+(Sign a, Sign b) {
    switch (a) {
        case Sign::none: {
            return b;
        }
        case Sign::once: {
            return -b;
        }
        case Sign::twice: {
            break;
        }
    }
    return -(-b);
}

auto operator+=(Sign &a, Sign b) -> auto & {
    a = a + b;
    return a;
}

auto operator<<(std::ostream &out, Sign op) -> std::ostream & {
    switch (op) {
        case Sign::none: {
            break;
        }
        case Sign::once: {
            out << "not ";
            break;
        }
        case Sign::twice: {
            out << "not not ";
            break;
        }
    }
    return out;
}

struct Literal {
    virtual ~Literal() = default;
    virtual void print(std::ostream &out) const = 0;
    virtual void add_sign(Sign sign) = 0;
    [[nodiscard]] auto to_string() const -> std::string {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    friend auto operator<<(std::ostream &out, Literal const &literal) -> std::ostream & {
        literal.print(out);
        return out;
    }
};

using ULiteral = std::unique_ptr<Literal>;

enum class Relation {
    less,
    less_equal,
    greater,
    greater_equal,
    equal,
    inequal,
};

auto operator<<(std::ostream &out, Relation op) -> std::ostream & {
    switch (op) {
        case Relation::less: {
            out << "<";
            break;
        }
        case Relation::less_equal: {
            out << "<=";
            break;
        }
        case Relation::greater: {
            out << ">";
            break;
        }
        case Relation::greater_equal: {
            out << ">=";
            break;
        }
        case Relation::equal: {
            out << "=";
            break;
        }
        case Relation::inequal: {
            out << "!=";
            break;
        }
    }
    return out;
}

struct LiteralRelation : Literal {
    LiteralRelation(UTerm lhs, std::vector<std::pair<Relation, UTerm>> rhs)
        : sign(Sign::none), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    LiteralRelation(Sign sign, UTerm lhs, std::vector<std::pair<Relation, UTerm>> rhs)
        : sign(sign), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    void print(std::ostream &out) const override {
        out << sign << *lhs;
        for (auto &&guard : rhs) {
            out << guard.first << *guard.second;
        }
    }
    void add_sign(Sign s) override { sign += s; }
    Sign sign;
    UTerm lhs;
    std::vector<std::pair<Relation, UTerm>> rhs;
};

struct LiteralBoolean : Literal {
    LiteralBoolean(bool value) : sign(Sign::none), value(value) {}
    LiteralBoolean(Sign sign, bool value) : sign(sign), value(value) {}
    void print(std::ostream &out) const override { out << sign << (value ? "#true" : "#false"); }
    void add_sign(Sign s) override { sign += s; }
    Sign sign;
    bool value;
};

struct LiteralSymbolic : Literal {
    LiteralSymbolic(UTerm term) : sign(Sign::none), term(std::move(term)) {}
    LiteralSymbolic(Sign sign, UTerm term) : sign(sign), term(std::move(term)) {}
    void print(std::ostream &out) const override { out << sign << *term; }
    void add_sign(Sign s) override { sign += s; }
    Sign sign;
    UTerm term;
};

struct HeadLiteral {
    virtual ~HeadLiteral() = default;
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    friend auto operator<<(std::ostream &out, HeadLiteral const &literal) -> std::ostream & {
        literal.print(out);
        return out;
    }
};

using UHeadLiteral = std::unique_ptr<HeadLiteral>;

struct Disjunction : HeadLiteral {
    using Element = std::pair<ULiteral, std::vector<ULiteral>>;
    using Elements = std::vector<Element>;
    Disjunction(Elements elems) : elems{std::move(elems)} {}
    void print(std::ostream &out) const override {
        bool sem = false;
        for (const auto &elem : elems) {
            if (sem) {
                out << ";";
            }
            sem = true;
            out << *elem.first;
            if (!elem.second.empty()) {
                out << ":";
                bool comma = false;
                for (const auto &lit : elem.second) {
                    if (comma) {
                        out << ",";
                    }
                    comma = true;
                    out << *lit;
                }
            }
        }
    }

    Elements elems;
};

struct HeadTheoryAtom : HeadLiteral {
    void print(std::ostream &out) const override { out << "&p{...}"; }
};

struct HeadAggregate : HeadLiteral {
    void print(std::ostream &out) const override { out << "#count{...}"; }
};

struct HeadSetAggregate : HeadLiteral {
    void print(std::ostream &out) const override { out << "{...}"; }
};

namespace grammar {

namespace dsl = lexy::dsl;

using encoding = lexy::utf8_encoding;
using input = StreamInput<encoding>;
using iterator = input::iterator;
using lexeme = lexy::lexeme_for<input>;

struct control {
    static constexpr auto whitespace = dsl::ascii::space | dsl::newline;
};

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
    static constexpr auto rule = []() {
        auto skip_ws = dsl::while_(control::whitespace);
        auto peek = dsl::peek_not(dsl::semicolon / LEXY_LIT(")"));
        auto item = dsl::p<nested_expr>;
        auto sep = dsl::token(dsl::comma + skip_ws + peek);
        return dsl::list(item, dsl::sep(sep));
    }();
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

struct relation {
    static constexpr auto entities = lexy::symbol_table<Relation> //
                                         .map<LEXY_SYMBOL("<=")>(Relation::less_equal)
                                         .map<LEXY_SYMBOL("<")>(Relation::less)
                                         .map<LEXY_SYMBOL(">=")>(Relation::greater_equal)
                                         .map<LEXY_SYMBOL(">")>(Relation::greater)
                                         .map<LEXY_SYMBOL("!=")>(Relation::inequal)
                                         .map<LEXY_SYMBOL("=")>(Relation::equal);

    static constexpr auto rule = dsl::symbol<entities>;
    static constexpr auto value = lexy::forward<Relation>;
};

struct kw_not {
    static constexpr auto rule = [] {
        auto head = dsl::ascii::lower;
        auto tail = dsl::ascii::alpha_digit_underscore / LEXY_LIT("'");
        auto id = dsl::identifier(head, tail);

        return LEXY_KEYWORD("not", id);
    }();
};

struct atom : lexy::scan_production<ULiteral> {
    struct guard {
        static constexpr auto rule = dsl::p<relation> >> dsl::p<nested_expr>;
        static constexpr auto value = lexy::construct<std::pair<Relation, UTerm>>;
    };

    struct guards {
        static constexpr auto rule = dsl::list(dsl::p<guard>);
        static constexpr auto value = lexy::as_list<std::vector<std::pair<Relation, UTerm>>>;
    };

    struct bool_atom {
        static constexpr auto bool_symbols = lexy::symbol_table<bool> //
                                                 .map<LEXY_SYMBOL("#true")>(true)
                                                 .map<LEXY_SYMBOL("#false")>(false);
        static constexpr auto rule = dsl::symbol<bool_symbols>;
        static constexpr auto value = lexy::new_<LiteralBoolean, ULiteral>;
    };

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        scan_result res_lit;
        if (scanner.template branch<bool_atom>(res_lit)) {
            return res_lit;
        }
        auto res_term = scanner.template parse<nested_expr>();
        if (!res_term.has_value()) {
            return lexy::scan_failed;
        }
        lexy::scan_result<std::vector<std::pair<Relation, UTerm>>> res_guards;
        if (scanner.template branch<guards>(res_guards)) {
            return std::make_unique<LiteralRelation>(std::move(res_term).value(), std::move(res_guards).value());
        }
        // Note: we might have overparsed and have to remedy the situation.
        if (!res_term.value()->is_atom()) {
            scanner.error("relation expected", scanner.position());
            return lexy::scan_failed;
        }
        return std::make_unique<LiteralSymbolic>(std::move(res_term).value());
    }
};

struct literal {
    static constexpr auto rule = dsl::opt(kw_not::rule) + dsl::opt(kw_not::rule) + dsl::p<atom>;
    static constexpr auto value =
        lexy::callback<ULiteral>([](lexy::nullopt, lexy::nullopt, ULiteral lit) { return std::move(lit); },
                                 [](lexy::nullopt, ULiteral lit) {
                                     lit->add_sign(Sign::once);
                                     return std::move(lit);
                                 },
                                 [](ULiteral lit) {
                                     lit->add_sign(Sign::twice);
                                     return std::move(lit);
                                 });
};

enum class AggregateFunction {
    count,
    sum,
    sump,
    min,
    max,
};

struct head_aggregate : lexy::scan_production<UHeadLiteral> {
    struct theory_atom {
        static constexpr auto rule = LEXY_LIT("&") >> dsl::p<identifier> + LEXY_LIT("{") + LEXY_LIT("}");
        // TODO: proper construction
        static constexpr auto value =
            lexy::callback<UHeadLiteral>([](auto &&...) { return std::make_unique<HeadTheoryAtom>(); });
    };
    struct aggregate {
        static constexpr auto rule = LEXY_LIT("#count") >> LEXY_LIT("{") + LEXY_LIT("}");
        // TODO: proper construction
        static constexpr auto value =
            lexy::callback<UHeadLiteral>([](auto &&...) { return std::make_unique<HeadAggregate>(); });
    };
    struct set_aggregate {
        static constexpr auto rule = LEXY_LIT("{") >> LEXY_LIT("}");
        // TODO: proper construction
        static constexpr auto value =
            lexy::callback<UHeadLiteral>([](auto &&...) { return std::make_unique<HeadSetAggregate>(); });
    };

    struct condition {
        static constexpr auto rule = dsl::opt(LEXY_LIT(":") >> dsl::list(dsl::p<literal>, dsl::sep(LEXY_LIT(","))));
        static constexpr auto value = lexy::as_list<std::vector<ULiteral>>;
    };

    struct element {
        static constexpr auto rule = dsl::p<literal> + dsl::p<condition>;
        static constexpr auto value = lexy::construct<std::pair<ULiteral, std::vector<ULiteral>>>;
    };

    static constexpr auto sep = LEXY_LIT(",") / LEXY_LIT(";") / LEXY_LIT("|");

    struct elements {
        static constexpr auto rule = dsl::opt(dsl::list(sep >> dsl::p<element>));
        static constexpr auto value = lexy::as_list<std::vector<std::pair<ULiteral, std::vector<ULiteral>>>>;
    };

    struct disjunction {
        static constexpr auto rule = dsl::list(dsl::p<element>, dsl::sep(sep));
        static constexpr auto value = lexy::as_list<Disjunction::Elements> >> lexy::new_<Disjunction, UHeadLiteral>;
    };

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto branch_aggregate = [&scanner](scan_result &res_lit, lexy::scan_result<UTerm> &lhs,
                                           Relation rel = Relation::less_equal) -> bool {
            if (scanner.template branch<aggregate>(res_lit)) {
                // TODO: set left guard
                return true;
            }
            if (scanner.template branch<set_aggregate>(res_lit)) {
                // TODO: set left guard
                return true;
            }
            return false;
        };
        if (scanner.peek(dsl::p<kw_not>)) {
            return scanner.template parse<disjunction>();
        }
        scan_result res_lit;
        if (scanner.template branch<theory_atom>(res_lit)) {
            return res_lit;
        }
        lexy::scan_result<UTerm> res_lhs;
        if (branch_aggregate(res_lit, res_lhs)) {
            return res_lit;
        }
        res_lhs = scanner.template parse<nested_expr>();
        if (!res_lhs.has_value()) {
            return lexy::scan_failed;
        }
        if (branch_aggregate(res_lit, res_lhs)) {
            return res_lit;
        }
        Disjunction::Element elem;
        Disjunction::Elements elems;
        lexy::scan_result<Relation> res_rel;
        if (scanner.template branch<relation>(res_rel)) {
            if (branch_aggregate(res_lit, res_lhs, res_rel.value())) {
                return res_lit;
            }
            // Note: a bit unwieldy because the relation has already been consumed
            auto res_rhs = scanner.template parse<nested_expr>();
            lexy::scan_result<std::vector<std::pair<Relation, UTerm>>> res_guards;
            scanner.template branch<atom::guards>(res_guards);
            auto res_cond = scanner.template parse<condition>();
            auto res_elems = scanner.template parse<elements>();
            if (!scanner) {
                return lexy::scan_failed;
            }
            std::vector<std::pair<Relation, UTerm>> guards;
            if (res_guards.has_value()) {
                guards = std::move(res_guards).value();
            }
            guards.insert(guards.begin(), std::make_pair(res_rel.value(), std::move(res_rhs).value()));
            elem.first = std::make_unique<LiteralRelation>(std::move(res_lhs).value(), std::move(guards));
            elem.second = std::move(res_cond).value();
            elems = std::move(res_elems).value();
        } else if (res_lhs.value()->is_atom()) {
            auto res_cond = scanner.template parse<condition>();
            auto res_elems = scanner.template parse<elements>();
            if (!scanner) {
                return lexy::scan_failed;
            }
            elem.first = std::make_unique<LiteralSymbolic>(std::move(res_lhs).value());
            elem.second = std::move(res_cond).value();
            elems = std::move(res_elems).value();
        } else {
            scanner.error("relation expected", scanner.position());
            return lexy::scan_failed;
        }
        elems.insert(elems.begin(), std::move(elem));
        return std::make_unique<Disjunction>(std::move(elems));
    }
};

struct statement : control {
    static constexpr auto rule = dsl::p<literal> + dsl::lit_c<';'>;
    static constexpr auto value = lexy::forward<ULiteral>;
};

} // namespace grammar

namespace test {

namespace dsl = lexy::dsl;

template <class P> struct parse_root : grammar::control {
    static constexpr auto rule = dsl::p<P> + dsl::eof;
    static constexpr auto value = lexy::forward<typename decltype(P::value)::return_type>;
};

template <class P> struct match_root : grammar::control {
    static constexpr auto rule = dsl::p<P> + dsl::eof;
};

using term = parse_root<grammar::nested_expr>;
using literal = parse_root<grammar::literal>;
using head_aggregate = parse_root<grammar::head_aggregate>;

} // namespace test

template <typename Control> auto parse(std::string str) -> std::string {
    std::istringstream in;
    in.str(std::move(str));
    auto input = grammar::input{in};
    auto stm = lexy::parse<Control>(input, report_error);
    return stm.has_value() ? stm.value()->to_string() : "<failed>";
}

template <typename Control> auto match(std::string str) {
    std::istringstream in;
    in.str(std::move(str));
    auto input = grammar::input{in};
    auto res = lexy::validate<Control>(input, report_error);
    return res.is_success();
}

} // namespace

TEST_CASE("terms") {
    REQUIRE(parse<test::term>("42") == "42");
    REQUIRE(parse<test::term>("f") == "f");
    REQUIRE(parse<test::term>("f(  )+5") == "(f+5)");
    REQUIRE(parse<test::term>("f(1)") == "f(1)");
    REQUIRE(parse<test::term>("f ( 1 , 2 ; 4 )") == "f(1,2;4)");
    REQUIRE(parse<test::term>("1 + f") == "(1+f)");
    REQUIRE(parse<test::term>("@f(1,2)") == "@f(1,2)");
    REQUIRE(parse<test::term>("|42|") == "|42|");
    REQUIRE(parse<test::term>("||42||") == "||42||");
    REQUIRE(parse<test::term>("f(_,X)") == "f(_,X)");
    REQUIRE(parse<test::term>("(a)") == "a");
    REQUIRE(parse<test::term>("(a;a,b;a,b,c)") == "(a;a,b;a,b,c)");
    REQUIRE(parse<test::term>("(a, ; a,b,;a,b,c, )") == "(a,;a,b;a,b,c)");
}

TEST_CASE("literals") {
    REQUIRE(parse<test::literal>("#true") == "#true");
    REQUIRE(parse<test::literal>("#false") == "#false");
    REQUIRE(parse<test::literal>("1 < 2") == "1<2");
    REQUIRE(parse<test::literal>("-f+1 < 2") == "((-f)+1)<2");
    REQUIRE(parse<test::literal>("p(X)") == "p(X)");
    // TODO: get rid of parenthesis
    REQUIRE(parse<test::literal>("-p(X)") == "(-p(X))");
    REQUIRE(parse<test::literal>("not p") == "not p");
    REQUIRE(parse<test::literal>("not not p") == "not not p");
}

TEST_CASE("head literals") {
    // theory_atom | aggregate | set_aggregate | disjunction | '-'? ...
    REQUIRE(parse<test::head_aggregate>("&x{}") == "&p{...}");
    REQUIRE(parse<test::head_aggregate>("#count{}") == "#count{...}");
    REQUIRE(parse<test::head_aggregate>("{}") == "{...}");
    REQUIRE(parse<test::head_aggregate>("not a") == "not a");
    REQUIRE(parse<test::head_aggregate>("-a") == "(-a)");
    // identifier pool? relation ...
    REQUIRE(parse<test::head_aggregate>("a<{}") == "{...}");
    REQUIRE(parse<test::head_aggregate>("a<#count{}") == "#count{...}");
    REQUIRE(parse<test::head_aggregate>("a<b<c") == "a<b<c");
    REQUIRE(parse<test::head_aggregate>("a<a:a") == "a<a:a");
    REQUIRE(parse<test::head_aggregate>("a<a:a;a") == "a<a:a;a");
    REQUIRE(parse<test::head_aggregate>("a<a,a") == "a<a;a");
    // identifier pool? ...
    REQUIRE(parse<test::head_aggregate>("a{}") == "{...}");
    REQUIRE(parse<test::head_aggregate>("a#count{}") == "#count{...}");
    REQUIRE(parse<test::head_aggregate>("-a(X)") == "(-a(X))");
    REQUIRE(parse<test::head_aggregate>("a:a") == "a:a");
    REQUIRE(parse<test::head_aggregate>("a:a;a") == "a:a;a");
    REQUIRE(parse<test::head_aggregate>("a,a") == "a;a");
    // term relation ...
    REQUIRE(parse<test::head_aggregate>("a+1{}") == "{...}");
    REQUIRE(parse<test::head_aggregate>("a+1#count{}") == "#count{...}");
    REQUIRE(parse<test::head_aggregate>("a+1<{}") == "{...}");
    REQUIRE(parse<test::head_aggregate>("a+1<#count{}") == "#count{...}");
    REQUIRE(parse<test::head_aggregate>("a+1<b<c") == "(a+1)<b<c");
    REQUIRE(parse<test::head_aggregate>("a+1<a:a") == "(a+1)<a:a");
    REQUIRE(parse<test::head_aggregate>("a+1<a:a;a") == "(a+1)<a:a;a");
    REQUIRE(parse<test::head_aggregate>("a+1<a,a") == "(a+1)<a;a");
    REQUIRE(parse<test::head_aggregate>("a+1<>a,a") == "<failed>");
    // disjunctions
    REQUIRE(parse<test::head_aggregate>("a,b") == "a;b");
    REQUIRE(parse<test::head_aggregate>("a;b") == "a;b");
    REQUIRE(parse<test::head_aggregate>("a|b") == "a;b");
}

TEST_CASE("scan") {
    std::istringstream in;
    in.str("42  *-\n2-32**3+'_Xa_'-_xA<5;\n43+'_$;");
    auto input = grammar::input{in};
    auto scanner = lexy::scan<grammar::control>(input, report_error);
    auto stm = scanner.parse<grammar::statement>();
    REQUIRE(stm.has_value());
    REQUIRE(stm.value()->to_string() == "((((42*(-2))-(32**3))+'_Xa_')-_xA)<5");
    input.discard_before(scanner.position());
    stm = scanner.parse<grammar::statement>();
    REQUIRE(!stm.has_value());
}
