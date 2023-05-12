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

template <class T> struct p_elem {
    p_elem(T const &elem) : elem(elem) {}
    friend auto operator<<(std::ostream &out, p_elem const &elem) -> std::ostream & {
        out << elem.elem;
        return out;
    }

    T const &elem;
};

template <class T> struct p_elem<std::unique_ptr<T>> {
    p_elem(std::unique_ptr<T> const &elem) : elem(elem) {}
    friend auto operator<<(std::ostream &out, p_elem const &elem) -> std::ostream & {
        out << *elem.elem;
        return out;
    }

    std::unique_ptr<T> const &elem;
};

template <class T> struct p_range {
    p_range(T const &rng, char const *sep = ",") : rng(rng), sep(sep) {}
    friend auto operator<<(std::ostream &out, p_range const &rng) -> std::ostream & {
        bool comma = false;
        for (auto &elem : rng.rng) {
            if (comma) {
                out << rng.sep;
            }
            comma = true;
            out << p_elem{elem};
        }
        return out;
    }
    T const &rng;
    char const *sep;
};

template <class T, class F> struct p_range_with {
    p_range_with(T const &rng, char const *sep, F &&f) : rng(rng), f(std::forward<F>(f)), sep(sep) {}
    p_range_with(T const &rng, F &&f) : p_range_with(rng, ",", std::forward<F>(f)) {}
    friend auto operator<<(std::ostream &out, p_range_with const &rng) -> std::ostream & {
        bool comma = false;
        for (auto &elem : rng.rng) {
            if (comma) {
                out << rng.sep;
            }
            comma = true;
            rng.f(out, elem);
        }
        return out;
    }
    T const &rng;
    F f;
    char const *sep;
};

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
using UTermVec = std::vector<UTerm>;
using UTermVecVec = std::vector<UTermVec>;

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
    using Element = std::variant<UTermVec, UTerm>;
    using ElementVec = std::vector<Element>;
    explicit TermTuple(ElementVec args) : args(std::move(args)) {}

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
                        } else if constexpr (std::is_same_v<T, UTermVec>) {
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

    ElementVec args;
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
    explicit TermAbs(UTermVec pool) : pool(std::move(pool)) {}

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

    UTermVec pool;
};

struct TermFunction : Term {
    explicit TermFunction(std::string name, UTermVecVec args, bool external)
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
    UTermVecVec args;
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
using ULiteralVec = std::vector<ULiteral>;

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

using Guard = std::pair<Relation, UTerm>;
using GuardVec = std::vector<Guard>;

struct LiteralRelation : Literal {
    LiteralRelation(UTerm lhs, GuardVec rhs) : sign(Sign::none), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    LiteralRelation(Sign sign, UTerm lhs, GuardVec rhs) : sign(sign), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    void print(std::ostream &out) const override {
        out << sign << *lhs;
        for (auto &&guard : rhs) {
            out << guard.first << *guard.second;
        }
    }
    void add_sign(Sign s) override { sign += s; }
    Sign sign;
    UTerm lhs;
    GuardVec rhs;
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
    [[nodiscard]] virtual auto print_empty() const -> bool { return false; }
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
    using Element = std::pair<ULiteral, ULiteralVec>;
    using ElementVec = std::vector<Element>;
    Disjunction(ElementVec elems) : elems{std::move(elems)} {}
    [[nodiscard]] auto print_empty() const -> bool override { return elems.empty(); }
    void print(std::ostream &out) const override {
        out << p_range_with(elems, ";", [](std::ostream &out, auto const &elem) {
            out << *elem.first;
            if (!elem.second.empty()) {
                out << ":" << p_range(elem.second);
            }
        });
    }

