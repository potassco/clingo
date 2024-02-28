#include "gringo/input/test.hh"

#include <gringo/input/program.hh>

#include <gringo/logger.hh>

namespace Gringo::Input::Test {

namespace {

using SV = std::vector<std::string>;

auto rewrite_program(std::vector<char const *> stms) -> SV {
    ParseHelper ph;
    UnprocessedProgram upr;
    for (auto const *str : stms) {
        if (auto stm = ph.statement(str); stm) {
            add(ph, *stm, upr);
        }
    }
    Program prg{RewriteOptions{}};
    prg.join(ph, ph, std::move(upr));
    SV ret;
    prg.visit_stms(ph, [&ret](auto const &stm) { ret.emplace_back(to_str(stm)); });
    return ret;
}

} // namespace

TEST_CASE("rewrite_program") {
    // test const evaluation
    REQUIRE(rewrite_program({"#const n = 1.", "#const m = n.", "#const o = n+k.", "#program part(k,n).", "a(k,n).",
                             "b(k,m,X) :- a(k,X)."}) == SV{"#const n=1. [default]", "#const m=1. [default]",
                                                           "#program part(k,n).", "a(k,n).", "b(k,1,X) :- a(k,X)."});
    // test parameter mapping
    REQUIRE(rewrite_program({"#program a(x,y).", "p(x,z).", "#program a(y,z).", "p(y,z)."}) ==
            SV{"#program a(x,y).", "p(x,z).", "p(x,y)."});
    REQUIRE(rewrite_program({"#program a(x,y).", "p(x,z).", "#program a(y,z).", "p(x,z)."}) ==
            SV{"#program a(__p_0,y).", "p(__p_0,z).", "p(x,y)."});
    REQUIRE(rewrite_program({"#program a(x,y).", "p(x,z).", "#program a(y,z).", "p(x,z,__p_0)."}) ==
            SV{"#program a(__p_1,y).", "p(__p_1,z).", "p(x,y,__p_0)."});
}

} // namespace Gringo::Input::Test
