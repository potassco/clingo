#include "test.hh"

#include <clingo/input/rewrite/rewrite_theory.hh>

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Input::Test {

namespace {

auto rewrite_statement(std::string const &str) -> std::string {
    auto ph = ParseHelper{};
    auto thy = std::get<StmTheory>(opt_value(ph.statement(R"(
        #theory x {
            a {
                - : 1, unary;
                + : 2, binary, left;
                - : 3, binary, right;
                + : 4, unary
            };
            b {
                * : 1, binary, left;
                / : 2, binary, right
            };
            &p/0: a, {<,>}, b, any

        }.)")));
    ph.parser().add_theory(ph.logger(), thy);

    if (auto stm = ph.statement(str); stm) {
        if (auto rev = rewrite_theory(ph, *stm); rev) {
            return to_str(*rev);
        }
        if (ph.parser().has_error()) {
            return "<error>";
        }
        return to_str(*stm);
    }
    return "<error>";
}

} // namespace

TEST_CASE("rewrite_theory") {
    REQUIRE(rewrite_statement("&p { +x-y }.") == "&p { ((+ x) - y) }.");
    REQUIRE(rewrite_statement("&p { -x+y }.") == "&p { (- (x + y)) }.");
    REQUIRE(rewrite_statement("&p { x+y-z }.") == "&p { (x + (y - z)) }.");
    REQUIRE(rewrite_statement("&p { x-y+z }.") == "&p { ((x - y) + z) }.");
    REQUIRE(rewrite_statement("&p { x+y+z }.") == "&p { ((x + y) + z) }.");
    REQUIRE(rewrite_statement("&p { x-y-z }.") == "&p { (x - (y - z)) }.");
    REQUIRE(rewrite_statement("&p { -x+y+z-a-b-c } < x*y*z/a/b/c.") ==
            "&p { (- ((x + y) + (z - (a - (b - c))))) } < ((x * y) * (z / (a / (b / c)))).");
    REQUIRE(rewrite_statement("&p { -x+y+z-a-b-c } < x/y/z*a*c.") ==
            "&p { (- ((x + y) + (z - (a - (b - c))))) } < (((x / (y / z)) * a) * c).");
}

} // namespace CppClingo::Input::Test