    ElementVec elems;
};

struct HeadTheoryAtom : HeadLiteral {
    void print(std::ostream &out) const override { out << "&p{...}"; }
};

enum class AggregateFunction {
    count,
    sum,
    sump,
    min,
    max,
};

auto operator<<(std::ostream &out, AggregateFunction fun) -> std::ostream & {
    switch (fun) {
        case AggregateFunction::count: {
            out << "#count";
            break;
        }
        case AggregateFunction::sum: {
            out << "#sum";
            break;
        }
        case AggregateFunction::sump: {
            out << "#sum+";
            break;
        }
        case AggregateFunction::min: {
            out << "#min";
            break;
        }
        case AggregateFunction::max: {
            out << "#max";
            break;
        }
    }
    return out;
}

struct HeadAggregate : HeadLiteral {
    using Element = std::tuple<UTermVec, ULiteral, ULiteralVec>;
    using ElementVec = std::vector<Element>;
    HeadAggregate(AggregateFunction fun, ElementVec elems) : fun(fun), elements(std::move(elems)) {}
    HeadAggregate(AggregateFunction fun, ElementVec elems, Relation rel, UTerm rhs)
        : fun(fun), elements(std::move(elems)), right_guard(std::make_pair(rel, std::move(rhs))) {}
    void set_left_guard(UTerm lhs, Relation rel) { left_guard = std::make_pair(std::move(lhs), rel); }
    void print(std::ostream &out) const override {
        if (left_guard) {
            out << *left_guard->first << left_guard->second;
        }
        out << fun << "{" << p_range_with(elements, ";", [](std::ostream &out, auto const &elem) {
            out << p_range{std::get<0>(elem), ","} << ":" << *std::get<1>(elem);
            if (!std::get<2>(elem).empty()) {
                out << ":" << p_range{std::get<2>(elem)};
            }
        }) << "}";
        if (right_guard) {
            out << right_guard->first << *right_guard->second;
        }
    }
    AggregateFunction fun;
    ElementVec elements;
    std::optional<std::pair<UTerm, Relation>> left_guard;
    std::optional<std::pair<Relation, UTerm>> right_guard;
};

using UHeadAggregate = std::unique_ptr<HeadAggregate>;

struct HeadSetAggregate : HeadLiteral {
    using Element = std::pair<ULiteral, ULiteralVec>;
    using ElementVec = std::vector<Element>;
    HeadSetAggregate(ElementVec elements) : elements{std::move(elements)} {}
    HeadSetAggregate(ElementVec elements, Relation rel, UTerm rhs)
        : elements{std::move(elements)}, right_guard(std::make_pair(rel, std::move(rhs))) {}
    void set_left_guard(UTerm lhs, Relation rel) { left_guard = std::make_pair(std::move(lhs), rel); }
    void print(std::ostream &out) const override {
        if (left_guard) {
            out << *left_guard->first << left_guard->second;
        }
        out << "{" << p_range_with(elements, ";", [](std::ostream &out, auto const &elem) {
            out << *std::get<0>(elem);
            if (!std::get<1>(elem).empty()) {
                out << ":" << p_range{std::get<1>(elem)};
            }
        }) << "}";
        if (right_guard) {
            out << right_guard->first << *right_guard->second;
        }
    }
    ElementVec elements;
    std::optional<std::pair<UTerm, Relation>> left_guard;
    std::optional<std::pair<Relation, UTerm>> right_guard;
};

using UHeadSetAggregate = std::unique_ptr<HeadSetAggregate>;

struct BodyLiteral {
    virtual ~BodyLiteral() = default;
    virtual void add_sign(Sign sign) = 0;
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    friend auto operator<<(std::ostream &out, BodyLiteral const &literal) -> std::ostream & {
        literal.print(out);
        return out;
    }
};

using UBodyLiteral = std::unique_ptr<BodyLiteral>;
using UBodyLiteralVec = std::vector<UBodyLiteral>;

struct ConditionalLiteral : BodyLiteral {
    ConditionalLiteral(ULiteral literal, ULiteralVec condition)
        : literal{std::move(literal)}, condition{std::move(condition)} {}
    void add_sign(Sign s) override { literal->add_sign(s); }
    void print(std::ostream &out) const override {
        out << *literal;
        if (!condition.empty()) {
            out << ":" << p_range(condition);
        }
    }

