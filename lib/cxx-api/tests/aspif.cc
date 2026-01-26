#include <algorithm>
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
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
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
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
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

TEST_CASE_METHOD(Fixture, "aspif choice", "[cxx][aspif][choice]") {
    parse(R"(asp 1 0 0
1 1 3 1 2 3 0 0
1 0 0 1 2 3 1 1 2 1 3 1
4 1 a 1 1
4 1 b 1 2
4 1 c 1 3
0
)");
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {},
                          {"a"},
                          {"b"},
                          {"c"},
                      });
}

TEST_CASE_METHOD(Fixture, "aspif disjunction", "[cxx][aspif][disjunction]") {
    parse(R"(asp 1 0 0
1 0 1 4 0 1 3
1 0 1 3 0 1 4
1 0 1 1 0 1 2
1 0 1 2 0 1 1
1 0 2 1 2 0 2 -3 -4
1 0 2 3 4 0 2 -1 -2
4 1 a 1 1
4 1 b 1 2
4 1 c 1 3
4 1 d 1 4
0
)");
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {"a", "b"},
                          {"c", "d"},
                      });
}

TEST_CASE_METHOD(Fixture, "aspif project", "[cxx][aspif][project]") {
    ctl = Control(lib, {"--project", "0"});
    parse(R"(asp 1 0 0
1 0 1 1 0 0
1 0 0 0 1 -4
1 0 1 6 0 1 4
1 1 2 2 3 0 0
1 0 1 4 0 1 3
1 0 1 4 0 1 2
1 0 0 0 2 3 2
4 1 a 1 3
3 2 2 3
0
)");
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {},
                          {"a"},
                      });
}

TEST_CASE_METHOD(Fixture, "aspif output v1 fact", "[cxx][aspif][output_v1][fact]") {
    parse(R"(asp 1 0 0
1 0 1 1 0 0
4 1 a 0
0
)");
    auto sym = Function(lib, "a", {});
    REQUIRE(!ctl.base().terms().contains(sym));
    REQUIRE(ctl.base().contains(sym));
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {"a"},
                      });
}

TEST_CASE_METHOD(Fixture, "aspif output v1 cond", "[cxx][aspif][output_v1][cond]") {
    parse(R"(asp 1 0 0
1 1 2 1 2 0 0
4 1 a 1 1
4 1 b 1 2
4 1 c 2 1 2
4 1 d 0
0
)");
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {"a", "b", "c", "d"},
                          {"a", "d"},
                          {"b", "d"},
                          {"d"},
                      });
    auto sym = Function(lib, "d", {});
    REQUIRE(ctl.base().terms().contains(sym));
    REQUIRE_FALSE(ctl.base().contains(sym));
}

TEST_CASE_METHOD(Fixture, "aspif output v2 cond", "[cxx][aspif][output_v2][cond]") {
    parse(R"(asp 2 0 0
1 1 2 1 2 0 0
4 1 0 1 a
4 2 0 0
4 1 1 1 b
4 2 1 1 1
4 1 2 1 c
4 2 2 2 1 2
0
)");
    auto a = Function(lib, "a", {});
    auto b = Function(lib, "b", {});
    auto c = Function(lib, "c", {});
    REQUIRE(ctl.base().terms().contains(a));
    REQUIRE(ctl.base().terms().contains(b));
    REQUIRE(ctl.base().terms().contains(c));
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {"a"},
                          {"a"},
                          {"a", "b"},
                          {"a", "b", "c"},
                      });
}

TEST_CASE_METHOD(Fixture, "aspif output symbols", "[cxx][aspif][output_v2][symbols]") {
    parse(R"(asp 2 0 0 symbols
1 0 1 1 0 0
1 0 1 2 0 0
1 0 1 3 0 0
4 5 0 1 p
4 4 1 1
4 7 2 0 0 1 1
4 0 2 1
4 4 3 2
4 7 4 0 0 1 3
4 0 4 2
4 4 5 3
4 7 6 0 0 1 5
4 0 6 3
0
)");
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {"p(1)", "p(2)", "p(3)"},
                      });

    ctl = Control(lib, {"0", "--opt-mode=optN"});
    parse(R"(asp 2 0 0 symbols
1 0 1 1 0 0
4 5 0 1 p
4 3 1 0
4 3 2 1
4 5 3 5 a"b
c
4 5 4 1 f
4 4 5 1
4 7 6 0 4 1 5
4 6 7 1 5
4 6 8 0
4 7 9 0 0 6 1 2 3 6 7 8
4 0 9 1
0
)");
    models.clear();
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    const auto *res = R"(p(#inf,#sup,"a\"b\nc",f(1),(1,),()))";
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {res},
                      });
}

