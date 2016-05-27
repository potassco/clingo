// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#include "tests/tests.hh"
#include "tests/term_helper.hh"
#include "tests/output/solver_helper.hh"

namespace Gringo { namespace Output { namespace Test {

TEST_CASE("output-lparse", "[output]") {
    SECTION("unpool") {
        REQUIRE(
            "([[a(1),a(2),a(4)]],[])" ==
            IO::to_string(solve(
                "a(X) :- X=(1;2;4)."
            ))
        );
        REQUIRE(
            "([[p(1),p(2),q(3)],[p(1),p(2),q(4)]],[])" ==
            IO::to_string(solve(
                "1 {q(3;4)} 1."
                "p(1;2) :- q(3;4)."
            ))
        );
        REQUIRE(
            "([],[])" ==
            IO::to_string(solve(
                "1 {q(3;4)} 1."
                "#disjoint { (1;2) : (2;3) : q(3;4) }."
            ))
        );
        REQUIRE(
            "([[a,p(1),q(3)],[a,p(1),q(4)],[a,p(2),q(3)],[a,p(2),q(4)]],[])" ==
            IO::to_string(solve(
                "1 { p(1;2) } 1."
                "1 { q(3;4) } 1."
                "a :- p(1;2) : q(3;4)."
            ))
        );
        REQUIRE(
            "([[p(1),p(2),q(3)],[p(1),p(2),q(4)]],[])" ==
            IO::to_string(solve(
                "1 { q(3;4) } 1."
                "p(1;2) : q(3;4)."
            ))
        );
        REQUIRE(
            "([[a,p(1),q(3)],[a,p(1),q(4)],[a,p(2),q(3)],[a,p(2),q(4)],[q(3)],[q(4)]],[])" ==
            IO::to_string(solve(
                "1 { q(3;4) } 1."
                "0 { p(1;2) } 1."
                "a :- (1;2) { q(3;4) : p(1;2) }."
            ))
        );
        REQUIRE(
            "([[],[a,p(1)],[a,p(2)]],[])" ==
            IO::to_string(solve(
                "0 { p(1;2) } 1."
                "a :- (2;3) #count { q(3;4) : p(1;2) }."
            ))
        );
        REQUIRE(
            "([[p(1),q(3),q(4)],[p(2),q(3),q(4)]],[])" ==
            IO::to_string(solve(
                "0 { p(1;2) } 1."
                "(1;2) { q(3;4) : p(1;2) }."
            ))
        );
        REQUIRE(
            "([[p(1),q(3)],[p(1),q(3),q(4)],[p(1),q(4)],[p(2),q(3)],[p(2),q(3),q(4)],[p(2),q(4)]],[])" ==
            IO::to_string(solve(
                "0 { p(1;2) } 1."
                "(1;2) #count { t(5;6) : q(3;4) : p(1;2) }."
            ))
        );
    }

    SECTION("recCondBug") {
        REQUIRE(
            "([[0,2,4,5,9],[0,2,5,9],[0,4,9],[0,9]],[])" ==
            IO::to_string(solve(
                "holds(atom(A)) :- rule(lit(pos,atom(A)),B); holds(B).\n"
                "{ holds(atom(A)) : head_aggregate_element_set(I, head_aggregate_element(_,lit(pos,atom(A)),C))\n"
                "                 , holds(C)\n"
                "} :- rule(head_aggregate(left(#inf,less_equal),count,head_aggregate_element_set(I),right(less_equal,#sup)),C)\n"
                "   ; holds(C)\n"
                "   .\n"
                "% :- rule(lit(pos,false),B); holds(B) .\n"
                "% NOTE: there are more heads but this is enough to support --lparse-rewrite\n"
                "\n"
                "holds(conjunction(I)) :- conjunction(I)\n"
                "                       ;         holds(A) : conjunction(I,lit(pos,A))\n"
                "                       ; not     holds(A) : conjunction(I,lit(neg,A))\n"
                "                       ; not not holds(A) : conjunction(I,lit(neg_neg,A))\n"
                "                       .\n"
                "\n"
                "body_aggregate(L,F,S,U) :- conjunction(_,lit(_,body_aggregate(L,F,S,U))). % TODO: generate in meta\n"
                "\n"
                "holds(body_aggregate(left(#inf,less_equal),sump,body_aggregate_element_set(S),right(greater_equal,U)))\n"
                "    :- body_aggregate(left(#inf,less_equal),sump,body_aggregate_element_set(S),right(greater_equal,U))\n"
                "     ; #sum+ { W,T : body_aggregate_element_set(S,body_aggregate_element((W,T),conjunction(C))), holds(C) } >= U % TODO: if the holds is ommitted strange things appear to happen\n"
                "     .\n"
                "% NOTE: there are more body aggregates but this is enough to support --lparse-rewrite\n"
                "\n"
                "% TODO: handle minimize constraint\n"
                "\n"
                "#show.\n"
                "#show I : holds(conjunction(I)), conjunction(I).\n"
                "rule(head_aggregate(left(#inf,less_equal),count,head_aggregate_element_set(0),right(less_equal,#sup)),conjunction(0)).\n"
                "conjunction(0).\n"
                "head_aggregate_element_set(0).\n"
                "head_aggregate_element_set(0,head_aggregate_element((),lit(pos,atom(p(1))),conjunction(0))).\n"
                "head_aggregate_element_set(0,head_aggregate_element((),lit(pos,atom(p(2))),conjunction(0))).\n"
                "rule(lit(pos,atom(__aux(1))),conjunction(1)).\n"
                "conjunction(1).\n"
                "conjunction(1,lit(pos,atom(p(2)))).\n"
                "conjunction(1,lit(neg,atom(p(2)))).\n"
                "rule(lit(pos,atom(__aux(2))),conjunction(2)).\n"
                "conjunction(2).\n"
                "conjunction(2,lit(pos,atom(p(2)))).\n"
                "rule(lit(pos,atom(__aux(2))),conjunction(3)).\n"
                "conjunction(3).\n"
                "conjunction(3,lit(pos,atom(__aux(1)))).\n"
                "rule(lit(pos,atom(__aux(4))),conjunction(6)).\n"
                "conjunction(4).\n"
                "conjunction(4,lit(pos,atom(p(1)))).\n"
                "conjunction(5).\n"
                "conjunction(5,lit(pos,atom(__aux(2)))).\n"
                "body_aggregate_element_set(0).\n"
                "body_aggregate_element_set(0,body_aggregate_element((1,(0,())),conjunction(4))).\n"
                "body_aggregate_element_set(0,body_aggregate_element((2,(1,())),conjunction(5))).\n"
                "conjunction(6).\n"
                "conjunction(6,lit(pos,body_aggregate(left(#inf,less_equal),sump,body_aggregate_element_set(0),right(greater_equal,1)))).\n"
                "rule(lit(pos,atom(__aux(5))),conjunction(7)).\n"
                "conjunction(7).\n"
                "conjunction(7,lit(pos,body_aggregate(left(#inf,less_equal),sump,body_aggregate_element_set(0),right(greater_equal,3)))).\n"
                "rule(lit(pos,atom(__aux(3))),conjunction(8)).\n"
                "conjunction(8).\n"
                "conjunction(8,lit(pos,atom(__aux(4)))).\n"
                "conjunction(8,lit(neg,atom(__aux(5)))).\n"
                "rule(lit(pos,false),conjunction(9)).\n"
                "conjunction(9).\n"
                "conjunction(9,lit(neg,atom(__aux(3)))).\n"
                "show_atom(p(1)).\n"
                "show_atom(p(2)).\n"
            ))
        );
    }

    SECTION("empty") {
        REQUIRE("([[]],[])" == IO::to_string(solve("")));
    }

    SECTION("projectionBug") {
        REQUIRE("([[p(1),p(2)]],[])" == IO::to_string(solve(
            "q((1,x),2).\n"
            "p(A) :- q((A,_),_).\n"
            "p(B) :- q((A,_),B).\n"
            , {"p("})));
    }
    SECTION("aggregateBug") {
        REQUIRE(
            "([[a(1),a(2),b(1)],[a(1),a(2),b(1),b(2)]],[])" ==
            IO::to_string(solve(
            "a(1)."
            "a(2)."
            "b(1)."
            "{ b(X) } :- a(X).")));
    }

    SECTION("aggregateMinBug") {
        REQUIRE(
            "([[a(20),a(50),a(60),output(20)],[a(20),a(50),output(20)],[a(50),a(60),output(50)],[a(50),output(50)]],[])" ==
            IO::to_string(solve(
                "a(50)."
                "{ a(20) ; a(60) }."
                "output(X) :- X = #min { C : a(C) }."
            )));
    }

    SECTION("recHeadAggregateBug") {
        REQUIRE(
            "(["
            "[r(v(x13),e(r1,n(a11))),r(v(x13),e(r1,v(x3))),r(v(x13),e(r1,v(x7))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x7),n(a11)),r(v(x9),e(r1,n(a11))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x13),e(r1,v(x3))),r(v(x13),e(r1,v(x7))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x13),e(r1,v(x3))),r(v(x13),e(r1,v(x7))),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x13),e(r1,v(x3))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x7),n(a11)),r(v(x9),e(r1,n(a11))),r(v(x9),e(r1,v(x3)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x13),e(r1,v(x3))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x7),n(a11)),r(v(x9),e(r1,n(a11))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x13),e(r1,v(x3))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x13),e(r1,v(x3))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x13),e(r1,v(x3))),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x13),e(r1,v(x3))),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x7),n(a11)),r(v(x9),e(r1,n(a11))),r(v(x9),e(r1,v(x3)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x7),n(a11)),r(v(x9),e(r1,n(a11))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3)))],"
            "[r(v(x13),e(r1,n(a11))),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),e(r1,v(x3))),r(v(x13),e(r1,v(x7))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),e(r1,v(x3))),r(v(x13),e(r1,v(x7))),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),e(r1,v(x3))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x7),n(a11)),r(v(x9),e(r1,n(a11))),r(v(x9),e(r1,v(x3)))],"
            "[r(v(x13),e(r1,v(x3))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x7),n(a11)),r(v(x9),e(r1,n(a11))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),e(r1,v(x3))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3)))],"
            "[r(v(x13),e(r1,v(x3))),r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),e(r1,v(x3))),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3)))],"
            "[r(v(x13),e(r1,v(x3))),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x7),n(a11)),r(v(x9),e(r1,n(a11))),r(v(x9),e(r1,v(x3)))],"
            "[r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x7),n(a11)),r(v(x9),e(r1,n(a11))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3)))],"
            "[r(v(x13),n(a11)),r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))],"
            "[r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3)))],"
            "[r(v(x7),e(r1,n(a11))),r(v(x7),e(r1,v(x4))),r(v(x9),e(r1,v(x3))),r(v(x9),e(r1,v(x7)))]"
            "],[])" == IO::to_string(solve(
            "atom(A) :- hasatom(A,_,_).\n"
            "atom(A) :- hasatom(e(_,A),_,_).\n"
            "nonvatom(X) :- atom(X), X!=v(V):atom(v(V)).\n"
            "\n"
            "triviallyfalse(n(X),n(Y)) :- atom(n(X)), atom(n(Y)), X!=Y.\n"
            "triviallyfalse(e(R1,C1),e(R2,C2)) :- atom(e(R1,C1)),\n"
            "                                               atom(e(R2,C2)), R1!=R2. \n"
            "triviallyfalse(n(C),e(R,C1)) :- atom(n(C)), atom(e(R,C1)).\n"
            "triviallyfalse(e(R,C1),n(C)) :- atom(n(C)), atom(e(R,C1)).\n"
            "\n"
            "subs(X,Y) :- subs(e(R,X),e(R,Y)).\n"
            "\n"
            "1 { subs(Y,X):hasatom(Y,|P-1|,E),not triviallyfalse(Y,X) } :- hasatom(X,P,E), nonvatom(X).\n"
            "\n"
            "1 { subs(Y,C):hasatom(Y,|P-1|,E),not triviallyfalse(Y,C) } :- subs(X,C), hasatom(X,P,E),\n"
            "                  nonvatom(C), not hasatom(C,PX,E):hasatom(C,PX,E).\n"
            "\n"
            "subs(X,Z) :- subs(X,Y), subs(Y,Z), X!=Z, X!=Y, Z!=Y.\n"
            "\n"
            ":- subs(X,Y), triviallyfalse(X,Y).\n"
            "\n"
            "greater(X,Y) :- subs(v(X),e(R,v(Y))).\n"
            ":- greater(X,X).\n"
            "greater(X,Z) :- greater(X,Y), greater(Y,Z), X!=Y, X!=Z, Y!=Z.\n"
            "\n"
            "atom(C) :- diseq(C,D).\n"
            "atom(D) :- diseq(C,D).\n"
            "\n"
            "1 { dissubs(X,Y); dissubs(Y,X) } :- diseq(X,Y).\n"
            "ok(C,D) :- dissubs(C,D), subs(D,D'), nonvatom(D'), not subs(C,D'), not subs(C',D'):subs(C,C').\n"
            ":- dissubs(C,D), not ok(C,D).\n"
            "\n"
            "r(v(X),C) :- subs(v(X),C), relevant(X), nonvatom(C).\n"
            "\n"
            "%equation 1\n"
            "hasatom(v(x3), 0, 1).\n"
            "hasatom(e(r1, n(a11)), 1, 1).\n"
            "\n"
            "%equation 2\n"
            "hasatom(v(x9), 0, 2).\n"
            "hasatom(v(x5), 1, 2).\n"
            "hasatom(v(x9), 1, 2).\n"
            "\n"
            "%equation 3\n"
            "hasatom(v(x6), 0, 3).\n"
            "hasatom(v(x4), 1, 3).\n"
            "hasatom(v(x9), 1, 3).\n"
            "\n"
            "%equation 4\n"
            "hasatom(v(x1), 0, 4).\n"
            "hasatom(v(x1), 1, 4).\n"
            "hasatom(v(x9), 1, 4).\n"
            "\n"
            "%equation 5\n"
            "hasatom(v(x1), 0, 5).\n"
            "hasatom(e(r1, v(x7)), 1, 5).\n"
            "\n"
            "%equation 6\n"
            "hasatom(v(x4), 0, 6).\n"
            "hasatom(v(x3), 1, 6).\n"
            "hasatom(v(x13), 1, 6).\n"
            "\n"
            "%equation 7\n"
            "hasatom(v(x2), 0, 7).\n"
            "hasatom(e(r1, v(x4)), 1, 7).\n"
            "\n"
            "%equation 8\n"
            "hasatom(v(x7), 0, 8).\n"
            "hasatom(v(x2), 1, 8).\n"
            "hasatom(v(x7), 1, 8).\n"
            "\n"
            "%equation 9\n"
            "hasatom(v(x5), 0, 9).\n"
            "hasatom(e(r1, v(x3)), 1, 9).\n"
            "\n"
            "%disequation 1\n"
            "diseq(v(x6), v(x9)).\n"
            "\n"
            "relevant(x7).\n"
            "relevant(x9).\n"
            "relevant(x13).\n"
            , {"r("})));

    }

    SECTION("disjunctionBug") {
        REQUIRE(
            "([],[-:1:21-22: info: atom does not occur in any rule head:\n  d\n])" ==
            IO::to_string(solve(
                "b."
                "c :- b."
                "a :- c."
                "c : d :- a."
            )));
    }

    SECTION("disjunctionBug2") {
        REQUIRE(
            "([[b(0),h(0),h(1),p(0)]],[])" ==
            IO::to_string(solve(
                "p(0)."
                "h(0;1)."
                "b(0)."
                "p(X) : h(X) :- p(Y), b(Y)."
            )));
    }

    SECTION("aggregateNotNot") {
        REQUIRE(
            "([[p(2)],[p(2),p(3)],[p(2),p(3),p(4)],[p(2),p(3),p(4),p(5)],[p(2),p(3),p(5)],[p(2),p(4)],[p(2),p(4),p(5)],[p(2),p(5)],[p(4)],[p(4),p(5)]],[])" ==
            IO::to_string(solve(
            "{ p(1..5) }."
            ":- not not 2 != #min { X:p(X) } != 4.")));
        REQUIRE(
            "([[p(2)],[p(2),p(3)],[p(2),p(3),p(4)],[p(2),p(3),p(4),p(5)],[p(2),p(3),p(5)],[p(2),p(4)],[p(2),p(4),p(5)],[p(2),p(5)],[p(4)],[p(4),p(5)]],[])" ==
            IO::to_string(solve(
            "{ p(1..5) }."
            ":- 2 != #min { X:p(X) } != 4.")));
        REQUIRE(
            "([[p(2)],[p(2),p(3)],[p(2),p(3),p(4)],[p(2),p(3),p(4),p(5)],[p(2),p(3),p(5)],[p(2),p(4)],[p(2),p(4),p(5)],[p(2),p(5)],[p(4)],[p(4),p(5)]],[])" ==
            IO::to_string(solve(
            "{ p(1..5) }."
            "h :- not 2 != #min { X:p(X) } != 4."
            ":- not h.", {"p("})));
        REQUIRE(
            "([[p(2)],[p(2),p(3)],[p(2),p(3),p(4)],[p(2),p(3),p(4),p(5)],[p(2),p(3),p(5)],[p(2),p(4)],[p(2),p(4),p(5)],[p(2),p(5)],[p(4)],[p(4),p(5)]],[])" ==
            IO::to_string(solve(
            "{ p(1..5) }."
            ":- not 2 = #min { X:p(X) }, not #min { X:p(X) } = 4.")));
    }
    SECTION("aggregateRecBug") {
        REQUIRE(
            "([],[])" ==
            IO::to_string(solve(
            "a :- {a}!=1."
            )));
        REQUIRE(
            "([],[])" ==
            IO::to_string(solve(
            "a :- #sum {1:a}!=1."
            )));
        REQUIRE(
            "([[b]],[])" ==
            IO::to_string(solve(
                "b :- 0  #sum+ { 1: b }."
            )));
        REQUIRE(
            "([[b]],[])" ==
            IO::to_string(solve(
                "b :- 0  #sum { 1: b }."
            )));
    }
    SECTION("symTabBug") {
        REQUIRE(
            "([[does(a,0),does(a,1)],[does(a,0),does(b,1)]],[])" ==
            IO::to_string(solve(
                "time(0..1).\n"
                "1 { does(M,T) : legal(M,T) } 1 :- time(T).\n"
                "legal(a,T) :- time(T).\n"
                "legal(b,T) :- does(a,0), time(T).\n",
                {"does"})));
    }
    SECTION("recAntiAggr") {
        REQUIRE(
            "([[p],[r]],[])" ==
            IO::to_string(solve(
                "r :- #sum { 1:p } < 1.\n"
                "p :- not r.")));
        REQUIRE(
            "([[],[p]],[])" ==
            IO::to_string(solve("p :- #sum { 1:not p } < 1.")));
        REQUIRE(
            "([[],[p]],[])" ==
            IO::to_string(solve("p :- not #sum { 1:p } < 1.")));
        REQUIRE(
            "([[],[p]],[])" ==
            IO::to_string(solve("p :- not #sum { 1:not p } > 0.")));
        REQUIRE(
            "([[],[p]],[])" ==
            IO::to_string(solve("p :- not not #sum { 1:p } > 0.")));
        REQUIRE(
            "([[],[p]],[])" ==
            IO::to_string(solve("p :- not not #sum { 1:not p } < 1.")));
    }
    SECTION("headAggrPropagateBug") {
        REQUIRE(
            "([[a(c),a(p),b(c,d),b(c,e),b(c,f),b(c,g),b(p,d),b(p,e),b(p,f),b(p,g)],"
            "[a(c),a(p),b(c,d),b(c,e),b(c,g),b(p,d),b(p,e),b(p,g),e(3)],"
            "[b(c,d),b(c,e),b(p,d),b(p,e),e(2),e(3)]],[-:1:19-20: info: atom does not occur in any rule head:\n  c\n])" == IO::to_string(solve(
                "b(S,h) :- b(S,X), c.\n"
                "b(c,d).\n"
                "b(p,X) :- b(c,X).\n"
                "b(c,e).\n"
                "a(S)   :- b(S,g).\n"
                "1 = { e(3); b(S,f) : a(S) } :- b(S,e).\n"
                "1 = { e(2); b(S,g) } :- b(S,d).\n")));
        //REQUIRE("[-:1:19-20: info: atom does not occur in any rule head:\n  c\n]" == IO::to_string(msg.messages));
    }
    SECTION("headAggregateBug") {
        REQUIRE("([[q(a),r(a)]],[-:1:18-22: info: atom does not occur in any rule head:\n  z(X)\n])" == IO::to_string(solve("1 { q(a); p(X) : z(X) }. r(X) :- q(X).")));
        REQUIRE("([[q(a),r(a)]],[-:1:12-16: info: atom does not occur in any rule head:\n  z(X)\n])" == IO::to_string(solve("1 { p(X) : z(X); q(a) }. r(X) :- q(X).")));
    }
    SECTION("head") {
        REQUIRE("([[],[a],[a,b],[b]],[])" == IO::to_string(solve("{a;b}.")));
        REQUIRE("([[a],[b]],[])" == IO::to_string(solve("1{a;b}1.")));
        REQUIRE("([[p(1)],[p(1),p(2)],[p(1),p(3)],[p(1),p(4)],[p(2)],[p(2),p(3)],[p(2),p(4)],[p(3)],[p(3),p(4)],[p(4)]],[])" == IO::to_string(solve("1#count{X:p(X):X=1..4}2.")));
        REQUIRE("([[p(1)],[p(2)]],[])" == IO::to_string(solve("1#sum+{X:p(X):X=1..4}2.")));
        REQUIRE("([[p(1)],[p(2)]],[])" == IO::to_string(solve("1#sum {X:p(X):X=1..4}2.")));
        REQUIRE(
            "([[p(1)],[p(1),p(2)],[p(1),p(2),p(3)],[p(1),p(2),p(3),p(4)],[p(1),p(2),p(4)],[p(1),p(3)],[p(1),p(3),p(4)],[p(1),p(4)],"
            "[p(2)],[p(2),p(3)],[p(2),p(3),p(4)],[p(2),p(4)]],[])" == IO::to_string(solve("1#min{X:p(X):X=1..4}2.")));
        REQUIRE("([[p(1)],[p(1),p(2)],[p(2)]],[])" == IO::to_string(solve("1#max{X:p(X):X=1..4}2.")));
        REQUIRE("([[c,p]],[])" == IO::to_string(solve("{p}. 1 {c:p}.")));
    }

    SECTION("assign") {
        REQUIRE("([[p,q(1)],[q(0)]],[])" ==IO::to_string(solve("{p}. q(M):-M=#count{1:p}.")));
        REQUIRE("([[p,q(1)],[q(0)]],[])" ==IO::to_string(solve("{p}. q(M):-M=#sum+{1:p}.")));
        REQUIRE("([[p,q(1)],[q(0)]],[])" ==IO::to_string(solve("{p}. q(M):-M=#sum{1:p}.")));
        REQUIRE("([[p,q(p)],[q(#sup)]],[])" ==IO::to_string(solve("{p}. q(M):-M=#min{p:p}.")));
        REQUIRE("([[p,q(p)],[q(#inf)]],[])" ==IO::to_string(solve("{p}. q(M):-M=#max{p:p}.")));
        REQUIRE(
            "([[p(1),p(2),q(1)],"
            "[p(1),p(3),q(1)],"
            "[p(1),p(4),q(1)],"
            "[p(2),p(3),q(2)],"
            "[p(2),p(4),q(2)],"
            "[p(3),p(4),q(3)]],[])" == IO::to_string(solve("2{p(1..4)}2. q(M):-M=#min{X:p(X)}.")));
        REQUIRE(
            "([[p(1),p(2),q(2)],"
            "[p(1),p(3),q(3)],"
            "[p(1),p(4),q(4)],"
            "[p(2),p(3),q(3)],"
            "[p(2),p(4),q(4)],"
            "[p(3),p(4),q(4)]],[])" == IO::to_string(solve("2{p(1..4)}2. q(M):-M=#max{X:p(X)}.")));
    }

    SECTION("conjunction") {
        REQUIRE(
            "([],[-:1:4-5: info: atom does not occur in any rule head:\n  b\n])" ==
            IO::to_string(solve(
                "a:-b:c.\n"
                "c:-a.\n")));
        REQUIRE(
            "([[a,b,c]],[])" ==
            IO::to_string(solve(
                "a:-b:c.\n"
                "c:-a.\n"
                "b:-c.\n", {"a", "b", "c"})));
        REQUIRE(
            "([[a,b,c,d]],[])" ==
            IO::to_string(solve(
                "a:-b:c,d.\n"
                "c:-a.\n"
                "d:-a.\n"
                "b:-c.\n"
                "b:-d.\n", {"a","b","c","d"})));
        REQUIRE(
            "([[a(1),a(2),a(3),c,q],[a(3)]],[])" ==
            IO::to_string(solve(
                "{c}.\n"
                "a(1):-c.\n"
                "a(2):-c.\n"
                "a(3).\n"
                "q:-a(X):X=1..3.\n")));
        REQUIRE(
            "([[p],[q]],[])" ==
            IO::to_string(solve(
                "p :- p:q.\n"
                "q :- q:p.\n")));
        REQUIRE(
            "([[p,q]],[])" ==
            IO::to_string(solve(
                "p :- p:q.\n"
                "q :- q:p.\n"
                "p :- q.\n"
                "q :- p.\n")));
        REQUIRE(
            "([[],[p(1),r,s],[p(2),q,r],[p(3),r,s]],[])" ==
            IO::to_string(solve(
                "{ p(1..3) } 1.\n"
                "q :- p(Y..X) : Y = 2, X = 2.\n"
                "r :- p(X) : #true.\n"
                "s :- p(X-1;2*(X..X+1)-3) : X=2.\n"
                )));
        REQUIRE(
            "([[d(a),d(b),q(1,a),q(2,a)],[d(a),d(b),q(1,b),q(2,b)]],[])" ==
            IO::to_string(solve(
                "d(a;b).\n"
                "2 { q(1,a); q(1,b); q(2,a); q(2,b) } 2.\n"
                " :- q(1,A;2,A) : d(A).\n"
                )));
    }
    SECTION("disjunction") {
        REQUIRE(
            "([[],[a,b,c,d],[c,x,y]],[])" ==
            IO::to_string(solve(
                "{ y; d } 1.\n"
                "c :- y.\n"
                "c :- d.\n"
                "b :- d.\n"
                "x:y | a:b :- c.\n"
                "b :- a.\n"
                )));
        REQUIRE(
            "([[a,b,c,d,x5]],[])" ==
            IO::to_string(solve(
                "x5:-b.\n"
                "x5:-not c.\n"
                "d:-c.\n"
                "c:-d.\n"
                "\n"
                "x5|d:-not not b.\n"
                "a:-x5.\n"
                "\n"
                "c:-a.\n"
                "b:-c.\n"
                )));

        REQUIRE(
            "([[a,b,c],[b],[b,c],[c]],[])" ==
            IO::to_string(solve(
                "1{b;c}.\n"
                "a:b,c;not a.\n")));
        REQUIRE(
            "([[],[p(1)],[p(1),p(2)],[p(2)]],[])" ==
            IO::to_string(solve(
                "q(1..2).\n"
                "p(X); not p(X) :- q(X).\n", {"p("})));
        REQUIRE(
            "([[],[p(1),r(1)],[r(1)]],[])" ==
            IO::to_string(solve(
                "q(1).\n"
                "p(X); not p(X); not r(X) :- q(X).\n"
                "r(X); not r(X) :- q(X).\n", {"p(", "r("})));
        REQUIRE(
            "([[a,aux,b,c,p,s_b1,s_b2,s_c2]],[])" ==
            IO::to_string(solve(
                "aux :- { a; b; c } > 2.\n"

                "p :- aux.\n"
                "p :- not s_c2.\n"

                "s_b1 :- a.\n"
                "s_b1 :- b.\n"
                "a; b :- s_b1.\n"

                "s_b2 :- a, b.\n"
                "a :- s_b2.\n"
                "b :- s_b2.\n"

                "s_c2 :- c, s_b1.\n"
                "s_c2 :- s_b2.\n"
                "c; s_b2 :- s_c2.\n"
                "s_b1; s_b2 :- s_c2.\n"

                "p; s_c2 :- not not aux.\n"

                "a :- p.\n"
                "b :- p.\n"
                "c :- p.\n"

                "p :- a.\n"
                "p :- b.\n"
                "p :- c.\n")));
        REQUIRE(
            "([[p(1,a),p(2,a)],[p(1,b),p(2,b)]],[])" ==
            IO::to_string(solve(
                "d(a;b).\n"
                "p(1,A;2,A) : d(A).\n", {"p("})));
        REQUIRE(
            "([[],[p(1),p(2)]],[])" ==
            IO::to_string(solve(
                "r(1).\n"
                "q(1,2).\n"
                "p(1..Y) : q(X,Y) | not p(1..Y) : q(X,Y) :- r(X).\n", {"p("})));
    }

    SECTION("show") {
        REQUIRE(
            "(["
            "[(1,2,3),-q(1),42],"
            "[(1,2,3),-q(1),42],"
            "[(1,2,3),-q(1),42,p(1)],"
            "[(1,2,3),42],"
            "[(1,2,3),42],"
            "[(1,2,3),42,boo(1)],"
            "[(1,2,3),42,boo(1)],"
            "[(1,2,3),42,boo(1),p(1)],"
            "[(1,2,3),42,p(1)]"
            "],[])" == IO::to_string(solve(
                "#show p/1.\n"
                "#show -q/1.\n"
                "#show boo(X):q(X).\n"
                "#show -p/-1.\n"
                "#show (1,2,3).\n"
                "\n"
                "{p(1); q(1); -p(1); -q(1)}.\n"
                "\n"
                "#const p=42.\n")));
        REQUIRE(
            "([[a,c,x=1,y=1]],[])" == IO::to_string(solve(
                "a. b.\n"
                "$x $= 1. $y $= 1. $z $= 1.\n"
                "#show a/0.\n"
                "#show c.\n"
                "#show $x/0.\n"
                "#show $y.\n"
                )));
        REQUIRE(
            "([[x=1],[y=1]],[])" ==
            IO::to_string(solve(
                "{b}.\n"
                "$x $= 1. $y $= 1.\n"
                "#show.\n"
                "#show $x:b.\n"
                "#show $y:not b.\n"
                )));
    }

    SECTION("aggregates") {
        REQUIRE("([[]],[])" == IO::to_string(solve("#sum { 1:b; 2:c } < 1.\n")));
        REQUIRE("([[p(1),p(2)],[p(1),p(3)],[p(2),p(3)]],[])" == IO::to_string(solve("{p(1..3)}.\n:-{p(X)}!=2.")));
        REQUIRE("([[],[a,b],[b]],[])" == IO::to_string(solve("#sum { -1:a; 1:b } >= 0.")));
        REQUIRE("([[],[a,b],[b]],[])" == IO::to_string(solve("#sum { 1:a; 2:b } != 1.")));
        REQUIRE("([],[])" == IO::to_string(solve("a. {a} 0.")));
    }

    SECTION("aggregates2") {
        REQUIRE(
            "([[c]],[])" == IO::to_string(solve(
                "a :- not { c } >= 1, not c."
                "b :- a, #false."
                "c :- not b, {b; not b} >= 1."
                )));
        REQUIRE(
            "([[c]],[])" == IO::to_string(solve(
                "a :- not not { c } >= 1, not c."
                "b :- a, #false."
                "c :- not b, {b; not b} >= 1."
                )));
    }

    SECTION("invert") {
        std::string prg =
            "p(-1;2).\n"
            "q(X) :- p((-(X+3))+2).\n"
            "a(a;-b).\n"
            "b(X) :- a(---X).\n"
            "c(X) :- a(----X).\n";
        REQUIRE("([[a(-b),a(a),b(-a),b(b),c(-b),c(a),p(-1),p(2),q(-3),q(0)]],[])" == IO::to_string(solve(prg)));
    }

    SECTION("minimize") {
        std::string prg;
        prg =
            "{a; b; c; d}.\n"
            "#minimize {1,a:a; 1,b:b; 1,c:c; 1,d:d}.\n"
            "ok :- a,    b,not c,not d.\n"
            "ok :- not a,b,    c,    d.\n"
            "ok :- a,not b,    c,    d.\n"
            ":- not ok.\n";
        REQUIRE("([[a,b]],[])" == IO::to_string(solve(prg, {"a", "b", "c", "d"}, {2})));
        REQUIRE("([],[])" == IO::to_string(solve(prg, {"a", "b", "c", "d"}, {1})));
        prg =
            "{a; b; c; d}.\n"
            ":~ a. [1,a]\n"
            ":~ b. [1,b]\n"
            ":~ c. [1,c]\n"
            ":~ d. [1,d]\n"
            "ok :- a,    b,not c,not d.\n"
            "ok :- not a,b,    c,    d.\n"
            "ok :- a,not b,    c,    d.\n"
            ":- not ok.\n";
        REQUIRE("([[a,b]],[])" == IO::to_string(solve(prg, {"a", "b", "c", "d"}, {2})));
        REQUIRE("([],[])" == IO::to_string(solve(prg, {"a", "b", "c", "d"}, {1})));
        prg =
            "{a; b; c; d}.\n"
            "#maximize {-1,a:a; -1,b:b; -1,c:c; -1,d:d}.\n"
            "ok :- a,    b,not c,not d.\n"
            "ok :- not a,b,    c,    d.\n"
            "ok :- a,not b,    c,    d.\n"
            ":- not ok.\n";
        REQUIRE("([[a,b]],[])" == IO::to_string(solve(prg, {"a", "b", "c", "d"}, {2})));
        REQUIRE("([],[])" == IO::to_string(solve(prg, {"a", "b", "c", "d"}, {1})));
        prg =
            "{a; b; c; d}.\n"
            "#minimize {3,a:a; 3,b:b; 1,x:c; 1,x:d}.\n"
            "ok :- a,    b,not c,not d.\n"
            "ok :- not a,b,    c,    d.\n"
            "ok :- a,not b,    c,    d.\n"
            ":- not ok.\n"
            ;
        REQUIRE("([[a,c,d],[b,c,d]],[])" == IO::to_string(solve(prg, {"a", "b", "c", "d"}, {4})));
        REQUIRE("([],[])" == IO::to_string(solve(prg, {"a", "b", "c", "d"}, {3})));
        prg =
            "{a; b; c; d}.\n"
            "#minimize {3@2,a:a; 3@2,b:b; 1@2,x:c; 1@2,x:d}.\n"
            "#minimize {1@1,x:a; 1@1,x:c; 1@1,y:b}.\n"
            "ok :- a,    b,not c,not d.\n"
            "ok :- not a,b,    c,    d.\n"
            "ok :- a,not b,    c,    d.\n"
            ":- not ok.\n"
            ;
        REQUIRE("([[a,c,d]],[])" == IO::to_string(solve(prg, {"a", "b", "c", "d"}, {4, 1})));
        REQUIRE("([],[])" == IO::to_string(solve(prg, {"a", "b", "c", "d"}, {4, 0})));
        REQUIRE("([[]],[])" == IO::to_string(solve("{p}. #maximize{1:not p}.", {"p"}, {-1})));
    }

    SECTION("csp") {
        REQUIRE(
            "([[p(1)=1,p(2)=1,x=1],[p(1)=1,p(2)=2,x=1],[p(1)=2,p(2)=1,x=1],[p(1)=2,p(2)=2,x=1]],[])" ==
            IO::to_string(solve(
                "1 $<= $p(1..2) $<= 2.\n"
                "$x $= 1.\n"
                )));
        REQUIRE(
            "([[x=0,y=0,z=2],[x=0,y=0,z=3],[x=0,y=1,z=3],[x=1,y=0,z=3]],[])" ==
            IO::to_string(solve(
                "0 $<= $(x;y;z) $<= 3.\n"
                "$x $+ $y $+ -1$*$z $<= -2.\n"
                )));
        REQUIRE(
            "([[x=0,y=0,z=2],[x=0,y=0,z=3],[x=0,y=1,z=3],[x=1,y=0,z=3]],[])" ==
            IO::to_string(solve(
                "0 $<= $(x;y;z) $<= 3.\n"
                "p:-$x $+ $y $+ -1$*$z $<= -2.\n"
                ":- not p.\n", {"x", "y", "z"}
                )));
    }

    SECTION("cspbound") {
        REQUIRE(
            "([[x=4],[x=5]],[])" ==
            IO::to_string(solve(
                "$x $<= 5.\n"
                ":- $x $<= 3, $x $<=4.\n", {"x="}
                )));
        REQUIRE(
            "([[x=2],[x=4],[x=6]],[])" ==
            IO::to_string(solve(
                "$x $= 2*X : X = 1..3.\n", {"x="}
                )));
        REQUIRE(
            "([[x=1],[x=3],[x=7]],[])" ==
            IO::to_string(solve(
                "$x $= 1; $x $= 3 :- $x $!= 7."
                , {"x="})));
    }

    SECTION("disjoint") {
        REQUIRE(
            "([[x,x=2,y=2],[x=2,y=1],[x=2,y=2]],[])" ==
            IO::to_string(solve(
                "1 $<= $x $<= 2.\n"
                "1 $<= $y $<= 2.\n"
                "{x}.\n"
                "#disjoint{ 1:1; 2:$x; 2:$y : x }.\n"
                )));
        REQUIRE(
            "([[x=2]],[])" ==
            IO::to_string(solve(
                "1 $<= $x $<= 2.\n"
                "#disjoint{ 1:1; 2:$x }.\n"
                )));
        REQUIRE(
            "([[a,x=1],[b,x=1],[x=1]],[])" ==
            IO::to_string(solve(
                "$x $= 1.\n"
                "{ a; b }.\n"
                "#disjoint{ a:$x:a; b:$x:b }.\n"
                )));
        REQUIRE(
            "([[a,b,y=2],[a,y=2],[b,y=2],[y=1],[y=2]],[])" ==
            IO::to_string(solve(
                "1 $<= $y $<= 2.\n"
                "{ a; b }.\n"
                "#disjoint{ 1:1:a; 1:1:b; 2:$y }.\n"
                )));
        REQUIRE(
            "([[p(1)=1,p(2)=1,p(3)=1,q(1)=2,q(2)=2,q(3)=2],[p(1)=2,p(2)=2,p(3)=2,q(1)=1,q(2)=1,q(3)=1]],[])" ==
            IO::to_string(solve(
                "1 $<= $(p(1..3);q(1..3)) $<= 2.\n"
                "#disjoint{ 1:$p(1..3); 2:$q(1..3) }.\n"
                )));
        REQUIRE(
            "([[x=6,y=35]],[])" ==
            IO::to_string(solve(
                "6  $<= $x $<=  7.\n"
                "35 $<= $y $<= 36.\n"
                "not #disjoint{ 1:6$*$y; 2:35$*$x }.\n"
                )));
        REQUIRE(
            "([[x=1,y=1,z=1]"
            ",[x=2,y=2,z=2]"
            ",[x=3,y=3,z=3]],[])" ==
            IO::to_string(solve(
                "1  $<= $(x;y;z) $<=  3.\n"
                "not #disjoint{ 1:2$*$x $+ 3$*$y; 2:2$*$y $+ 3$*$z; 3:2$*$z $+ 3$*$x }.\n"
                )));
        REQUIRE(
            "([[x=6,y=35]],[])" ==
            IO::to_string(solve(
                "6  $<= $x $<=  7.\n"
                "35 $<= $y $<= 36.\n"
                "not #disjoint{ 1:6$*$y; 2:35$*$x }.\n"
                )));
        REQUIRE(
            "([[a],[a,b]],[])" ==
            IO::to_string(solve(
                "{b}.\n"
                "a :- #disjoint { 1 : 1 : a; 2 : 2 : a; 3 : 3 : b }.\n"
                )));
        REQUIRE(
            "([],[])" ==
            IO::to_string(solve(
                "{b}.\n"
                "a :- #disjoint { 1 : 1 : a; 2 : 1 : a; 3 : 3 : b }.\n"
                )));
        REQUIRE(
            "([[a]],[])" ==
            IO::to_string(solve(
                "{b}.\n"
                "a :- #disjoint { 1 : 1 : a; 2 : 2 : a; 3 : 2 : b }.\n"
                )));
        REQUIRE(
            "([[b]],[])" ==
            IO::to_string(solve(
                "{b}.\n"
                "a :- #disjoint { 1 : 1 : a; 2 : 1 : a; 3 : 3 : b; 4 : 3 : b }.\n"
                )));
        REQUIRE(
            576 == solve(
                "#const n = 4.\n"
                "row(1..n).\n"
                "col(1..n).\n"
                "dom(1,n).\n"
                "\n"
                "L $<= $cell(X,Y) $<= U :- row(X), col(Y), dom(L,U).\n"
                ":- col(Y), not #disjoint { X : $cell(X,Y) : row(X) }.\n"
                ":- row(X), not #disjoint { Y : $cell(X,Y) : col(Y) }.\n"
                ).first.size());
    }

    SECTION("queens") {
        REQUIRE(
            "([[q(1,2),q(2,4),q(3,6),q(4,1),q(5,3),q(6,5)],"
            "[q(1,3),q(2,6),q(3,2),q(4,5),q(5,1),q(6,4)],"
            "[q(1,4),q(2,1),q(3,5),q(4,2),q(5,6),q(6,3)],"
            "[q(1,5),q(2,3),q(3,1),q(4,6),q(5,4),q(6,2)]],[])" ==
            IO::to_string(solve(
                "#const n = 6.\n"
                "n(1..n).\n"
                "\n"
                "q(X,Y) :- n(X), n(Y), not not q(X,Y).\n"
                "\n"
                "        c(r,X; c,Y) :- q(X,Y).\n"
                "not not c(r,N; c,N) :- n(N).\n"
                "\n"
                "n(r,X,Y-1,X,Y; c,X-1,Y,X,Y; d1,X-1,Y-1,X,Y;     d2,X-1,Y+1,X,Y      ) :- n(X), n(Y).\n"
                "c(r,N,0;       c,0,N;       d1,N-1,0; d1,0,N-1; d2,N-1,n+1; d2,0,N+1) :- n(N).\n"
                "\n"
                "c(C,XX,YY) :-     c(C,X,Y), n(C,X,Y,XX,YY), not q(XX,YY).\n"
                "           :- not c(C,X,Y), n(C,X,Y,XX,YY),     q(XX,YY).\n", {"q("})));
        REQUIRE(
            48 == solve(
                "#const n=4.\n"
                "1 $<= $(row(X);col(X)) $<= n :- X=1..n.\n"
                "$row(X) $!= $row(Y) :- X=1..n, Y=1..n, X<Y.\n"
                "$col(X) $!= $col(Y) :- X=1..n, Y=1..n, X<Y.\n"
                "$row(X) $+ $col(Y) $!= $row(Y) $+ $col(X) :- X=1..n, Y=1..n, X<Y.\n"
                "$row(X) $+ $col(X) $!= $row(Y) $+ $col(Y) :- X=1..n, Y=1..n, X<Y.\n"
                ).first.size());
        std::string q5 =
            "([[q(1)=1,q(2)=3,q(3)=5,q(4)=2,q(5)=4]"
            ",[q(1)=1,q(2)=4,q(3)=2,q(4)=5,q(5)=3]"
            ",[q(1)=2,q(2)=4,q(3)=1,q(4)=3,q(5)=5]"
            ",[q(1)=2,q(2)=5,q(3)=3,q(4)=1,q(5)=4]"
            ",[q(1)=3,q(2)=1,q(3)=4,q(4)=2,q(5)=5]"
            ",[q(1)=3,q(2)=5,q(3)=2,q(4)=4,q(5)=1]"
            ",[q(1)=4,q(2)=1,q(3)=3,q(4)=5,q(5)=2]"
            ",[q(1)=4,q(2)=2,q(3)=5,q(4)=3,q(5)=1]"
            ",[q(1)=5,q(2)=2,q(3)=4,q(4)=1,q(5)=3]"
            ",[q(1)=5,q(2)=3,q(3)=1,q(4)=4,q(5)=2]],[])";
        REQUIRE(
            q5 == IO::to_string(solve(
                "#const n=5.\n"
                "1 $<= $q(1..n) $<= n.\n"
                "$q(X) $!= $q(Y) :- X=1..n, Y=1..n, X<Y.\n"
                "X $+ $q(Y) $!= Y $+ $q(X) :- X=1..n, Y=1..n, X<Y.\n"
                "X $+ $q(X) $!= Y $+ $q(Y) :- X=1..n, Y=1..n, X<Y.\n"
                )));
        REQUIRE(
            q5 == IO::to_string(solve(
                "#const n=5.\n"
                "1 $<= $q(1..n) $<= n.\n"
                "#disjoint { X : $q(X)$+0 : X=1..n }.\n"
                "#disjoint { X : $q(X)$+X : X=1..n }.\n"
                "#disjoint { X : $q(X)$-X : X=1..n }.\n"
                )));
        REQUIRE(
            q5 == IO::to_string(solve(
                "#const n=5.\n"
                "1 $<= $q(1..n) $<= n.\n"
                "#disjoint { X : $q(X)     : X=1..n }.\n"
                ":- not #disjoint { X : $q(X)$+ X : X=1..n }.\n"
                "not not #disjoint { X : $q(X)$+-X : X=1..n }.\n"
                )));
    }

    SECTION("undefinedRule") {
        REQUIRE(
            "([[q(2)]],[-:4:3-6: info: operation undefined:\n  (A+B)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "b(1).\n"
                "q(A+B) :- a(A), b(B).\n", {"q("})));
        REQUIRE(
            "([[q(1,1,2)]],[-:4:27-30: info: operation undefined:\n  (A+B)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "b(1).\n"
                "q(A,B,X) :- a(A), b(B), X=A+B.\n", {"q("})));
        REQUIRE(
            "([[q(1,1)]],[-:4:29-32: info: operation undefined:\n  (A+B)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "b(1).\n"
                "q(A,B) :- a(A), b(B), not b(A+B).\n", {"q("})));
        REQUIRE(
            "([[q(1)]],[-:4:3-7: info: interval undefined:\n  A..B\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "b(1).\n"
                "q(A..B) :- a(A), b(B).\n", {"q("})));
    }

    SECTION("undefinedCSP") {
        REQUIRE( "([[a(1),a(a),b(1)]],[-:4:1-2: info: number expected:\n  A\n])" == IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "b(1).\n"
                "A $<= B :- a(A), b(B).\n")));
        REQUIRE(
            "([[a(1),a(a),b(1)]],[-:4:16-17: info: number expected:\n  A\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "b(1).\n"
                ":- a(A), b(B), A $< B.\n")));
        REQUIRE(
            "([[a(1),a(2)=1,a(a),b(1)]],[-:5:19-20: info: operation undefined:\n  (1*A+1)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "b(1).\n"
                "$a(2) $<= 1.\n"
                ":- a(A), b(B), $a(A+1) $< B.\n")));
    }

    SECTION("undefinedCSPDisjoint") {
        REQUIRE(
            "([[a(1),a(a)]],[-:3:17-18: info: number expected:\n  X\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "#disjoint { X : X : a(X) }.\n")));
        REQUIRE(
            "([[a(1),a(1)=1,a(a)]],[-:4:20-21: info: operation undefined:\n  (1*X+1)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "$a(1) $= 1.\n"
                "#disjoint { X : $a(X+1) : a(X) }.\n")));
    }

    SECTION("undefinedBodyAggregate") {
        REQUIRE(
            "([[a(1),a(a),h]],[-:3:15-16: info: operation undefined:\n  (1*X+1)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "h :- #count { X+1 : a(X) } < 2.\n")));
        REQUIRE(
            "([[a(1),a(a),h]],[-:3:14-15: info: operation undefined:\n  (1*X+1)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "h :- { not a(X+1) : a(X) } < 2.\n")));
        REQUIRE(
            "([[a(1),a(a),g(1)]],[-:3:30-31: info: operation undefined:\n  (1*X+1)\n,-:4:30-31: info: operation undefined:\n  (1*X+1)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "g(X) :- not X < #count { } < X+1, a(X).\n"
                "h(X) :-     X < #count { } < X+1, a(X).\n")));
    }

    SECTION("undefinedAssignmentAggregate") {
        REQUIRE(
            "([[a(1),a(a),h(1)]],[-:3:22-23: info: operation undefined:\n  (1*X+1)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "h(C) :- C = #count { X+1 : a(X) }.\n")));
        REQUIRE(
            "([[a(1),a(a),h(1)]],[-:3:21-22: info: operation undefined:\n  (1*X+1)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "h(C) :- C = { not a(X+1) : a(X) }.\n")));
        REQUIRE(
            "([[]],[])" ==
            IO::to_string(solve(
                "h(C) :- C+1 = #min { a }.\n")));
    }

    SECTION("undefinedHeadAggregate") {
        REQUIRE(
            "([[a(1),a(a)],[a(1),a(a),p(1)]],[-:3:10-11: info: operation undefined:\n  (1*X+1)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "#count { X+1 : p(X) : a(X) }.\n")));
        REQUIRE(
            "([[a(1),a(a)],[a(1),a(a),p(2)]],[-:3:16-17: info: operation undefined:\n  (1*X+1)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "#count { X : p(X+1) : a(X) }.\n")));
        REQUIRE(
            "([[a(1),a(a)],[a(1),a(a),p(2)]],[-:3:5-6: info: operation undefined:\n  (1*X+1)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "{ p(X+1) : a(X) }.\n")));
        REQUIRE(
            "([[p(1)]],[-:3:1-2: info: operation undefined:\n  (1*X+-1)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1).\n"
                "X-1 < { p(X) } < X+1 :- a(X).\n", {"p("})));
    }

    SECTION("undefinedConjunction") {
        REQUIRE(
            "([[a(a)],[h]],[-:4:10-11: info: operation undefined:\n  (1*A+1)\n])" ==
            IO::to_string(solve(
                "{a(a)}.\n"
                "a(1..2).\n"
                "p(2..3).\n"
                "h :- p(1+A):a(A).\n", {"h", "a(a)"})));
        REQUIRE(
            "([[a(a)],[a(a),p(2),p(3)],[h],[p(2),p(3)]],[-:4:14-15: info: operation undefined:\n  (1*A+1)\n])" ==
            IO::to_string(solve(
                "{a(a)}.\n"
                "a(1..2).\n"
                "{p(2..3)} != 1.\n"
                "h :- not p(1+A):a(A).\n", {"h", "a(a)", "p("})));
        REQUIRE(
            "([[a(a),h],[a(a),p(2),p(3)],[h],[p(2),p(3)]],[-:4:24-25: info: operation undefined:\n  (1*A+1)\n])" ==
            IO::to_string(solve(
                "{a(a)}.\n"
                "a(1..2).\n"
                "{p(2..3)} != 1.\n"
                "h :- not p(X):a(A),X=1+A.\n", {"h", "a(a)", "p("})));
    }

    SECTION("undefinedDisjunction") {
        REQUIRE(
            "([[]],[-:3:5-6: info: operation undefined:\n  (1*A+1)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1..2).\n"
                "p(1+A):a(A).\n", {"p("})));
        REQUIRE(
            "([[p(2)],[p(3)]],[-:3:15-16: info: operation undefined:\n  (1*A+1)\n])" ==
            IO::to_string(solve(
                "a(a).\n"
                "a(1..2).\n"
                "p(X):a(A),X=1+A.\n", {"p("})));
    }

    SECTION("undefinedScript") {
        REQUIRE(
            "([[]],[-:1:3-13: info: operation undefined:\n  function 'failure' not found\n])" ==
            IO::to_string(solve(
                "a(@failure()).\n")));
    }

    SECTION("minMax") {
        REQUIRE(
            "([[a],[b]],[])" ==
            IO::to_string(solve(
                "a :- #min { 1:a; 2:b } != 2.\n"
                "b :- #max { 1:a; 2:b } != 1.\n")));
        REQUIRE(
            "([[a,b]],[])" ==
            IO::to_string(solve(
                "a :- b.\n"
                "b :- a.\n"
                "a :- #min { 1:a; 2:b } != 2.\n"
                "b :- #max { 1:a; 2:b } != 1.\n")));
        REQUIRE(
            "([[a,b,c,w,z],[a,b,w,z],[a,c,w,z],[a,w,z],[b,c,x,y,z],[b,x,y,z],[c,w,y],[w,y]],[])" ==
            IO::to_string(solve(
                "{a;b;c}.\n"
                "w :- #min { 1:a;2:b;3:c } != 2.\n"
                "x :- #min { 1:a;2:b;3:c } = 2.\n"
                "y :- #min { 1:a;2:b;3:c } >= 2.\n"
                "z :- #min { 1:a;2:b;3:c } <= 2.\n")));
    }

    SECTION("nonmon") {
        REQUIRE(
            "([[true(e,x1,1)]],[])" ==
            IO::to_string(solve(
                "{ true(e,L,C) } :- var(e,L,C).\n"
                "true(a,L,C) :- unequal, var(a,L,C).\n"
                "unequal :- int(V), #sum{ C,Q,L : true(Q,L,C) } != V.\n"
                ":- not unequal.\n"

                "var(e,x1,1).\n"
                "var(e,x2,2).\n"
                "var(a,y1,2).\n"
                "var(a,y2,3).\n"
                "int(5).\n", {"true(e"})));
        REQUIRE(
            "([[true(e,x1,1)]],[])" ==
            IO::to_string(solve(
                "{ true(e,L,C) } :- var(e,L,C).\n"
                "true(a,L,C) :- unequal, var(a,L,C).\n"
                "unequal :- int(V), #sum{ C,Q,L : true(Q,L,C) } != V.\n"
                ":- not unequal.\n"

                "var(e,x1,1).\n"
                "var(e,x2,2).\n"
                "var(a,y1,2).\n"
                "var(a,y2,3).\n"
                "int(5).\n", {"true(e"})));

        REQUIRE(
            "([],[])" ==
            IO::to_string(solve(
                "{ true(e,L,C) } :- var(e,L,C).\n"
                "true(a,L,C) :- unequal, var(a,L,C).\n"
                "unequal :- int(V), #sum{ C,Q,L : true(Q,L,C) } != V.\n"
                ":- not unequal.\n"

                "var(e,x1,2).\n"
                "var(e,x2,4).\n"
                "var(a,y1,-2).\n"
                "var(a,y2,4).\n"
                "int(4).\n", {"true(e"})));

        REQUIRE(
            "([[],[true(e,x1,6),true(e,x2,3)]],[])" ==
            IO::to_string(solve(
                "{ true(e,L,C) } :- var(e,L,C).\n"
                "true(a,L,C) :- unequal, var(a,L,C).\n"
                "unequal :- int(V), #sum{ C,Q,L : true(Q,L,C) } != V.\n"
                ":- not unequal.\n"

                "var(e,x1,6).\n"
                "var(e,x2,3).\n"
                "var(a,y1,-2).\n"
                "var(a,y2,1).\n"
                "int(4).\n", {"true(e"})));
    }

    SECTION("edge") {
        REQUIRE(
            "([[path(a,b),path(b,c),path(c,d),path(d,a)]],[])" ==
            IO::to_string(solve(
                "node(a;b;c;d).\n"
                "edge(a,b;b,(c;d);c,(a;d);d,a).\n"
                "1 { path(X,Y) : edge(X,Y) } 1 :- node(X).\n"
                "1 { path(X,Y) : edge(X,Y) } 1 :- node(Y).\n"
                "start(X) :- X = #min { Y : node(Y) }.\n"
                "#edge (X,Y) : path(X,Y), not start(Y).\n", {"path("})));
    }

    SECTION("bugRewriteCond") {
        REQUIRE(
            "([[p(1)],[p(1),p(2)],[p(2)]],[])" ==
            IO::to_string(solve(
                "{p(1..2)}.\n"
                ":- #false:p(X).\n", {"p("})));
    }

}

