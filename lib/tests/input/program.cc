#include <logger.hh>

#include <input/program.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

namespace {

using SV = std::vector<std::string>;

auto rewrite_program(std::vector<char const *> stms) -> SV {
    ParseHelper ph;
    UnprocessedProgram upr;
    for (auto const *str : stms) {
        add(ph, *ph.statement(str), upr);
    }
    Program prg{RewriteOptions{}};
    prg.join(ph, ph, std::move(upr));
    SV ret;
    prg.visit_stms(ph, [&ret](auto const &stm) { ret.emplace_back(to_str(stm)); });
    return ret;
}

} // namespace

TEST_CASE("rewrite_program") {
    REQUIRE(rewrite_program({"#const n = 1.", "#const m = n.", "#const o = n+k.", "#program part(k,n).", "a(k,n).",
                             "b(k,m,X) :- a(k,X)."}) == SV{"#const n=1. [default]", "#const m=1. [default]",
                                                           "#program part(k,n).", "a(k,n).", "b(k,1,X) :- a(k,X)."});
}

} // namespace Gringo::Input::Test