    ULiteral literal;
    ULiteralVec condition;
};

struct BodyAggregate : BodyLiteral {
    using Element = std::tuple<UTermVec, ULiteralVec>;
    using ElementVec = std::vector<Element>;
    BodyAggregate(AggregateFunction fun, ElementVec elems) : fun(fun), elements(std::move(elems)) {}
    BodyAggregate(AggregateFunction fun, ElementVec elems, Relation rel, UTerm rhs)
        : fun(fun), elements(std::move(elems)), right_guard(std::make_pair(rel, std::move(rhs))) {}
    void add_sign(Sign s) override { sign += s; }
    void set_left_guard(UTerm lhs, Relation rel) { left_guard = std::make_pair(std::move(lhs), rel); }
    void print(std::ostream &out) const override {
        out << sign;
        if (left_guard) {
            out << *left_guard->first << left_guard->second;
        }
        out << fun << "{" << p_range_with(elements, ";", [](std::ostream &out, auto const &elem) {
            out << p_range{std::get<0>(elem), ","};
            if (!std::get<1>(elem).empty()) {
                out << ":" << p_range{std::get<1>(elem)};
            }
        }) << "}";
        if (right_guard) {
            out << right_guard->first << *right_guard->second;
        }
    }
    Sign sign = Sign::none;
    AggregateFunction fun;
    ElementVec elements;
    std::optional<std::pair<UTerm, Relation>> left_guard;
    std::optional<std::pair<Relation, UTerm>> right_guard;
};

using UBodyAggregate = std::unique_ptr<BodyAggregate>;

struct BodySetAggregate : BodyLiteral {
    using Element = std::pair<ULiteral, ULiteralVec>;
    using ElementVec = std::vector<Element>;
    BodySetAggregate(ElementVec elements) : elements{std::move(elements)} {}
    BodySetAggregate(ElementVec elements, Relation rel, UTerm rhs)
        : elements{std::move(elements)}, right_guard(std::make_pair(rel, std::move(rhs))) {}
    void add_sign(Sign s) override { sign += s; }
    void set_left_guard(UTerm lhs, Relation rel) { left_guard = std::make_pair(std::move(lhs), rel); }
    void print(std::ostream &out) const override {
        out << sign;
        if (left_guard) {
            out << *left_guard->first << left_guard->second;
        }
        out << "{" << p_range_with(elements, ";", [](std::ostream &out, auto const &elem) {
            out << *std::get<0>(elem);
            if (!std::get<1>(elem).empty()) {
                out << ":" << p_range{std::get<1>(elem)};
            }
        }) << "}";
        if (right_guard) {
            out << right_guard->first << *right_guard->second;
        }
    }
    Sign sign = Sign::none;
    ElementVec elements;
    std::optional<std::pair<UTerm, Relation>> left_guard;
    std::optional<std::pair<Relation, UTerm>> right_guard;
};

using UBodySetAggregate = std::unique_ptr<BodySetAggregate>;

struct BodyTheoryAtom : BodyLiteral {
    void add_sign(Sign s) override { sign += s; }
    void print(std::ostream &out) const override { out << sign << "&p{...}"; }
    Sign sign = Sign::none;
};

struct Statement {
    virtual ~Statement() = default;
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    friend auto operator<<(std::ostream &out, Statement const &stm) -> std::ostream & {
        stm.print(out);
        return out;
    }
};

using UStatement = std::unique_ptr<Statement>;

struct Rule : Statement {
    Rule(UHeadLiteral head, UBodyLiteralVec body) : head{std::move(head)}, body{std::move(body)} {}
    void print(std::ostream &out) const override {
        out << *head;
        if (head->print_empty() || !body.empty()) {
            out << ":-" << p_range(body, ";");
        }
        out << ".";
    }
    UHeadLiteral head;
    UBodyLiteralVec body;
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
    static constexpr auto value = lexy::as_list<UTermVec>;
};

struct pool {
    static constexpr auto rule = dsl::parenthesized.list(
        dsl::opt(dsl::peek_not(dsl::semicolon / LEXY_LIT(")")) >> dsl::p<tuple>), dsl::sep(dsl::semicolon));
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
    using return_type = std::variant<UTermVec, UTerm>;

    [[nodiscard]] static auto make(std::optional<UTermVec> tuple, bool force_tuple) -> return_type {
        if (tuple.has_value()) {
            if (!force_tuple && tuple->size() == 1) {
                return std::move(tuple->front());
            }
            return std::move(tuple.value());
        }
        auto ret = UTermVec{};
        ret.emplace_back();
        return ret;
    }
    auto operator()(std::optional<UTermVec> tuple, lexy::nullopt /*unused*/) const -> return_type {
        return make(std::move(tuple), false);
    }
    auto operator()(std::optional<UTermVec> tuple) const -> return_type { return make(std::move(tuple), true); }
};

struct make_pool {
    using return_type = UTerm;
    auto operator()(TermTuple::ElementVec pool) const -> UTerm {
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
    static constexpr auto value = lexy::collect<TermTuple::ElementVec>(make_tuple{}) >> make_pool();
};

struct math_abs {
    static constexpr auto rule =
        dsl::brackets(LEXY_LIT("|"), LEXY_LIT("|")).list(dsl::p<nested_expr>, dsl::sep(dsl::semicolon));
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

struct atom {
    using scan_result = lexy::scan_result<UTerm>;

    struct expected_relation {
        static constexpr auto name = "expected relation";
    };

    struct guard {
        static constexpr auto rule = dsl::p<relation> >> dsl::p<nested_expr>;
        static constexpr auto value = lexy::construct<std::pair<Relation, UTerm>>;
    };

    struct guards {
        static constexpr auto rule = dsl::list(dsl::p<guard>);
        static constexpr auto value = lexy::as_list<GuardVec>;
    };

    struct bool_atom {
        static constexpr auto bool_symbols = lexy::symbol_table<bool> //
                                                 .map<LEXY_SYMBOL("#true")>(true)
                                                 .map<LEXY_SYMBOL("#false")>(false);
        static constexpr auto rule = dsl::symbol<bool_symbols>;
        static constexpr auto value = lexy::new_<LiteralBoolean, ULiteral>;
    };

    static constexpr auto is_atom = dsl::context_flag<atom>;

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto res_term = scanner.template parse<nested_expr>();
        if (res_term.has_value() && res_term.value()->is_atom()) {
            scanner.parse(is_atom.set());
        }
        return res_term;
    }

