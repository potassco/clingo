#include <input/algo/rewrite_arithmetic.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

template <class T> auto simplify_str(std::optional<T> value) -> std::string {
    if (value) {
        auto store = make_symbol_store(true, true);
        auto res = simplify(*store, value.value());
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

TEST_CASE("simplify") {
    REQUIRE(simplify_str(parse_term("1+2")) == "3");
    REQUIRE(simplify_str(parse_term("-1")) == "-1");
    REQUIRE(simplify_str(parse_term("-f(-|1-2|)")) == "-f(-1)");
    REQUIRE(simplify_str(parse_term("-f(-|X|)")) == "-f(-|X|)");
    REQUIRE(simplify_str(parse_term("-f(1+2+X,-X)")) == "-f(3+X,-X)");
    REQUIRE(simplify_str(parse_term("(1+2+X,-X)")) == "(3+X,-X)");
    REQUIRE(simplify_str(parse_term("--f")) == "f");
    REQUIRE(simplify_str(parse_term("---f")) == "-f");
    REQUIRE(simplify_str(parse_term("1+a")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("f(1+a)")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("f(X+a)")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("-(1,2)")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("-(1,X)")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("|()|")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("|(X,)|")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("|f|")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("|f(X)|")) == "<undefined>");
}

} // namespace Gringo::Input::Test