#ifdef WITH_PYTHON

TEST_CASE("output-lparse-python", "[output][python]") {
    REQUIRE(
        "([[p(39),q(\"a\"),q(1),q(2),q(a),r(2),r(3),s((1,2)),s((1,3)),s((2,1))]],[])" == IO::to_string(solve(
            "#script (python)\n"
            "import clingo\n"
            "def conv(a): return a.number if hasattr(a, 'number') else a\n"
            "def pygcd(a, b): return b if conv(a) == 0 else pygcd(conv(b) % conv(a), a)\n"
            "def pytest():    return [1, 2, clingo.function(\"a\"), \"a\"]\n"
            "def pymatch():   return [(1,2),(1,3),(2,1)]\n"
            "#end.\n"
            "\n"
            "p(@pygcd(2*3*7*13,3*11*13)).\n"
            "q(@pytest()).\n"
            "r(X) :- (1,X)=@pymatch().\n"
            "s(X) :- X=@pymatch().\n"
            )));
}

#endif // WITH_PYTHON

#ifdef WITH_LUA

TEST_CASE("output-lparse-lua", "[output][lua]") {
    REQUIRE(
        "([[p(39),q(\"a\"),q(1),q(2),q(a),r(2),r(3),s((1,2)),s((1,3)),s((2,1))]],[])" == IO::to_string(solve(
            "#script (lua)\n"
            "function conv(a) if type(a) == 'number' then return a else return a.number end end\n"
            "function luagcd(a, b) if conv(a) == 0 then return b else return luagcd(conv(b) % conv(a), a) end end\n"
            "function luatest()    return {1, 2, clingo.fun(\"a\"), \"a\"} end\n"
            "function luamatch()   return {clingo.tuple{1,2},clingo.tuple{1,3},clingo.tuple{2,1}} end\n"
            "#end.\n"
            "\n"
            "p(@luagcd(2*3*7*13,3*11*13)).\n"
            "q(@luatest()).\n"
            "r(X) :- (1,X)=@luamatch().\n"
            "s(X) :- X=@luamatch().\n"
            )));
}
#endif // WITH_LUA

} } } // namespace Test Output Gringo

