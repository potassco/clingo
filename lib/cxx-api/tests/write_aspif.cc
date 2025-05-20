#include <algorithm>
#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "cbs.hh"
#include "tempfile.hh"

namespace Clingo::Test {

namespace {

struct Fixture {
    Library lib;
    Control ctl = Control(lib, {"0", "--opt-mode=optN"});
    MV models;
    int n = 0;

    [[nodiscard]] auto parse(std::string_view content, bool symbols = false) const -> TempFile {
        auto ctl = Control{lib};
        ctl.parse_string(content);
        ctl.ground();
        auto tmp = TempFile{};
        ctl.write_aspif(tmp.path().c_str(),
                        (symbols ? WriteAspifFlags::symbols : WriteAspifFlags::none) | WriteAspifFlags::preamble_auto);
        return tmp;
    }
};

} // namespace

TEST_CASE_METHOD(Fixture, "write_aspif rule", "[cxx][write_aspif][rule]") {
    {
        auto tmp = parse("a. {b}. c :- b.");
        ctl.parse_files({tmp.path().c_str()});
    }
    {
        auto mcb = MCB(models);
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == MV{{"a"}, {"a", "b", "c"}});
}

TEST_CASE_METHOD(Fixture, "write_aspif aggregate", "[cxx][write_aspif][aggregate]") {
    // If you need to change ctl options for this test, do it here:
    ctl = Control(lib, {"0", "--trans-ext", "no"});
    {
        auto tmp = parse("{a;b;c}. :- 2 {a;b;c} 2.");
        ctl.parse_files({tmp.path().c_str()});
    }
    {
        auto mcb = MCB(models);
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == MV{{}, {"a"}, {"a", "b", "c"}, {"b"}, {"c"}});
}

TEST_CASE_METHOD(Fixture, "write_aspif disjunction", "[cxx][write_aspif][disjunction]") {
    {
        auto tmp = parse("a | b | c. a :- b. b :- a.");
        ctl.parse_files({tmp.path().c_str()});
    }
    {
        auto mcb = MCB(models);
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == MV{{"a", "b"}, {"c"}});
}

TEST_CASE_METHOD(Fixture, "write_aspif minimize", "[cxx][write_aspif][minimize]") {
    {
        auto tmp = parse("#minimize { 1:a; 2:b; 3:c }. 1 {a; b; c}. :- a, not b, not c. :- b, not a, not c.");
        ctl.parse_files({tmp.path().c_str()});
    }
    {
        auto mcb = MCB(models);
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == MV{{"a", "b"}, {"c"}});
}

TEST_CASE_METHOD(Fixture, "write_aspif project", "[cxx][write_aspif][project]") {
    ctl.config()["solve"]["project"] = "1";
    {
        auto tmp = parse("1 {a; b; c}. #show a/0. #project a/0. #project b/0.");
        ctl.parse_files({tmp.path().c_str()});
    }
    {
        auto mcb = MCB(models);
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == MV{{}, {}, {"a"}, {"a"}});
}

TEST_CASE_METHOD(Fixture, "write_aspif output", "[cxx][write_aspif][output]") {
    bool symbols = GENERATE(false, true);
    {
        auto tmp = parse("1 {a; x}. #show a/0. #show b : x. #show c : a, x.", symbols);
        ctl.parse_files({tmp.path().c_str()});
    }
    {
        auto mcb = MCB(models);
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == MV{{"a"}, {"a", "b", "c"}, {"b"}});
}

TEST_CASE_METHOD(Fixture, "write_aspif external", "[cxx][write_aspif][external]") {
    {
        auto tmp = parse("#external a. [true] #external b. [false] #external c. [free]");
        ctl.parse_files({tmp.path().c_str()});
    }
    {
        auto mcb = MCB(models);
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == MV{{"a"}, {"a", "c"}});
}

TEST_CASE_METHOD(Fixture, "write_aspif heuristic", "[cxx][write_aspif][heuristic]") {
    ctl.config()["solver"]["heuristic"] = "domain";
    ctl.config()["solve"]["models"] = "1";
    {
        auto tmp = parse("{a; b}.\n"
                         "#heuristic a. [1,true]\n"
                         "#heuristic b. [0,false]\n");
        ctl.parse_files({tmp.path().c_str()});
    }
    {
        auto mcb = MCB(models);
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == MV{{"a"}});
}

TEST_CASE_METHOD(Fixture, "write_aspif edge", "[cxx][write_aspif][edge]") {
    {
        auto tmp = parse("{a; b}.\n"
                         "#edge (a,b) : a.\n"
                         "#edge (b,a) : b.\n");
        ctl.parse_files({tmp.path().c_str()});
    }
    {
        auto mcb = MCB(models);
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == MV{{}, {"a"}, {"b"}});
}

TEST_CASE_METHOD(Fixture, "write_aspif theory", "[cxx][write_aspif][theory]") {
    {
        auto tmp = parse("#theory p {\n"
                         "p { + : 0, binary, left };\n"
                         "&p/0: p, {<}, p, any\n"
                         "}.\n"
                         "&p{ 1,f(1+2),[1],{2},(3,),(4) }.\n"
                         "&p{} < 2.\n");
        ctl.parse_files({tmp.path().c_str()});
    }
    auto atoms = std::vector<std::string>{};
    for (auto const &atom : ctl.base().theory()) {
        atoms.emplace_back(atom.to_string());
    }
    std::ranges::sort(atoms);
    REQUIRE(atoms == std::vector<std::string>{"&p { 1,f((1+2)),[1],{2},(3),4 }", "&p { } < 2"});
}

} // namespace Clingo::Test