    static constexpr auto rule = dsl::p<bool_atom> |
                                 dsl::else_ >> is_atom.create() + dsl::scan +
                                                   (dsl::p<guards> | is_atom.is_set() | dsl::error<expected_relation>);
    static constexpr auto value = lexy::callback<ULiteral>(
        lexy::forward<ULiteral>, lexy::new_<LiteralSymbolic, ULiteral>, lexy::new_<LiteralRelation, ULiteral>);
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

struct aggregate_function {
    static constexpr auto symbols = lexy::symbol_table<AggregateFunction> //
                                        .map<LEXY_SYMBOL("#count")>(AggregateFunction::count)
                                        .map<LEXY_SYMBOL("#sum")>(AggregateFunction::sum)
                                        .map<LEXY_SYMBOL("#sum+")>(AggregateFunction::sump)
                                        .map<LEXY_SYMBOL("#min")>(AggregateFunction::min)
                                        .map<LEXY_SYMBOL("#max")>(AggregateFunction::max);
    static constexpr auto rule = dsl::symbol<symbols>;
    static constexpr auto value = lexy::forward<AggregateFunction>;
};

struct head_literal {
    using scan_result = lexy::scan_result<UTerm>;

    struct theory_atom {
        // TODO: proper construction
        static constexpr auto rule = LEXY_LIT("&") >> dsl::p<identifier> + LEXY_LIT("{") + LEXY_LIT("}");
        static constexpr auto value =
            lexy::callback<UHeadLiteral>([](auto &&...) { return std::make_unique<HeadTheoryAtom>(); });
    };

    static constexpr auto right_guard = dsl::peek(LEXY_LIT(":") / LEXY_LIT(".")) |
                                        dsl::else_ >> dsl::if_(dsl::p<relation>) + dsl::p<nested_expr>;

    struct condition {
        static constexpr auto rule = dsl::opt(dsl::not_followed_by(LEXY_LIT(":"), LEXY_LIT("-")) >>
                                              dsl::list(dsl::p<literal>, dsl::sep(LEXY_LIT(","))));
        static constexpr auto value = lexy::as_list<ULiteralVec>;
    };

    struct conditional_literal {
        static constexpr auto rule = dsl::p<literal> + dsl::p<condition>;
        static constexpr auto value = lexy::construct<std::pair<ULiteral, ULiteralVec>>;
    };

    struct aggregate_element {
        // Note: gringo also accepts
        //   HeadElem ::= Tuple? ':' Literal (':' Condition?)?
        // It is probably not worth the effort to support an empty condition
        // after a colon (but possible with a lookahead of [;}]).
        static constexpr auto rule = dsl::opt(dsl::peek_not(LEXY_LIT(":")) >> dsl::p<tuple>) + LEXY_LIT(":") +
                                     dsl::p<literal> + dsl::p<condition>;
        static constexpr auto value = lexy::callback<HeadAggregate::Element>(
            [](std::optional<UTermVec> tuple, ULiteral lit, std::optional<ULiteralVec> cond) {
                auto ret = HeadAggregate::Element{UTermVec{}, std::move(lit), ULiteralVec{}};
                if (tuple) {
                    std::get<0>(ret) = std::move(tuple).value();
                }
                if (cond) {
                    std::get<2>(ret) = std::move(cond).value();
                }
                return ret;
            });
    };

    struct aggregate_elements {
        static constexpr auto rule =
            dsl::opt(dsl::peek_not(LEXY_LIT("}")) >> dsl::list(dsl::p<aggregate_element>, dsl::sep(LEXY_LIT(";"))));
        static constexpr auto value = lexy::as_list<HeadAggregate::ElementVec>;
    };

    struct aggregate {
        static constexpr auto rule = dsl::p<aggregate_function> >>
                                     LEXY_LIT("{") + dsl::p<aggregate_elements> + LEXY_LIT("}") + right_guard;
        static constexpr auto value = lexy::callback<UHeadAggregate>(
            lexy::new_<HeadAggregate, UHeadAggregate>,
            [](AggregateFunction fun, HeadAggregate::ElementVec elems, UTerm rhs) {
                return std::make_unique<HeadAggregate>(fun, std::move(elems), Relation::less_equal, std::move(rhs));
            });
    };

    struct set_aggregate_elements {
        static constexpr auto rule =
            dsl::opt(dsl::peek_not(LEXY_LIT("}")) >> dsl::list(dsl::p<conditional_literal>, dsl::sep(LEXY_LIT(";"))));
        static constexpr auto value = lexy::as_list<HeadSetAggregate::ElementVec>;
    };

    struct set_aggregate {
        static constexpr auto rule = LEXY_LIT("{") >> dsl::p<set_aggregate_elements> >> LEXY_LIT("}") + right_guard;
        static constexpr auto value = lexy::callback<UHeadSetAggregate>(
            lexy::new_<HeadSetAggregate, UHeadSetAggregate>, [](HeadSetAggregate::ElementVec elems, UTerm rhs) {
                return std::make_unique<HeadSetAggregate>(std::move(elems), Relation::less_equal, std::move(rhs));
            });
    };

