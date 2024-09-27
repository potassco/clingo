#include <gringo/grounder/grounder.hh>

#include <gringo/output/text.hh>

#include <gringo/util/algorithm.hh>

#include <catch2/catch_test_macros.hpp>

namespace Gringo::Test {

TEST_CASE("grounder_text") {
    auto opts = Input::RewriteOptions{};
    auto log = Gringo::Logger{};
    log.set_level(LogLevel::error);
    auto store = Gringo::make_symbol_store(true, false);
    auto buf = Util::OutputBuffer{};
    auto out = Gringo::Output::make_text_output(buf);
    Gringo::Grounder grd{log, *store, opts, *out};
    auto params = Input::ProgramParamVec{{store->string("base"), {}}};

    SECTION("fact") {
        grd.parse("a.");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "a.\n");
    }
    SECTION("basic") {
        grd.parse("a. b :- a.");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "a.\n"
                              "b.\n");
    }
    SECTION("normal") {
        grd.parse("a :- not b. b :- not a.");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "b :- not a.\n"
                              "a :- not b.\n");
    }
    SECTION("condlit_strat") {
        grd.parse("a :- not b. b :- not a. c :- a : b.");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "b :- not a.\n"
                              "a :- not b.\n"
                              "c :- #false: b, not a.\n");
    }
    SECTION("condlit_rec") {
        grd.parse("b :- b : a. a :- a : b.");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "b :- b: a.\n"
                              "a :- a: b.\n");
        auto res = store->gc();
        REQUIRE(res.first == 4);
        REQUIRE(res.second == 0);
    }
    SECTION("condlit_rec") {
        grd.parse("p(1..3). q(3..5). r(X) :- p(X); q(X).");
        grd.prepare();
        REQUIRE(grd.ground(params));
        auto res = store->gc();
        REQUIRE(res.first == 15);
        REQUIRE(res.second == 2);
    }
    SECTION("condlit_bug") {
        grd.parse(R"(
            a.
            a :- d.

            b :- a.
            { c } :- b.

            d :- ID=1; c: X=ID+0.)");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "a.\n"
                              "b.\n"
                              "{ c }.\n"
                              "d :- c: .\n");
    }
    SECTION("disjunction") {
        grd.parse(R"(
            { q(1..2) }.
            { p(1..3) }.

            a(2,2).
            p(2).

            a(X,Y) : p(Y) :- q(X).

            b(X,Y) :- a(X,Y).

            a(X) : X>10 :- q(X).)");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "a(2,2).\n"
                              "p(2).\n"
                              "{ q(1) }.\n"
                              "{ q(2) }.\n"
                              "{ p(1) }.\n"
                              "{ p(3) }.\n"
                              "a(1,2); a(1,1): p(1); a(1,3): p(3) :- q(1).\n"
                              "#true :- q(2).\n"
                              "b(2,2).\n"
                              "b(1,2) :- a(1,2).\n"
                              "b(1,1) :- a(1,1).\n"
                              "b(1,3) :- a(1,3).\n"
                              "#false :- q(1).\n"
                              "#false :- q(2).\n");
    }
    SECTION("hd_aggr") {
        grd.parse(R"(
            c(1..3).
            {a(X) : c(X)} >= 2.
            {b(X) : c(X)} >= 4.

            aa(X) :- a(X).
            bb(X) :- b(X).)");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "c(1).\n"
                              "c(2).\n"
                              "c(3).\n"
                              "#sum+ { 1,0,a(1): a(1); 1,0,a(2): a(2); 1,0,a(3): a(3) } >= 2.\n"
                              "#sum+ { 1,0,b(1): b(1); 1,0,b(2): b(2); 1,0,b(3): b(3) } >= 4.\n"
                              "aa(1) :- a(1).\n"
                              "aa(2) :- a(2).\n"
                              "aa(3) :- a(3).\n");
    }
    SECTION("bd_aggr") {
        grd.parse(R"(
            d(1..5).
            { p(X) : d(X) }.
            a(X) :- d(X), 3 <= #sum { 1,Y: p(Y), Y < X } <= 4.)");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "d(1).\n"
                              "d(2).\n"
                              "d(3).\n"
                              "d(4).\n"
                              "d(5).\n"
                              "{ p(1) }.\n"
                              "{ p(2) }.\n"
                              "{ p(3) }.\n"
                              "{ p(4) }.\n"
                              "{ p(5) }.\n"
                              "a(4) :- 3 <= #sum+ { 1,1: p(1); 1,2: p(2); 1,3: p(3) } <= 4.\n"
                              "a(5) :- 3 <= #sum+ { 1,1: p(1); 1,2: p(2); 1,3: p(3); 1,4: p(4) } <= 4.\n");
    }
    SECTION("bd_aggr") {
        grd.parse(R"(
            d(1..5).
            m(3).
            a(X) :- d(X), #sum { 1,Y: d(Y), Y < X } >= B, m(B).
            b(X) :- d(X), not #sum { 1,Y: d(Y), Y < X } < 3.
            c(X) :- d(X), not not #sum { 1,Y: d(Y), Y < X } >= 3.)");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "m(3).\n"
                              "d(1).\n"
                              "d(2).\n"
                              "d(3).\n"
                              "d(4).\n"
                              "d(5).\n"
                              "a(4).\n"
                              "a(5).\n"
                              "b(4).\n"
                              "b(5).\n"
                              "c(4).\n"
                              "c(5).\n");
    }
    SECTION("aggr_rec") {
        grd.parse(R"(
            d(1..2).
            s(1..2).
            a(1,0).
            { a(X,T) : d(X) } >= 1 :- { a(X,T-1) : d(X) } >= 1, s(T).)");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "a(1,0).\n"
                              "d(1).\n"
                              "d(2).\n"
                              "s(1).\n"
                              "s(2).\n"
                              "#sum+ { 1,0,a(1,1): a(1,1); 1,0,a(2,1): a(2,1) } >= 1.\n"
                              "#sum+ { 1,0,a(1,2): a(1,2); 1,0,a(2,2): a(2,2) } >= 1"
                              " :- #sum+ { 1,0,a(1,1): a(1,1); 1,0,a(2,1): a(2,1) } >= 1.\n");
    }
    store->gc();
}

} // namespace Gringo::Test
