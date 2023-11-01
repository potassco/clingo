#include <input/algo/rewrite_arithmetic.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

template <class T> auto simplify_str(std::optional<T> value) -> std::string {
    if (value) {
        auto store = make_symbol_store(true, true);
        NameGen gen{*store, {}, "__Aux_"};
        auto res = simplify(*store, gen, value.value());
        return std::visit(
            [&value](auto &&val) -> std::string {
                GRINGO_MATCH(val, Symbol) {
                    std::ostringstream oss;
                    oss << val;
                    return oss.str();
                }
                GRINGO_MATCH(val, Term) { return to_str(val); }
                GRINGO_MATCH(val, std::monostate) { return "<undefined>"; }
                GRINGO_MATCH(val, std::nullopt_t) { return to_str(value.value()); }
            },
            res);
    }
    return "<failed>";
}

TEST_CASE("simplify_unary") {
    // numeric
    REQUIRE(simplify_str(parse_term("-1")) == "-1");
    REQUIRE(simplify_str(parse_term("-X+1")) == "-1*X+1");
    REQUIRE(simplify_str(parse_term("|-1|")) == "1");
    // any
    REQUIRE(simplify_str(parse_term("-X")) == "-X");
    REQUIRE(simplify_str(parse_term("--X")) == "-(-X)");
    REQUIRE(simplify_str(parse_term("---X")) == "-X");
    REQUIRE(simplify_str(parse_term("|-X|")) == "|-X|");
    REQUIRE(simplify_str(parse_term("|1*X+0|")) == "|X|");
    // symbolic
    REQUIRE(simplify_str(parse_term("--f")) == "f");
    REQUIRE(simplify_str(parse_term("---f")) == "-f");
    REQUIRE(simplify_str(parse_term("-f(-|X|)")) == "-f(-|X|)");
    // fail
    REQUIRE(simplify_str(parse_term("~a")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("-(1,2)")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("-(1,X)")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("|()|")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("|(X,)|")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("|f|")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("|f(X)|")) == "<undefined>");
}

TEST_CASE("simplify_binary") {
    // evaluate constant
    REQUIRE(simplify_str(parse_term("1+2")) == "3");
    // keep variables
    REQUIRE(simplify_str(parse_term("X")) == "X");
    // variable to linear
    REQUIRE(simplify_str(parse_term("X+0")) == "1*X+0");
    // linear + constant
    REQUIRE(simplify_str(parse_term("(2*X+3)+2")) == "2*X+5");
    REQUIRE(simplify_str(parse_term("(2*X+3)-2")) == "2*X+1");
    REQUIRE(simplify_str(parse_term("(2*X+3)*2")) == "4*X+6");
    REQUIRE(simplify_str(parse_term("(2*X+3)/2")) == "(2*X+3)/2");
    REQUIRE(simplify_str(parse_term("(1*X+0)/2")) == "X/2");
    REQUIRE(simplify_str(parse_term("(1*X+0)*0")) == "X*0");
    // constant + linear
    REQUIRE(simplify_str(parse_term("2*(2*X+3)")) == "4*X+6");
    REQUIRE(simplify_str(parse_term("2+(2*X+3)")) == "2*X+5");
    REQUIRE(simplify_str(parse_term("2-(2*X+3)")) == "-2*X+(-1)");
    REQUIRE(simplify_str(parse_term("2/(2*X+3)")) == "2/(2*X+3)");
    REQUIRE(simplify_str(parse_term("2/(1*X+0)")) == "2/X");
    REQUIRE(simplify_str(parse_term("0*(1*X+0)")) == "0*X");
    // linear + linear
    REQUIRE(simplify_str(parse_term("(2*X+3)+(3*X+5)")) == "5*X+8");
    REQUIRE(simplify_str(parse_term("(2*X+3)-(3*X+5)")) == "-1*X+(-2)");
    REQUIRE(simplify_str(parse_term("(2*X+3)-(2*X+5)")) == "0*X+(-2)");
    REQUIRE(simplify_str(parse_term("(2*X+3)+(3*Y+5)")) == "2*X+(3*Y+8)");
    REQUIRE(simplify_str(parse_term("(2*X+3)-(3*Y+5)")) == "2*X-(3*Y+2)");
    // unchanged + unchanged
    REQUIRE(simplify_str(parse_term("(X/2)-(Y/2)")) == "X/2-Y/2");
    // changed + changed
    REQUIRE(simplify_str(parse_term("(X/(2+0))-(Y/(2+0))")) == "X/2-Y/2");
    // fail
    REQUIRE(simplify_str(parse_term("1+a")) == "<undefined>");
}

TEST_CASE("simplify_symbolic") {
    REQUIRE(simplify_str(parse_term("-f(-|1-2|)")) == "-f(-1)");
    REQUIRE(simplify_str(parse_term("-f(1+2+X,-X)")) == "-f(1*X+3,-X)");
    REQUIRE(simplify_str(parse_term("(1+2+X,-X)")) == "(1*X+3,-X)");
    REQUIRE(simplify_str(parse_term("f(1+a)")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("f(X+a)")) == "<undefined>");
}

TEST_CASE("simplify_aux") {
    // TODO: also inspect what has been extracted
    REQUIRE(simplify_str(parse_term("1..2")) == "1*__Aux_0+0");
}
} // namespace Gringo::Input::Test