    static constexpr auto sep = LEXY_LIT(",") / LEXY_LIT(";") / LEXY_LIT("|");

    struct conditional_literals {
        static constexpr auto rule = dsl::opt(dsl::list(sep >> dsl::p<conditional_literal>));
        static constexpr auto value = lexy::as_list<Disjunction::ElementVec>;
    };

    struct disjunction {
        static constexpr auto rule = dsl::list(dsl::p<conditional_literal>, dsl::sep(sep));
        static constexpr auto value = lexy::as_list<Disjunction::ElementVec> >> lexy::new_<Disjunction, UHeadLiteral>;
    };

    static constexpr auto is_atom = dsl::context_flag<head_literal>;

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner, auto &&...args) -> scan_result {
        auto res_term = scanner.template parse<nested_expr>();
        if (res_term.has_value() && res_term.value()->is_atom()) {
            scanner.parse(is_atom.set());
        }
        return res_term;
    }

    struct rel_aggr_expected {
        static constexpr auto name = "relation or aggregate expected";
    };

    static constexpr auto with_rel =                //
        dsl::p<aggregate> | dsl::p<set_aggregate> | //
        dsl::else_ >>
            dsl::p<nested_expr> + dsl::opt(dsl::p<atom::guards>) + dsl::p<condition> + dsl::p<conditional_literals>;

    static constexpr auto with_term =                                              //
        dsl::p<relation> >> with_rel | dsl::p<aggregate> | dsl::p<set_aggregate> | //
        is_atom.is_set() >> dsl::p<condition> + dsl::p<conditional_literals> |     //
        dsl::else_ >> dsl::error<rel_aggr_expected>;

    static constexpr auto rule =                                          //
        dsl::peek(dsl::p<kw_not>) >> dsl::p<disjunction> |                //
        dsl::p<theory_atom> | dsl::p<aggregate> | dsl::p<set_aggregate> | //
        dsl::else_ >> is_atom.create() + dsl::scan + with_term;

    static constexpr auto value = lexy::callback<UHeadLiteral>(
        lexy::forward<UHeadLiteral>,
        [](UTerm term, auto aggr) {
            aggr->set_left_guard(std::move(term), Relation::less_equal);
            return std::move(aggr);
        },
        [](UTerm term, Relation rel, auto aggr) {
            aggr->set_left_guard(std::move(term), rel);
            return std::move(aggr);
        },
        [](UTerm term, Relation rel, UTerm rhs, std::optional<GuardVec> opt_guards, ULiteralVec cond,
           Disjunction::ElementVec elems) {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            elems.insert(elems.begin(),
                         Disjunction::Element{std::make_unique<LiteralRelation>(std::move(term), std::move(guards)),
                                              std::move(cond)});
            return std::make_unique<Disjunction>(std::move(elems));
        },
        [](UTerm term, ULiteralVec cond, Disjunction::ElementVec elems) {
            elems.insert(elems.begin(),
                         Disjunction::Element{std::make_unique<LiteralSymbolic>(std::move(term)), std::move(cond)});
            return std::make_unique<Disjunction>(std::move(elems));
        });
};

struct body_atom : lexy::transparent_production {
    using scan_result = lexy::scan_result<UTerm>;

    struct theory_atom {
        // TODO: proper construction
        static constexpr auto rule = LEXY_LIT("&") >> dsl::p<identifier> + LEXY_LIT("{") + LEXY_LIT("}");
        static constexpr auto value =
            lexy::callback<UBodyLiteral>([](auto &&...) { return std::make_unique<BodyTheoryAtom>(); });
    };

    static constexpr auto right_guard = dsl::peek(LEXY_LIT(":") / LEXY_LIT(".")) |
                                        dsl::else_ >> dsl::if_(dsl::p<relation>) + dsl::p<nested_expr>;

    struct condition {
        static constexpr auto rule = dsl::opt(LEXY_LIT(":") >> dsl::list(dsl::p<literal>, dsl::sep(LEXY_LIT(","))));
        static constexpr auto value = lexy::as_list<ULiteralVec>;
    };

    struct conditional_literal {
        static constexpr auto rule = dsl::p<literal> + dsl::p<condition>;
        static constexpr auto value = lexy::new_<ConditionalLiteral, UBodyLiteral>;
    };

    struct aggregate_element {
        static constexpr auto rule = dsl::opt(dsl::peek_not(LEXY_LIT(":")) >> dsl::p<tuple>) + dsl::p<condition>;
        static constexpr auto value =
            lexy::callback<BodyAggregate::Element>([](std::optional<UTermVec> tuple, std::optional<ULiteralVec> cond) {
                auto ret = BodyAggregate::Element{UTermVec{}, ULiteralVec{}};
                if (tuple) {
                    std::get<0>(ret) = std::move(tuple).value();
                }
                if (cond) {
                    std::get<1>(ret) = std::move(cond).value();
                }
                return ret;
            });
    };

