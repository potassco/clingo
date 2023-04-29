#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include <lexy/action/parse.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/grammar.hpp>

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

struct TermInteger : Term {
    explicit TermInteger(int v) : value(v) {}

    void print(std::ostream &out) const override { out << value; }

    int value;
};

struct TermVariable : Term {
    explicit TermVariable(std::string name) : name(std::move(name)) {}

    void print(std::ostream &out) const override { out << name; }

    std::string name;
};

struct TermFunction : Term {
    explicit TermFunction(std::string name) : name(std::move(name)) {}

    void print(std::ostream &out) const override { out << name; }

    std::string name;
};

enum class UnaryOperator {
    negate,
};

auto operator<<(std::ostream &out, UnaryOperator op) -> std::ostream & {
    assert(op == UnaryOperator::negate);
    out << "-";
    return out;
}

struct TermUnary : Term {
    explicit TermUnary(UnaryOperator op, UTerm e) : op(op), rhs(std::move(e)) {}

    void print(std::ostream &out) const override { out << "(" << op << *rhs << ")"; }

    UnaryOperator op;
    UTerm rhs;
};

enum BinaryOperator {
    plus,
    minus,
    times,
    div,
    pow,
};

auto operator<<(std::ostream &out, BinaryOperator op) -> std::ostream & {
    switch (op) {
        case plus: {
            out << "+";
            break;
        }
        case minus: {
            out << "-";
            break;
        }
        case times: {
            out << "*";
            break;
        }
        case div: {
            out << "/";
            break;
        }
        case pow: {
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
using iterator = StreamInput<>::iterator;

struct integer : lexy::token_production {
    static constexpr auto rule = LEXY_LIT("0x") >> dsl::integer<int, dsl::hex> | dsl::integer<int>;
    static constexpr auto value = lexy::forward<int>;
};

struct nested_expr : lexy::transparent_production {
    static constexpr auto whitespace = dsl::ascii::space | dsl::newline;
    static constexpr auto rule = dsl::recurse<struct expr>;
    static constexpr auto value = lexy::forward<UTerm>;
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
        if (scanner.branch(num_res, dsl::p<integer>)) {
            res = std::make_unique<TermInteger>(num_res.value());
            return res;
        }
        // parse a variable
        auto begin = scanner.position();

        while (scanner.branch(LEXY_LIT("_") / LEXY_LIT("'"))) {
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

struct expr : lexy::expression_production {
    struct expected_operand {
        static constexpr auto name = "expected operand";
    };

    static constexpr auto atom = dsl::p<atom_expr>;

    struct math_power : dsl::infix_op_right {
        static constexpr auto op = dsl::op<BinaryOperator::pow>(LEXY_LIT("**"));
        using operand = dsl::atom;
    };

    struct math_prefix : dsl::prefix_op {
        static constexpr auto op = dsl::op<UnaryOperator::negate>(LEXY_LIT("-"));
        using operand = math_power;
    };

    struct math_product : dsl::infix_op_left {
        static constexpr auto op = [] {
            auto star = dsl::not_followed_by(LEXY_LIT("*"), dsl::lit_c<'*'>);
            return dsl::op<BinaryOperator::times>(star) / dsl::op<BinaryOperator::div>(LEXY_LIT("/"));
        }();
        using operand = math_prefix;
    };

    struct math_sum : dsl::infix_op_left {
        static constexpr auto op =
            dsl::op<BinaryOperator::plus>(LEXY_LIT("+")) / dsl::op<BinaryOperator::minus>(LEXY_LIT("-"));
        using operand = math_product;
    };

    using operation = math_sum;
    static constexpr auto value =
        lexy::callback(lexy::forward<UTerm>, lexy::new_<TermVariable, UTerm>, lexy::new_<TermInteger, UTerm>,
                       lexy::new_<TermUnary, UTerm>, lexy::new_<TermBinary, UTerm>);
};

struct statement {
    static constexpr auto rule = dsl::p<nested_expr> + dsl::lit_c<';'> + dsl::position;
    static constexpr auto value = lexy::construct<std::pair<UTerm, iterator>>;
};

} // namespace grammar

} // namespace

TEST_CASE("term-test-working") {
    std::istringstream in;
    in.str("42  *-\n2-32**3+'_Xa_';\n43+'_$;");
    auto input = StreamInput{in};
    auto stm = lexy::parse<grammar::statement>(input, report_error);
    REQUIRE(stm.has_value());
    REQUIRE(stm.value().first->to_string() == "(((42*(-2))-(32**3))+'_Xa_')");
    input.discard_before(stm.value().second);
    stm = lexy::parse<grammar::statement>(input, report_error);
    REQUIRE(!stm.has_value());
}
