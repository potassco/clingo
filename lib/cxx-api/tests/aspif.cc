#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

#include "cbs.hh"
#include "tempfile.hh"

namespace Clingo::Test {

namespace {

struct Fixture {
    auto parse(std::string_view content) const {
        auto file = TempFile{content};
        ctl.parse_files({file.path().string()});
    }

    Library lib;
    Control ctl{lib, {"0"}};
    Base bse{ctl.base()};
};

} // namespace

TEST_CASE_METHOD(Fixture, "aspif rule", "[cxx][aspif][rule]") {
    parse(R"(asp 1 0 0
1 0 1 1 0 0
1 0 1 2 0 0
1 0 1 3 0 0
1 0 1 4 0 1 -5
1 0 1 6 0 1 -7
1 0 1 8 0 1 -9
1 0 1 5 0 1 -4
1 0 1 7 0 1 -6
1 0 1 9 0 1 -8
4 4 q(1) 1 4
4 4 p(1) 1 5
4 4 q(2) 1 6
4 4 p(2) 1 7
4 4 q(3) 1 8
4 4 p(3) 1 9
0
)");
    auto models = std::vector<std::vector<std::string>>{};
    {
        auto mcb = MCB{models};
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {"p(1)", "p(2)", "p(3)"},
                          {"p(1)", "p(2)", "q(3)"},
                          {"p(1)", "p(3)", "q(2)"},
                          {"p(1)", "q(2)", "q(3)"},
                          {"p(2)", "p(3)", "q(1)"},
                          {"p(2)", "q(1)", "q(3)"},
                          {"p(3)", "q(1)", "q(2)"},
                          {"q(1)", "q(2)", "q(3)"},
                      });
    ctl.parse_string("r(X) :- q(X-1), p(X), q(X+1).");
    ctl.ground();
    {
        auto mcb = MCB{models};
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {"p(1)", "p(2)", "p(3)"},
                          {"p(1)", "p(2)", "q(3)"},
                          {"p(1)", "p(3)", "q(2)"},
                          {"p(1)", "q(2)", "q(3)"},
                          {"p(2)", "p(3)", "q(1)"},
                          {"p(2)", "q(1)", "q(3)", "r(2)"},
                          {"p(3)", "q(1)", "q(2)"},
                          {"q(1)", "q(2)", "q(3)"},
                      });
}

} // namespace Clingo::Test