    struct aggregate_elements {
        static constexpr auto rule =
            dsl::opt(dsl::peek_not(LEXY_LIT("}")) >> dsl::list(dsl::p<aggregate_element>, dsl::sep(LEXY_LIT(";"))));
        static constexpr auto value = lexy::as_list<BodyAggregate::ElementVec>;
    };

    struct aggregate {
        static constexpr auto rule = dsl::p<aggregate_function> >>
                                     LEXY_LIT("{") + dsl::p<aggregate_elements> + LEXY_LIT("}") + right_guard;
        static constexpr auto value = lexy::callback<UBodyAggregate>(
            lexy::new_<BodyAggregate, UBodyAggregate>,
            [](AggregateFunction fun, BodyAggregate::ElementVec elems, UTerm rhs) {
                return std::make_unique<BodyAggregate>(fun, std::move(elems), Relation::less_equal, std::move(rhs));
            });
    };

    struct set_aggregate_element {
        static constexpr auto rule = dsl::p<literal> + dsl::p<condition>;
        static constexpr auto value = lexy::construct<BodySetAggregate::Element>;
    };

    struct set_aggregate_elements {
        static constexpr auto rule =
            dsl::opt(dsl::peek_not(LEXY_LIT("}")) >> dsl::list(dsl::p<set_aggregate_element>, dsl::sep(LEXY_LIT(";"))));
        static constexpr auto value = lexy::as_list<BodySetAggregate::ElementVec>;
    };

    struct set_aggregate {
        static constexpr auto rule = LEXY_LIT("{") >> dsl::p<set_aggregate_elements> >> LEXY_LIT("}") + right_guard;
        static constexpr auto value = lexy::callback<UBodySetAggregate>(
            lexy::new_<BodySetAggregate, UBodySetAggregate>, [](BodySetAggregate::ElementVec elems, UTerm rhs) {
                return std::make_unique<BodySetAggregate>(std::move(elems), Relation::less_equal, std::move(rhs));
            });
    };

    static constexpr auto is_atom = dsl::context_flag<body_atom>;

    struct rel_aggr_expected {
        static constexpr auto name = "relation or aggregate expected";
    };

    static constexpr auto with_rel = dsl::p<aggregate> | dsl::p<set_aggregate> |
                                     dsl::else_ >>
                                         dsl::p<nested_expr> + dsl::opt(dsl::p<atom::guards>) + dsl::p<condition>;

    static constexpr auto with_term =               //
        dsl::p<relation> >> with_rel |              //
        dsl::p<aggregate> | dsl::p<set_aggregate> | //
        is_atom.is_set() >> dsl::p<condition> |     //
        dsl::else_ >> dsl::error<rel_aggr_expected>;

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto res_term = scanner.template parse<nested_expr>();
        if (res_term.has_value() && res_term.value()->is_atom()) {
            scanner.parse(is_atom.set());
        }
        return res_term;
    }

    static constexpr auto rule = dsl::p<theory_atom> | dsl::p<aggregate> | dsl::p<set_aggregate> | //
                                 dsl::else_ >> is_atom.create() + dsl::scan + with_term;
    static constexpr auto value = lexy::callback<UBodyLiteral>(
        lexy::forward<UBodyLiteral>,
        [](UTerm term, auto aggr) {
            aggr->set_left_guard(std::move(term), Relation::less_equal);
            return std::move(aggr);
        },
        [](UTerm term, Relation rel, auto aggr) {
            aggr->set_left_guard(std::move(term), rel);
            return std::move(aggr);
        },
        [](UTerm term, Relation rel, UTerm rhs, std::optional<GuardVec> opt_guards, ULiteralVec cond) {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            auto lit = std::make_unique<LiteralRelation>(std::move(term), std::move(guards));
            return std::make_unique<ConditionalLiteral>(std::move(lit), std::move(cond));
        },
        [](UTerm term, ULiteralVec cond) {
            auto lit = std::make_unique<LiteralSymbolic>(std::move(term));
            return std::make_unique<ConditionalLiteral>(std::move(lit), std::move(cond));
        });
};

struct body_literal {
    struct sign {
        static auto constexpr rule = dsl::opt(kw_not::rule) + dsl::opt(kw_not::rule);
        static auto constexpr value = lexy::callback<Sign>([](lexy::nullopt, lexy::nullopt) { return Sign::none; }, //
                                                           [](lexy::nullopt) { return Sign::once; },                //
                                                           []() { return Sign::twice; });
    };

