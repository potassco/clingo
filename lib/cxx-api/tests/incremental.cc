#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

#include "cbs.hh"

namespace Clingo::Test {

namespace {

struct Fixture {
    Library lib;
    Control ctl{lib};
    Base bse = ctl.base();
};

} // namespace

TEST_CASE_METHOD(Fixture, "incremental", "[cxx][incremental][simplify]") {
    ctl.parse_string(R"(
            d(1..3).
            p(X) :- not q(X), d(X), X!=1.
            q(X) :- not p(X), d(X), X!=3.)");
    ctl.ground();
    auto sp2 = parse_term(lib, "p(2)");
    auto lq1 = bse.get(parse_term(lib, "q(1)"))->literal();
    auto lq2 = bse.get(parse_term(lib, "q(2)"))->literal();
    auto lp2 = bse.get(sp2)->literal();
    auto lp3 = bse.get(parse_term(lib, "p(3)"))->literal();
    REQUIRE(!bse.is_fact(lq2));
    REQUIRE(!bse.is_fact(lp2));
    REQUIRE(bse.is_fact(lq1) != bse.is_fact(lp3));
    REQUIRE(ctl.solve().get().satisfiable());
    REQUIRE(bse.is_fact(lq1));
    REQUIRE(bse.is_fact(lp3));
    REQUIRE(!bse.is_fact(lq2));
    REQUIRE(!bse.is_fact(lp2));
    ctl.parse_string("#program x. :- p(2).");
    ctl.ground({{"x", {}}});
    REQUIRE(ctl.solve().get().satisfiable());
    auto bp = bse.get(std::tuple{"p", 1});
    auto bq = bse.get(std::tuple{"q", 1});
    REQUIRE(bp->size() == 1);
    REQUIRE(bq->size() == 2);
    REQUIRE(!bse.contains(sp2));
}

TEST_CASE_METHOD(Fixture, "incremental minimize", "[cxx][incremental][simplify][minimize]") {
    auto solve_cfg = ctl.config()["solve"];
    solve_cfg["opt_mode"].value("optN");
    solve_cfg["models"].value("0");
    ctl.parse_string(R"(
        {a; b}.
        #minimize { 1: a; 2 : not b }.)");
    ctl.ground();
    auto models = MV{};
    {
        auto mcb = MCB{models};
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == MV{{"b"}});
    REQUIRE(ctl.stats()["summary"]["costs"][0].value() == 0.0);
    ctl.parse_string("#program x. :- not a. :- not b.");
    ctl.ground({{"x", {}}});
    {
        auto mcb = MCB{models};
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == MV{{"a", "b"}});
    REQUIRE(ctl.stats()["summary"]["costs"][0].value() == 1.0);
    ctl.parse_string("#program y. {c}. #minimize{ 1: b; 2 : not c }.");
    ctl.ground({{"y", {}}});
    {
        auto mcb = MCB{models};
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == MV{{"a", "b", "c"}});
    REQUIRE(ctl.stats()["summary"]["costs"][0].value() == 1.0);
}

} // namespace Clingo::Test