TEST_CASE_METHOD(Fixture, "aspif external", "[cxx][aspif][external]") {
    parse(R"(asp 1 0 0
5 1 1
5 2 2
5 3 0
4 1 a 1 1
4 1 b 1 2
4 1 c 1 3
0
)");
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {"a"},
                          {"a", "c"},
                      });
}

TEST_CASE_METHOD(Fixture, "aspif assume", "[cxx][aspif][assume]") {
    parse(R"(asp 1 0 0
1 1 3 1 2 3 0 0
6 2 1 -2
4 1 a 1 1
4 1 b 1 2
4 1 c 1 3
0
)");
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {"a"},
                          {"a", "c"},
                      });
}

TEST_CASE_METHOD(Fixture, "aspif heuristic", "[cxx][aspif][heuristic]") {
    ctl = Control(lib, {"--heuristic", "domain"});
    parse(R"(asp 1 0 0
1 1 2 1 2 0 0
4 1 a 1 1
4 1 b 1 2
7 4 1 1 0 0
7 5 2 0 0 0
0
)");
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {"a"},
                      });
}

TEST_CASE_METHOD(Fixture, "aspif edge", "[cxx][aspif][edge]") {
    parse(R"(asp 1 0 0
1 1 2 1 2 0 0
4 1 a 1 1
4 1 b 1 2
8 0 1 1 2
8 1 0 1 1
0
)");
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {},
                          {"a"},
                          {"b"},
                      });
}

TEST_CASE_METHOD(Fixture, "aspif theory", "[cxx][aspif][theory]") {
    parse(R"(asp 1 0 0
1 0 1 2 0 0
1 0 1 3 0 0
1 0 1 5 0 0
1 0 1 6 0 0
1 0 1 7 0 1 1
1 1 1 1 0 0
4 1 a 1 1
9 1 9 1 p
9 1 1 1 f
9 0 2 1
9 0 3 2
9 1 4 1 +
9 2 5 4 2 2 3
9 1 6 1 g
9 2 7 6 1 5
9 4 8 2 1 7 1 1
9 1 10 1 <
9 0 0 5
9 6 2 9 1 8 10 0
9 0 11 3
9 2 12 -1 3 2 3 11
9 4 13 1 12 0
9 5 3 9 1 13
9 2 14 -3 3 2 3 11
9 4 15 1 14 0
9 5 5 9 1 15
9 2 16 -2 3 2 3 11
9 4 17 1 16 0
9 5 6 9 1 17
9 4 18 1 2 0
9 5 7 9 1 18
0
)");
    std::vector<std::string> theory_atoms;
    for (const auto &atom : ctl.base().theory()) {
        theory_atoms.push_back(atom.to_string());
    }
    std::ranges::sort(theory_atoms);
    REQUIRE(theory_atoms == std::vector<std::string>{
                                "&p { (1,2,3) }",
                                "&p { 1 }",
                                "&p { [1,2,3] }",
                                "&p { f,g((1+2)): <literal: 1> } < 5",
                                "&p { {1,2,3} }",
                            });
}

TEST_CASE_METHOD(Fixture, "aspif comment", "[cxx][aspif][comment]") {
    parse(R"(asp 1 0 0
10 123
1 1 1 1 0 0
10 abc
4 1 a 1 1
0
)");
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {},
                          {"a"},
                      });
}

TEST_CASE_METHOD(Fixture, "aspif incremental", "[cxx][aspif][incremental]") {
    parse(R"(asp 1 0 0 incremental
1 1 1 1 0 0
4 1 a 1 1
0
1 1 1 2 0 0
4 1 b 1 2
0
)");
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {},
                          {"a"},
                          {"a", "b"},
                          {"b"},
                      });
}

TEST_CASE_METHOD(Fixture, "aspif incremental theory", "[cxx][aspif][incremental][theory]") {
    parse(R"(asp 1 0 0 incremental
1 0 1 1 0 0
9 1 2 1 p
9 1 0 1 a
9 4 1 1 0 0
9 5 1 2 1 1
0
1 0 1 3 0 0
9 1 2 1 p
9 1 0 1 b
9 4 1 1 0 0
9 5 3 2 1 1
0
)");
    std::vector<std::string> theory_atoms;
    for (const auto &atom : ctl.base().theory()) {
        theory_atoms.push_back(atom.to_string());
    }
    std::ranges::sort(theory_atoms);
    REQUIRE(theory_atoms == std::vector<std::string>{
                                "&p { a }",
                                "&p { b }",
                            });
}

TEST_CASE_METHOD(Fixture, "aspif incremental assume", "[cxx][aspif][incremental][assume]") {
    parse(R"(asp 1 0 0 incremental
1 1 1 1 0 0
6 1 1
4 1 a 1 1
0
1 1 1 2 0 0
6 1 2
4 1 b 1 2
0
)");
    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{
                          {"a", "b"},
                          {"b"},
                      });
}

} // namespace Clingo::Test
