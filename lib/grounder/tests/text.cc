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
    SECTION("aggr_rec") {
        grd.parse(R"(
            company(c1;c2;c3;c4).
            owns(c1,c2,60;c1,c3,20;c2,c3,35;c3,c4,51).

            controls(X,Y) :- #sum+ {
                S: owns(X,Y,S);
                S,Z: controls(X,Z), owns(Z,Y,S) } > 50, company(X), company(Y), X!=Y.
            )");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "company(c1).\n"
                              "company(c2).\n"
                              "company(c3).\n"
                              "company(c4).\n"
                              "owns(c1,c2,60).\n"
                              "owns(c1,c3,20).\n"
                              "owns(c2,c3,35).\n"
                              "owns(c3,c4,51).\n"
                              "controls(c1,c2).\n"
                              "controls(c3,c4).\n"
                              "controls(c1,c3).\n"
                              "controls(c1,c4).\n");
    }
    SECTION("aggr_rec") {
        grd.parse(R"(
            node(a;b;c;d;e).
            edge(a,c).
            edge(b,c;b,e).
            edge(c,d;c,e).

            reach(V) :- node(V), not edge(*,V).
            reach(V) :- node(V), #count { U: reach(U), edge(U,V) } >= 2.
            )");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "edge(a,c).\n"
                              "node(a).\n"
                              "node(b).\n"
                              "node(c).\n"
                              "node(d).\n"
                              "node(e).\n"
                              "edge(b,c).\n"
                              "edge(b,e).\n"
                              "edge(c,d).\n"
                              "edge(c,e).\n"
                              "reach(a).\n"
                              "reach(b).\n"
                              "reach(c).\n"
                              "reach(e).\n");
    }
    SECTION("rec") {
        grd.parse(R"(
            #const n = 10.

            % initial prime
            prime(1,2).

            % mark prime and all its multiples as sieved
            sieve(I,X) :- prime(I,X).
            sieve(I,Z) :- prime(I,X), sieve(I,Y), Z=X+Y<=n.

            % mark numbers not sieved by prime
            % (ignoring numbers smaller than the prime)
            nsieve(I,Z) :- prime(I,X), sieve(I,Y), Z=Y+(1..X-1)<=n.

            % mark all primes sieved so far
            % (ignoring numbers smaller than the prime)
            rec_sieve(I,X) :- sieve(I,X).
            rec_sieve(I,Y) :- prime(I,X), rec_sieve(I-1,Y), Y>=X.

            % mark all primes not sieved so far
            rec_nsieve(1,X) :- nsieve(1,X).
            rec_nsieve(I,X) :- nsieve(I,X), rec_nsieve(I-1,X).

            % mark the consecutive prefix that has been sieved
            sieved_prefix(I,X) :- prime(I,X).
            sieved_prefix(I,X) :- sieved_prefix(I,X-1), rec_sieve(I,X).

            % mark the first non sieved number as prime
            prime(I+1,X) :- sieved_prefix(I,X-1), rec_nsieve(I,X).)");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view().find("prime(1,2).") != std::string_view::npos);
        REQUIRE(buf.view().find("prime(2,3).") != std::string_view::npos);
        REQUIRE(buf.view().find("prime(3,5).") != std::string_view::npos);
        REQUIRE(buf.view().find("prime(4,7).") != std::string_view::npos);
        REQUIRE(buf.view().find("prime(5") == std::string_view::npos);
    }
    SECTION("rec") {
        grd.parse(R"(
            char_to_digit(X,X) :- X=0..9.

            digit(Y-1,Y,D) :- char(C,Y), char_to_digit(C,D), Y > 0.

            pos(A) :- digit(X,Y,A).

            sign(X, 1) :- char(p,X).
            sign(X,-1) :- char(m,X).

            num_1(X,Y,A)      :- digit(X,Y,A), not pos(X).
            num_1(X,Z,10*A+B) :- num_1(X,Y,A), digit(Y,Z,B).
            num(X,Y,A)        :- num_1(X,Y,A), not pos(Y+1).

            par_expr(Y-1,Z+1,A) :- char(o,Y), expr(Y,Z,A), char(c,Z+1), Y >= 0.

            sign_expr(Y-1,Z,A*S) :- par_expr(Y,Z,A), sign(Y,S), Y > 0.
            sign_expr(Y-1,Z,A*S) :- num(Y,Z,A),      sign(Y,S), Y > 0.

            expr(0,Y,A)   :- num(0,Y,A).
            expr(X,Y,A)   :- char(o,X), num(X,Y,A).
            expr(X,Y,A)   :- par_expr(X,Y,A).
            expr(X,Y,A)   :- sign_expr(X,Y,A).
            expr(X,Z,A+B) :- expr(X,Y,A), sign_expr(Y,Z,B).

            cmp(Y-1,A)   :- char(g,Y), num(Y,N,A), Y > 0.
            cmp(Y-1,A*S) :- char(g,Y), sign(Y+1,S), num(Y+1,N,A), Y > 0.

            eq(A,A) :- expr(0,Y,A), cmp(Y,A).
            lt(A,B) :- expr(0,Y,A), cmp(Y,B), A < B.
            gt(A,B) :- expr(0,Y,A), cmp(Y,B), A > B.

            char(p,1). char(0,2). char(m,3). char(o,4). char(o,5). char(6,6).
            char(m,7). char(7,8). char(p,9). char(3,10). char(p,11). char(o,12).
            char(4,13). char(c,14). char(c,15). char(m,16). char(2,17). char(p,18).
            char(o,19). char(p,20). char(0,21). char(c,22). char(c,23). char(p,24).
            char(2,25). char(m,26). char(o,27). char(p,28). char(1,29). char(c,30).
            char(g,31). char(m,32). char(4,33).)");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view().find("cmp(30,-4).") != std::string_view::npos);
        REQUIRE(buf.view().find("cmp(") == buf.view().rfind("cmp("));
    }
    SECTION("assign") {
        grd.parse(R"(
            d(1..5).
            p(X) :- X = #sum {Y: d(Y)}.
            )");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "d(1).\n"
                              "d(2).\n"
                              "d(3).\n"
                              "d(4).\n"
                              "d(5).\n"
                              "p(15).\n");
    }
    SECTION("assign_rec") {
        grd.parse(R"(
            d(1..5).
            s(1..3).
            p(1..5,0).
            p(X,T) :- X = #sum {Y: p(Y,T-1); Z: d(Z)}, s(T).
            )");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "d(1).\n"
                              "d(2).\n"
                              "d(3).\n"
                              "d(4).\n"
                              "d(5).\n"
                              "s(1).\n"
                              "s(2).\n"
                              "s(3).\n"
                              "p(1,0).\n"
                              "p(2,0).\n"
                              "p(3,0).\n"
                              "p(4,0).\n"
                              "p(5,0).\n"
                              "p(15,1) :- #sum { 1; 2; 3; 4; 5 } = 15.\n"
                              "p(15,2) :- #sum { 1; 2; 3; 4; 5; 15: p(15,1) } = 15.\n"
                              "p(15,3) :- #sum { 1; 2; 3; 4; 5; 15: p(15,2); 30: p(30,2) } = 15.\n"
                              "p(30,2) :- #sum { 1; 2; 3; 4; 5; 15: p(15,1) } = 30.\n"
                              "p(30,3) :- #sum { 1; 2; 3; 4; 5; 15: p(15,2); 30: p(30,2) } = 30.\n"
                              "p(45,3) :- #sum { 1; 2; 3; 4; 5; 15: p(15,2); 30: p(30,2) } = 45.\n"
                              "p(60,3) :- #sum { 1; 2; 3; 4; 5; 15: p(15,2); 30: p(30,2) } = 60.\n");
    }
    SECTION("bd_theory_elems") {
        grd.parse(R"(
            #theory t {
              t {
                + : 1, binary, left;
                - : 2, unary
              };
              &p/0: t, {<}, t, any

            }.
            c.
            d(1..3).
            a :- &p { -X+1: d(X); 1: c; : c; 1 : a; : a } < 5.
            )");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "c.\n"
                              "d(1).\n"
                              "d(2).\n"
                              "d(3).\n"
                              "a :- &p { ((-1)+1); ((-2)+1); ((-3)+1); 1; 1: a; : ; : a } < 5.\n");
    }
    SECTION("bd_theory_sign") {
        grd.parse(R"(
            #theory t {
              t {
                + : 1, binary, left;
                - : 2, unary
              };
              &p/0: t, {<}, t, any

            }.
            a :- &p.
            b :- not &p.
            c :- not not &p.
            )");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "a :- &p.\n"
                              "b :- not &p.\n"
                              "c :- not not &p.\n");
    }
    SECTION("bd_theory_rec") {
        grd.parse(R"(
            #theory t {
              t {
                + : 1, binary, left;
                - : 2, unary
              };
              &p/0: t, {<}, t, any

            }.
            d(1..3).
            c.
            b(X) :- a(X).
            a(X) :- &p { -X+1: b(X); X: c } < 5, d(X).
            )");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "c.\n"
                              "d(1).\n"
                              "d(2).\n"
                              "d(3).\n"
                              "b(1) :- a(1).\n"
                              "b(2) :- a(2).\n"
                              "b(3) :- a(3).\n"
                              "a(1) :- &p { ((-1)+1): b(1); 1 } < 5.\n"
                              "a(2) :- &p { ((-2)+1): b(2); 2 } < 5.\n"
                              "a(3) :- &p { ((-3)+1): b(3); 3 } < 5.\n");
    }
    SECTION("hd_theory_elems") {
        grd.parse(R"(
            #theory t {
              t {
                + : 1, binary, left;
                - : 2, unary
              };
              &p/0: t, {<}, t, any

            }.
            c.
            d(1..3).
            &p { -X+1: d(X); 1: c; : c } < 5.
            )");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "c.\n"
                              "d(1).\n"
                              "d(2).\n"
                              "d(3).\n"
                              "&p { ((-1)+1); ((-2)+1); ((-3)+1); 1; :  } < 5.\n");
    }
    SECTION("hd_theory_cond") {
        grd.parse(R"(
            #theory t {
              t {
                + : 1, binary, left;
                - : 2, unary
              };
              &p/0: t, {<}, t, any

            }.
            c.
            { a(1..3) }.
            &p { -X+1: a(X); X: c } < 5 :- a(X).
            )");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(buf.view() == "c.\n"
                              "{ a(1) }.\n"
                              "{ a(2) }.\n"
                              "{ a(3) }.\n"
                              "&p { ((-1)+1): a(1); 1 } < 5 :- a(1).\n"
                              "&p { ((-2)+1): a(2); 2 } < 5 :- a(2).\n"
                              "&p { ((-3)+1): a(3); 3 } < 5 :- a(3).\n");
    }
    store->gc();
}

} // namespace Gringo::Test