    static constexpr auto rule = dsl::p<sign> + dsl::p<body_atom>;
    static constexpr auto value = lexy::callback<UBodyLiteral>([](Sign sign, UBodyLiteral literal) {
        literal->add_sign(sign);
        return std::move(literal);
    });
};

struct statement : control {
    struct body {
        static constexpr auto sep = LEXY_LIT(",") / LEXY_LIT(";");
        static constexpr auto rule =
            dsl::opt(dsl::peek_not(LEXY_LIT(".")) >> dsl::list(dsl::p<body_literal>, dsl::sep(sep)));
        static constexpr auto value = lexy::as_list<UBodyLiteralVec>;
    };
    static constexpr auto if_body = LEXY_LIT(":-") >> dsl::p<body> + LEXY_LIT(".");
    static constexpr auto rule = if_body | dsl::else_ >> dsl::p<head_literal> + (LEXY_LIT(".") | if_body);
    // static constexpr auto rule = dsl::p<head_literal> + LEXY_LIT(".");
    static constexpr auto value = lexy::callback<UStatement>(
        lexy::new_<Rule, UStatement>,
        [](UHeadLiteral head) { return std::make_unique<Rule>(std::move(head), UBodyLiteralVec{}); },
        [](UBodyLiteralVec body) {
            return std::make_unique<Rule>(std::make_unique<Disjunction>(Disjunction::ElementVec{}), std::move(body));
        });
};

} // namespace grammar

namespace test {

namespace dsl = lexy::dsl;

template <class P, char t = '\0'> struct parse_root : grammar::control {
    static constexpr auto terminator() { return t; }
    static constexpr auto eof() {
        if constexpr (t == '\0') {
            return dsl::eof;
        } else {
            return dsl::lit_c<t> + dsl::eof;
        }
    }
    static constexpr auto rule = dsl::p<P> + eof();
    static constexpr auto value = lexy::forward<typename decltype(P::value)::return_type>;
};

template <class P> struct match_root : grammar::control {
    static constexpr auto rule = dsl::p<P> + dsl::eof;
};

using term = parse_root<grammar::nested_expr>;
using literal = parse_root<grammar::literal>;
using head_literal = parse_root<grammar::head_literal, '.'>;
using body_literal = parse_root<grammar::body_literal, '.'>;
using statement = parse_root<grammar::statement>;

} // namespace test

template <typename Control> auto parse(std::string str) -> std::string {
    if (Control::terminator() != '\0') {
        str.push_back('.');
    }
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
    REQUIRE(parse<test::literal>("5") == "<failed>");
}

TEST_CASE("head literals") {
    // theory_atom | aggregate | set_aggregate | not disjunction
    REQUIRE(parse<test::head_literal>("&x{}") == "&p{...}");
    REQUIRE(parse<test::head_literal>("#count{}") == "#count{}");
    REQUIRE(parse<test::head_literal>("{}") == "{}");
    REQUIRE(parse<test::head_literal>("not a") == "not a");
    // atom_like relation aggregate
    REQUIRE(parse<test::head_literal>("a<{}") == "a<{}");
    REQUIRE(parse<test::head_literal>("a<#count{}") == "a<#count{}");
    // atom_like relation term ...
    REQUIRE(parse<test::head_literal>("a<b<c") == "a<b<c");
    REQUIRE(parse<test::head_literal>("a<a:a") == "a<a:a");
    REQUIRE(parse<test::head_literal>("a<a:a;a") == "a<a:a;a");
    REQUIRE(parse<test::head_literal>("a<a,a") == "a<a;a");
    // atom_like aggregate
    REQUIRE(parse<test::head_literal>("a{}") == "a<={}");
    REQUIRE(parse<test::head_literal>("a#count{}") == "a<=#count{}");
    // term aggregate
    REQUIRE(parse<test::head_literal>("a+1{}") == "(a+1)<={}");
    REQUIRE(parse<test::head_literal>("a+1#count{}") == "(a+1)<=#count{}");
    // term relation aggregate
    REQUIRE(parse<test::head_literal>("a+1<{}") == "(a+1)<{}");
    REQUIRE(parse<test::head_literal>("a+1<#count{}") == "(a+1)<#count{}");
    // term relation term ...
    REQUIRE(parse<test::head_literal>("a+1<b<c") == "(a+1)<b<c");
    REQUIRE(parse<test::head_literal>("a+1<a:a") == "(a+1)<a:a");
    REQUIRE(parse<test::head_literal>("a+1<a:a;a") == "(a+1)<a:a;a");
    REQUIRE(parse<test::head_literal>("a+1<a,a") == "(a+1)<a;a");
    REQUIRE(parse<test::head_literal>("a+1<>a,a") == "<failed>");
    // atom ...
    REQUIRE(parse<test::head_literal>("-a") == "(-a)");
    REQUIRE(parse<test::head_literal>("-a(X)") == "(-a(X))");
    REQUIRE(parse<test::head_literal>("a:a") == "a:a");
    REQUIRE(parse<test::head_literal>("a:a;a") == "a:a;a");
    REQUIRE(parse<test::head_literal>("a,b") == "a;b");
    REQUIRE(parse<test::head_literal>("a;b") == "a;b");
    REQUIRE(parse<test::head_literal>("a|b") == "a;b");
    // aggregates with guards
    REQUIRE(parse<test::head_literal>("a<{}<b") == "a<{}<b");
    REQUIRE(parse<test::head_literal>("a{}b") == "a<={}<=b");
    // aggregate elements
    REQUIRE(parse<test::head_literal>("#sum{:a;1:a;1,2:a:b,c}") == "#sum{:a;1:a;1,2:a:b,c}");
    REQUIRE(parse<test::head_literal>("{1<2;1<2:a;a:b;a:b,c}") == "{1<2;1<2:a;a:b;a:b,c}");
}

