#include <input/program.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

TEST_CASE("program") {
    ParseHelper ph;
    UnprocessedProgram upr;
    upr.add(ph, *ph.statement("#const n = 1."));
    upr.add(ph, *ph.statement("#const m = n."));
    upr.add(ph, *ph.statement("#program part(k,n)."));
    upr.add(ph, *ph.statement("a(k,n)."));
    upr.add(ph, *ph.statement("b(k,m,X) :- a(k,X)."));

    Program prg{RewriteOptions{}};
    prg.join(ph, ph, std::move(upr));
}

} // namespace Gringo::Input::Test
