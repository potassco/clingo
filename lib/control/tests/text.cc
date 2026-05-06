#include <clingo/control/grounder.hh>

#include <clingo/output/text.hh>

#include <clingo/util/algorithm.hh>

#include <catch2/catch_test_macros.hpp>

#include <ranges>

namespace CppClingo::Test {

TEST_CASE("grounder_text") {
    auto store = make_symbol_store(true, false);
    SECTION("ground") {
        auto opts = Input::RewriteOptions{};
        auto log = Logger{};
        log.set_level(LogLevel::error);
        auto buf = Util::OutputBuffer{};
        auto out = Output::make_text_output(buf);
        auto grd = Control::Grounder{log, *store, opts, *out};
        auto params = Input::ProgramParamVec{{store->string("base"), {}}};
        SECTION("fact") {
            grd.parse("#show. a.");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "a.\n#show.\n");
        }
        SECTION("basic") {
            grd.parse("#show. a. b :- a.");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "a.\n"
                                  "b.\n#show.\n");
        }
        SECTION("bug-min") {
            grd.parse(R"(
                edge(1,2).
                edge(2,3).

                dom(1,2).
                dom(2,3).
                dom(1,3).

                dist(X,X,0) :- X=1..3.
                dist(X,Y,M+1) :- dom(X,Y), M = #min{N: dist(X,Z,N), edge(Z,Y)}, M != #sup.)");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "edge(1,2).\n"
                                  "edge(2,3).\n"
                                  "dom(1,2).\n"
                                  "dom(2,3).\n"
                                  "dom(1,3).\n"
                                  "dist(1,1,0).\n"
                                  "dist(2,2,0).\n"
                                  "dist(3,3,0).\n"
                                  "dist(1,2,1) :- #min { 0 } = 0.\n"
                                  "dist(2,3,1) :- #min { 0 } = 0.\n"
                                  "dist(1,3,2) :- #min { 1: dist(1,2,1) } = 1.\n"
                                  "#show edge/2.\n"
                                  "#show dom/2.\n"
                                  "#show dist/3.\n"
                                  "#show.\n");
        }
        SECTION("normal") {
            grd.parse("#show. a :- not b. b :- not a.");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "b :- not a.\n"
                                  "a :- not b.\n"
                                  "#show.\n");
        }
        SECTION("condlit_strat") {
            grd.parse("#show. a :- not b. b :- not a. c :- a : b.");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "b :- not a.\n"
                                  "a :- not b.\n"
                                  "c :- #false: b, not a.\n"
                                  "#show.\n");
        }
        SECTION("condlit_rec") {
            grd.parse("#show. b :- b : a. a :- a : b.");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "b :- b: a.\n"
                                  "a :- a: b.\n"
                                  "#show.\n");
            auto res = store->gc();
            REQUIRE(std::get<0>(res) == 1);
            REQUIRE(std::get<1>(res) == 4);
            REQUIRE(std::get<2>(res) == 0);
        }
        SECTION("condlit_rec") {
            grd.parse("#show. p(1..3). q(3..5). r(X) :- p(X); q(X).");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            auto res = store->gc();
            REQUIRE(std::get<0>(res) == 1);
            REQUIRE(std::get<1>(res) == 16);
            REQUIRE(std::get<2>(res) == 2);
        }
        SECTION("condlit_bug") {
            grd.parse(R"(
                #show.
                a.
                a :- d.

                b :- a.
                { c } :- b.

                d :- ID=1; c: X=ID+0.)");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "a.\n"
                                  "b.\n"
                                  "{ c }.\n"
                                  "d :- c: .\n"
                                  "#show.\n");
        }
        SECTION("disjunction") {
            grd.parse(R"(
                #show.
                { q(1..2) }.
                { p(1..3) }.

                a(2,2).
                p(2).

                a(X,Y) : p(Y) :- q(X).

                b(X,Y) :- a(X,Y).

                a(X) : X>10 :- q(X).)");
            REQUIRE(grd.ground(params) == GroundResult::ok);
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
                                  "#false :- q(2).\n"
                                  "#show.\n");
        }
        SECTION("hd_aggr") {
            grd.parse(R"(
                #show.
                c(1..3).
                {a(X) : c(X)} >= 2.
                {b(X) : c(X)} >= 4.

                aa(X) :- a(X).
                bb(X) :- b(X).)");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "c(1).\n"
                                  "c(2).\n"
                                  "c(3).\n"
                                  "#sum+ { 1,0,a(1): a(1); 1,0,a(2): a(2); 1,0,a(3): a(3) } >= 2.\n"
                                  "#sum+ { 1,0,b(1): b(1); 1,0,b(2): b(2); 1,0,b(3): b(3) } >= 4.\n"
                                  "aa(1) :- a(1).\n"
                                  "aa(2) :- a(2).\n"
                                  "aa(3) :- a(3).\n"
                                  "#show.\n");
        }
        SECTION("bd_aggr") {
            grd.parse(R"(
                #show.
                d(1..5).
                { p(X) : d(X) }.
                a(X) :- d(X), 3 <= #sum { 1,Y: p(Y), Y < X } <= 4.)");
            REQUIRE(grd.ground(params) == GroundResult::ok);
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
                                  "a(5) :- 3 <= #sum+ { 1,1: p(1); 1,2: p(2); 1,3: p(3); 1,4: p(4) } <= 4.\n"
                                  "#show.\n");
        }
        SECTION("bd_aggr") {
            grd.parse(R"(
                #show.
                d(1..5).
                m(3).
                a(X) :- d(X), #sum { 1,Y: d(Y), Y < X } >= B, m(B).
                b(X) :- d(X), not #sum { 1,Y: d(Y), Y < X } < 3.
                c(X) :- d(X), not not #sum { 1,Y: d(Y), Y < X } >= 3.)");
            REQUIRE(grd.ground(params) == GroundResult::ok);
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
                                  "c(5).\n"
                                  "#show.\n");
        }
        SECTION("aggr_rec") {
            grd.parse(R"(
                #show.
                d(1..2).
                s(1..2).
                a(1,0).
                { a(X,T) : d(X) } >= 1 :- { a(X,T-1) : d(X) } >= 1, s(T).)");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "a(1,0).\n"
                                  "d(1).\n"
                                  "d(2).\n"
                                  "s(1).\n"
                                  "s(2).\n"
                                  "#sum+ { 1,0,a(1,1): a(1,1); 1,0,a(2,1): a(2,1) } >= 1.\n"
                                  "#sum+ { 1,0,a(1,2): a(1,2); 1,0,a(2,2): a(2,2) } >= 1"
                                  " :- #sum+ { 1,0,a(1,1): a(1,1); 1,0,a(2,1): a(2,1) } >= 1.\n"
                                  "#show.\n");
        }
        SECTION("aggr_rec") {
            grd.parse(R"(
                #show.
                company(c1;c2;c3;c4).
                owns(c1,c2,60;c1,c3,20;c2,c3,35;c3,c4,51).

                controls(X,Y) :- #sum+ {
                    S: owns(X,Y,S);
                    S,Z: controls(X,Z), owns(Z,Y,S) } > 50, company(X), company(Y), X!=Y.
                )");
            REQUIRE(grd.ground(params) == GroundResult::ok);
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
                                  "controls(c1,c4).\n"
                                  "#show.\n");
        }
        SECTION("aggr_rec") {
            grd.parse(R"(
                #show.
                node(a;b;c;d;e).
                edge(a,c).
                edge(b,c;b,e).
                edge(c,d;c,e).

                reach(V) :- node(V), not edge(*,V).
                reach(V) :- node(V), #count { U: reach(U), edge(U,V) } >= 2.
                )");
            REQUIRE(grd.ground(params) == GroundResult::ok);
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
                                  "reach(e).\n"
                                  "#show.\n");
        }
        SECTION("rec") {
            grd.parse(R"(
                #show.
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
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view().find("prime(1,2).") != std::string_view::npos);
            REQUIRE(buf.view().find("prime(2,3).") != std::string_view::npos);
            REQUIRE(buf.view().find("prime(3,5).") != std::string_view::npos);
            REQUIRE(buf.view().find("prime(4,7).") != std::string_view::npos);
            REQUIRE(buf.view().find("prime(5") == std::string_view::npos);
        }
        SECTION("rec") {
            grd.parse(R"(
                #show.
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
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view().find("cmp(30,-4).") != std::string_view::npos);
            REQUIRE(buf.view().find("cmp(") == buf.view().rfind("cmp("));
        }
        SECTION("assign") {
            grd.parse(R"(
                #show.
                d(1..5).
                p(X) :- X = #sum {Y: d(Y)}.
                )");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "d(1).\n"
                                  "d(2).\n"
                                  "d(3).\n"
                                  "d(4).\n"
                                  "d(5).\n"
                                  "p(15).\n"
                                  "#show.\n");
        }
        SECTION("assign_rec") {
            grd.parse(R"(
                #show.
                d(1..5).
                s(1..3).
                p(1..5,0).
                p(X,T) :- X = #sum {Y: p(Y,T-1); Z: d(Z)}, s(T).
                )");
            REQUIRE(grd.ground(params) == GroundResult::ok);
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
                                  "p(60,3) :- #sum { 1; 2; 3; 4; 5; 15: p(15,2); 30: p(30,2) } = 60.\n"
                                  "#show.\n");
        }
        SECTION("bd_theory_elems") {
            grd.parse(R"(
                #show.
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
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "c.\n"
                                  "d(1).\n"
                                  "d(2).\n"
                                  "d(3).\n"
                                  "a :- &p { ((-1)+1); ((-2)+1); ((-3)+1); 1; 1: a; : ; : a } < 5.\n"
                                  "#show.\n");
        }
        SECTION("bd_theory_sign") {
            grd.parse(R"(
                #show.
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
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "a :- &p.\n"
                                  "b :- not &p.\n"
                                  "c :- not not &p.\n"
                                  "#show.\n");
        }
        SECTION("bd_theory_rec") {
            grd.parse(R"(
                #show.
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
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "c.\n"
                                  "d(1).\n"
                                  "d(2).\n"
                                  "d(3).\n"
                                  "b(1) :- a(1).\n"
                                  "b(2) :- a(2).\n"
                                  "b(3) :- a(3).\n"
                                  "a(1) :- &p { ((-1)+1): b(1); 1 } < 5.\n"
                                  "a(2) :- &p { ((-2)+1): b(2); 2 } < 5.\n"
                                  "a(3) :- &p { ((-3)+1): b(3); 3 } < 5.\n"
                                  "#show.\n");
        }
        SECTION("hd_theory_elems") {
            grd.parse(R"(
                #show.
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
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "c.\n"
                                  "d(1).\n"
                                  "d(2).\n"
                                  "d(3).\n"
                                  "&p { ((-1)+1); ((-2)+1); ((-3)+1); 1; :  } < 5.\n"
                                  "#show.\n");
        }
        SECTION("hd_theory_cond") {
            grd.parse(R"(
                #show.
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
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "c.\n"
                                  "{ a(1) }.\n"
                                  "{ a(2) }.\n"
                                  "{ a(3) }.\n"
                                  "&p { ((-1)+1): a(1); 1 } < 5 :- a(1).\n"
                                  "&p { ((-2)+1): a(2); 2 } < 5 :- a(2).\n"
                                  "&p { ((-3)+1): a(3); 3 } < 5 :- a(3).\n"
                                  "#show.\n");
        }
        SECTION("minimize") {
            grd.parse(R"(
                #show.
                {p(1..3)}.
                #minimize {X,p: p(X); a; 0 }.)");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "{ p(1) }.\n"
                                  "{ p(2) }.\n"
                                  "{ p(3) }.\n"
                                  " :~ p(1). [1,p].\n"
                                  " :~ p(2). [2,p].\n"
                                  " :~ p(3). [3,p].\n"
                                  " :~ . [0].\n"
                                  "#show.\n");
        }
        SECTION("heuristic") {
            grd.parse(R"(
                #show.
                {p(1,0,true)}.
                {p(2,0,level)}.
                p(3,1,init).
                { q(1..3) }.
                #heuristic q(W): p(W,P,T). [W@P,T])");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "p(3,1,init).\n"
                                  "{ p(1,0,true) }.\n"
                                  "{ p(2,0,level) }.\n"
                                  "{ q(1) }.\n"
                                  "{ q(2) }.\n"
                                  "{ q(3) }.\n"
                                  "#heuristic q(3). [3@1,init]\n"
                                  "#heuristic q(1): p(1,0,true). [1@0,true]\n"
                                  "#heuristic q(2): p(2,0,level). [2@0,level]\n"
                                  "#show.\n");
        }
        SECTION("edge") {
            grd.parse(R"(
                #show.
                p(1).
                {p(2..3)}.
                #edge (X,X+1): p(X).)");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "p(1).\n"
                                  "{ p(2) }.\n"
                                  "{ p(3) }.\n"
                                  "#edge (1,2).\n"
                                  "#edge (2,3): p(2).\n"
                                  "#edge (3,4): p(3).\n"
                                  "#show.\n");
        }
        SECTION("external") {
            grd.parse(R"(#show.
                         {a(1..2)}.
                         #external et(X) : a(X). [true]
                         #external ef(X) : a(X). [false]
                         #external ec(X) : a(X). [free]
                         #external en(X) : a(X).)");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "{ a(1) }.\n"
                                  "{ a(2) }.\n"
                                  "#external et(1). [true]\n"
                                  "#external et(2). [true]\n"
                                  "#external ef(1). [false]\n"
                                  "#external ef(2). [false]\n"
                                  "#external ec(1). [free]\n"
                                  "#external ec(2). [free]\n"
                                  "#external en(1). [false]\n"
                                  "#external en(2). [false]\n"
                                  "#show.\n");
        }
        SECTION("show") {
            grd.parse(R"(
                #show.
                {p(1..3)}.
                #show a.
                #show X: p(X).)");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "{ p(1) }.\n"
                                  "{ p(2) }.\n"
                                  "{ p(3) }.\n"
                                  "#show a.\n"
                                  "#show 1: p(1).\n"
                                  "#show 2: p(2).\n"
                                  "#show 3: p(3).\n"
                                  "#show.\n");
        }
        SECTION("show_sig") {
            grd.parse(R"(
                a.
                b.
                #show a/0.
                )");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "a.\n"
                                  "b.\n"
                                  "#show a/0.\n"
                                  "#show.\n");
        }
        SECTION("show_nothing") {
            grd.parse(R"(
                a.
                b.
                #show.
                )");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "a.\n"
                                  "b.\n"
                                  "#show.\n");
        }
        SECTION("bug-index-sequence") {
            grd.parse(R"(
                true(V) :- set(V).
                set(1) :- not true(2).
                set(2) :- not true(1).
                set(3) :- true(1).
                set(3) :- true(2).
                )");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "set(1) :- not true(2).\n"
                                  "set(2) :- not true(1).\n"
                                  "true(1) :- set(1).\n"
                                  "true(2) :- set(2).\n"
                                  "set(3) :- true(1).\n"
                                  "set(3) :- true(2).\n"
                                  "true(3) :- set(3).\n"
                                  "#show true/1.\n"
                                  "#show set/1.\n"
                                  "#show.\n");
        }
        SECTION("project") {
            grd.parse(R"(
                #show.
                {a; p(1..3); q(2..4)}.
                #project a.
                #project b.
                #project p(X): q(X).)");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "{ a }.\n"
                                  "{ p(1) }.\n"
                                  "{ p(2) }.\n"
                                  "{ p(3) }.\n"
                                  "{ q(2) }.\n"
                                  "{ q(3) }.\n"
                                  "{ q(4) }.\n"
                                  "#project a.\n"
                                  "#project p(2).\n"
                                  "#project p(3).\n"
                                  "#show.\n");
        }
        SECTION("simple_aggr") {
            grd.parse(R"(
                #show.
                p(Y) :- X=10, Y = #sum { X: X=11; Z: Z=2; Z: Z=3 }.)");
            REQUIRE(grd.ground(params) == GroundResult::ok);
            REQUIRE(buf.view() == "p(5).\n"
                                  "#show.\n");
        }
        SECTION("fstring") {
            using SV = std::vector<std::string>;
            auto ground = [&store](std::string_view prg) {
                auto opts = Input::RewriteOptions{};
                auto log = Logger{};
                log.set_level(LogLevel::error);
                auto buf = Util::OutputBuffer{};
                auto out = Output::make_text_output(buf);
                auto grd = Control::Grounder{log, *store, opts, *out};
                auto params = Input::ProgramParamVec{{store->string("base"), {}}};
                grd.parse(R"(
                    p(12345).
                    p(590438524098504958203485230458025823455).
                    x(p(f("x", y))).
                    x("xyz").
                    y(abc).
                    z(0x3042).
                    z(0x301).
                    z(0x41).
                )");
                grd.parse(prg);
                REQUIRE(grd.ground(params) == GroundResult::ok);
                auto res = SV{};
                for (auto line : buf.view() | std::views::split('\n') |
                                     std::views::filter([](auto sv) { return !sv.empty() && sv.front() == 'q'; })) {
                    res.emplace_back();
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    res.back().insert(res.back().end(), line.begin() + 3, line.end() - 3);
                }
                std::ranges::sort(res);
                return res;
            };
            // numbers
            REQUIRE(ground(R"(q(f"{X}") :- p(X).)") == SV{
                                                           "12345",
                                                           "590438524098504958203485230458025823455",
                                                       });
            REQUIRE(ground(R"(q(f"{X:b}") :- p(X).)") ==
                    SV{
                        "11000000111001",
                        "1101111000011001001010010110010011001000000110010111011000111101011001000000010101110111000010"
                        "00101111011111000000100110011011111",
                    });
            REQUIRE(ground(R"(q(f"{X:o}") :- p(X).)") == SV{
                                                             "30071",
                                                             "6741445131144031354365440127341057370046337",
                                                         });
            REQUIRE(ground(R"(q(f"{X:d}") :- p(X).)") == SV{
                                                             "12345",
                                                             "590438524098504958203485230458025823455",
                                                         });
            REQUIRE(ground(R"(q(f"{X:x}") :- p(X).)") == SV{
                                                             "1bc3252c99032ec7ac80aee117be04cdf",
                                                             "3039",
                                                         });
            REQUIRE(ground(R"(q(f"{X:X}") :- p(X).)") == SV{
                                                             "1BC3252C99032EC7AC80AEE117BE04CDF",
                                                             "3039",
                                                         });
            REQUIRE(ground(R"(q(f"{X:#x}") :- p(X).)") == SV{
                                                              "0x1bc3252c99032ec7ac80aee117be04cdf",
                                                              "0x3039",
                                                          });
            REQUIRE(ground(R"(q(f"{X:0=+55,}") :- p(X;-X).)") ==
                    SV{
                        "+00000000000000000000000000000000000000000000000012,345",
                        "+000590,438,524,098,504,958,203,485,230,458,025,823,455",
                        "-00000000000000000000000000000000000000000000000012,345",
                        "-000590,438,524,098,504,958,203,485,230,458,025,823,455",
                    });
            REQUIRE(ground(R"(q(f"{X:0= #55_x}") :- p(X;-X).)") ==
                    SV{
                        " 0x0000000000000000000000000000000000000000000000003039",
                        " 0x000000000001_bc32_52c9_9032_ec7a_c80a_ee11_7be0_4cdf",
                        "-0x0000000000000000000000000000000000000000000000003039",
                        "-0x000000000001_bc32_52c9_9032_ec7a_c80a_ee11_7be0_4cdf",
                    });
            // terms and strings
            REQUIRE(ground(R"(q(f"{X!s}") :- x(X).)") == SV{
                                                             "p(f(\\\"x\\\",y))",
                                                             "xyz",
                                                         });
            REQUIRE(ground(R"(q(f"{X!r}") :- x(X).)") == SV{
                                                             "\\\"xyz\\\"",
                                                             "p(f(\\\"x\\\",y))",
                                                         });
            // alignment
            REQUIRE(ground(R"(q(f"{X:.^7}") :- y(X).)") == SV{"..abc.."});
            REQUIRE(ground(R"(q(f"{X:.<7}") :- y(X).)") == SV{"abc...."});
            REQUIRE(ground(R"(q(f"{X:.>7}") :- y(X).)") == SV{"....abc"});
            REQUIRE(ground(R"(q(f"{X:.^8}") :- y(X).)") == SV{"..abc..."});
            // character
            REQUIRE(ground(R"(q(f"{X:c}") :- z(X).)") == SV{"A", "́", "あ"});
            // accessors
            REQUIRE(ground(R"(q(f"{X[0][0].name}") :- X=f(g(h(1))).)") == SV{"h"});
            // nested terms
            REQUIRE(ground(R"(q(f"x{f"{Y-1:0=5}":*^10}z") :- Y=0.)") == SV{"x**-0001***z"});
            // subsitution
            REQUIRE(ground(R"(q(f"{X}") :- X=1.)") == SV{"1"});
        }
    }
    // Note: the current implementation takes two iterations to clean up everything
    auto res = store->gc();
    REQUIRE(std::get<0>(res) == 0); // no owners
    res = store->gc();
    REQUIRE(std::get<0>(res) == 0); // no owners
    REQUIRE(std::get<1>(res) == 0); // no symbols
}

} // namespace CppClingo::Test