TEST_CASE("body literals") {
    // negation
    REQUIRE(parse<test::body_literal>("a") == "a");
    REQUIRE(parse<test::body_literal>("not a") == "not a");
    REQUIRE(parse<test::body_literal>("not not a") == "not not a");
    // theory_atom | aggregate | set_aggregate
    REQUIRE(parse<test::body_literal>("&x{}") == "&p{...}");
    REQUIRE(parse<test::body_literal>("#count{}") == "#count{}");
    REQUIRE(parse<test::body_literal>("{}") == "{}");
    // atom_like relation aggregate
    REQUIRE(parse<test::body_literal>("a<{}") == "a<{}");
    REQUIRE(parse<test::body_literal>("a<#count{}") == "a<#count{}");
    // atom_like relation term ...
    REQUIRE(parse<test::body_literal>("a<b<c") == "a<b<c");
    REQUIRE(parse<test::body_literal>("a<a:a") == "a<a:a");
    // atom_like aggregate
    REQUIRE(parse<test::body_literal>("a{}") == "a<={}");
    REQUIRE(parse<test::body_literal>("a#count{}") == "a<=#count{}");
    // term aggregate
    REQUIRE(parse<test::body_literal>("a+1{}") == "(a+1)<={}");
    REQUIRE(parse<test::body_literal>("a+1#count{}") == "(a+1)<=#count{}");
    // term relation aggregate
    REQUIRE(parse<test::body_literal>("a+1<{}") == "(a+1)<{}");
    REQUIRE(parse<test::body_literal>("a+1<#count{}") == "(a+1)<#count{}");
    // term relation term ...
    REQUIRE(parse<test::body_literal>("a+1<b<c") == "(a+1)<b<c");
    REQUIRE(parse<test::body_literal>("a+1<a:a") == "(a+1)<a:a");
    // atom ...
    REQUIRE(parse<test::body_literal>("-a") == "(-a)");
    REQUIRE(parse<test::body_literal>("-a(X)") == "(-a(X))");
    REQUIRE(parse<test::body_literal>("a:b,c") == "a:b,c");
    // aggregates with guards
    REQUIRE(parse<test::body_literal>("a<{}<b") == "a<{}<b");
    REQUIRE(parse<test::body_literal>("a{}b") == "a<={}<=b");
    // aggregate elements
    REQUIRE(parse<test::body_literal>("#sum{:a;1:a;1,2:a,b,c}") == "#sum{:a;1:a;1,2:a,b,c}");
    REQUIRE(parse<test::body_literal>("{1<2;1<2:a;a:b;a:b,c}") == "{1<2;1<2:a;a:b;a:b,c}");
}

TEST_CASE("statement") {
    // TODO
    // 1. ensure `:` is never followed by `-`
    // 2. print with spaces
    REQUIRE(parse<test::statement>(":-.") == ":-.");
    REQUIRE(parse<test::statement>("a.") == "a.");
    REQUIRE(parse<test::statement>("a:-.") == "a.");
    REQUIRE(parse<test::statement>("a:-b.") == "a:-b.");
    REQUIRE(parse<test::statement>("a:-b,c.") == "a:-b;c.");
    REQUIRE(parse<test::statement>("a:-b;c.") == "a:-b;c.");
    REQUIRE(parse<test::statement>("a:-a:b,c;d.") == "a:-a:b,c;d.");
    REQUIRE(parse<test::statement>(":-.") == ":-.");
}

TEST_CASE("program") {
    std::istringstream in;
    in.str("a.b.c");
    auto input = grammar::input{in};
    auto scanner = lexy::scan<grammar::control>(input, report_error);
    auto stm = scanner.parse<grammar::statement>();
    REQUIRE(stm.has_value());
    REQUIRE(stm.value()->to_string() == "a.");
    input.discard_before(scanner.position());
    stm = scanner.parse<grammar::statement>();
    REQUIRE(stm.has_value());
    REQUIRE(stm.value()->to_string() == "b.");
    input.discard_before(scanner.position());
    stm = scanner.parse<grammar::statement>();
    REQUIRE(!stm.has_value());
}
