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

#include "gringo/input/nongroundparser.hh"
#include "gringo/input/programbuilder.hh"
#include "gringo/input/program.hh"
#include "gringo/output/output.hh"
#include "gringo/scripts.hh"

#include "tests/tests.hh"
#include "tests/term_helper.hh"
#include "tests/gringo_module.hh"

namespace Gringo { namespace Input { namespace Test {

// {{{ declaration of TestProgram

class TestProgram : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestProgram);
        CPPUNIT_TEST(test_rewrite);
        CPPUNIT_TEST(test_defines);
        CPPUNIT_TEST(test_check);
        CPPUNIT_TEST(test_projection);
        CPPUNIT_TEST(test_statements);
        CPPUNIT_TEST(test_theory);
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void setUp();
    virtual void tearDown();

    void test_rewrite();
    void test_defines();
    void test_check();
    void test_projection();
    void test_statements();
    void test_theory();

    std::string message() { return replace_all(messages.back(), ";#inc_base", ""); }
    bool check(std::pair<Program, Defines> &&x);

    virtual ~TestProgram();

    std::vector<std::string> messages;
    std::unique_ptr<MessagePrinter> oldPrinter;
};

// }}}

using namespace Gringo::IO;
using namespace Gringo::Test;

// {{{ definition of auxiliary functions

namespace {

std::pair<Program, Defines> parse(std::string const &str) {
    std::ostringstream oss;
    Potassco::TheoryData td;
    Output::OutputBase out(td, {}, oss);
    std::pair<Program, Defines> ret;
    Scripts scripts(Gringo::Test::getTestModule());
    NongroundProgramBuilder pb{ scripts, ret.first, out, ret.second };
    NonGroundParser ngp{ pb };
    ngp.pushStream("-", gringo_make_unique<std::stringstream>(str));
    ngp.parse();
    return ret;
}

std::string rewrite(std::pair<Program, Defines> &&x) {
    x.second.init();
    x.first.rewrite(x.second);
    auto str(to_string(x.first));
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
    replace_all(str, ";[#inc_base]", "");
    replace_all(str, ":-[#inc_base].", ".");
    replace_all(str, ":-[#inc_base],", ":-");
    replace_all(str, ":-[#inc_base];", ":-");
    replace_all(str, ";  ", ";");
    replace_all(str, "{  ", "{");
    return str;
}

} // namespace

// }}}
// {{{ definition of TestProgram

void TestProgram::setUp() {
    oldPrinter = std::move(message_printer());
    message_printer() = gringo_make_unique<TestMessagePrinter>(messages);
}

void TestProgram::tearDown() {
    std::swap(message_printer(), oldPrinter);
}

bool TestProgram::check(std::pair<Program, Defines> &&x) {
    message_printer() = gringo_make_unique<TestMessagePrinter>(messages);
    x.first.rewrite(x.second);
    x.first.check();
    return !message_printer()->hasError();
}

void TestProgram::test_rewrite() {
    CPPUNIT_ASSERT_EQUAL(std::string("p(1):-q.p(2):-q.p(3):-q.p:-q."), rewrite(parse("p(1;2;3;):-q.")));
    CPPUNIT_ASSERT_EQUAL(std::string("p:-q(1).p:-q(2).p:-q(3).p:-q."), rewrite(parse("p:-q(1;2;3;).")));
    CPPUNIT_ASSERT_EQUAL(std::string("p(1):-q(3).p(2):-q(3).p(1):-q(4).p(2):-q(4)."), rewrite(parse("p(1;2):-q(3;4).")));
    CPPUNIT_ASSERT_EQUAL(std::string("p((X+Y)):-q(#Arith0);#Arith0=(X+Y)."), rewrite(parse("p(X+Y):-q(X+Y).")));
    CPPUNIT_ASSERT_EQUAL(std::string("#Arith0<=#count{(X+Y):q((X+Y)):r(#Arith0),s(#Arith1),#Arith1=(A+B)}:-t(#Arith0);#Arith0=(X+Y);1<=#count{(X+Y):u(#Arith0),v(#Arith2),#Arith2=(A+B)}."), rewrite(parse("X+Y#count{X+Y:q(X+Y):r(X+Y),s(A+B)}:-t(X+Y),1#count{X+Y:u(X+Y),v(A+B)}.")));
    CPPUNIT_ASSERT_EQUAL(std::string("p(#Range0):-q(#Range1);#range(#Range1,A,B);#range(#Range0,X,Y)."), rewrite(parse("p(X..Y):-q(A..B).")));
    CPPUNIT_ASSERT_EQUAL(std::string("p(1):-q.p(2):-q.p(3):-q.p:-q."), rewrite(parse("p(1;2;3;):-q.")));
    CPPUNIT_ASSERT_EQUAL(std::string("p(Z):-p(A,B);Y=#count{0,q(B):q(B)};X=#count{0,q(A):q(A)};Z=#count{0,r(X,Y):r(X,Y)}."), rewrite(parse("p(Z):-p(A,B),X={q(A)},Y={q(B)},Z={r(X,Y)}.")));
    CPPUNIT_ASSERT_EQUAL(std::string("p(Z):-Z=#count{0,p(X):p(X)};Z>0."), rewrite(parse("p(Z):-Z={p(X)},Z>0.")));
    CPPUNIT_ASSERT_EQUAL(std::string(":~[#inc_p(#Inc0,#Inc1)];0=0.[#Inc0@0,#Inc1]"), rewrite(parse("#program p(k,t). :~ #true. [ k,t ]")));
    CPPUNIT_ASSERT_EQUAL(std::string("#project p(#Arith0):-[p(#Arith0)];#Arith0=(X+X)."), rewrite(parse("#project p(X+X).")));
    CPPUNIT_ASSERT_EQUAL(std::string("#project p(X):-[p(X)].#project p(Y):-[p(Y)]."), rewrite(parse("#project p(X;Y).")));
    CPPUNIT_ASSERT_EQUAL(std::string("#edge(a,b).#edge(c,d)."), rewrite(parse("#edge (a,b;c,d).")));
    CPPUNIT_ASSERT_EQUAL(std::string("#edge((X+X),b)."), rewrite(parse("#edge (X+X,b).")));
    CPPUNIT_ASSERT_EQUAL(std::string("#edge(#Range0,b):-#range(#Range0,1,10)."), rewrite(parse("#edge (1..10,b).")));
    CPPUNIT_ASSERT_EQUAL(std::string("#heuristic a(#Arith0)[1@0,sign]:-[a(#Arith0)];#Arith0=(X+X)."), rewrite(parse("#heuristic a(X+X). [1@0,sign]")));
    CPPUNIT_ASSERT_EQUAL(std::string("#heuristic a[(X+X)@2,sign]:-[a]."), rewrite(parse("#heuristic a. [X+X@2,sign]")));
    CPPUNIT_ASSERT_EQUAL(std::string("#heuristic a(#Range0)[2@0,sign]:-[a(#Range0)];#range(#Range0,1,2)."), rewrite(parse("#heuristic a(1..2). [2,sign]")));
    CPPUNIT_ASSERT_EQUAL(std::string("#theory x{node{};&edge/1:node,head}.#false:-p(Z);not &edge((Z+Z)){(z),(Y): p(Y,#Arith1),#Arith1=(Y+Y)}."), rewrite(parse("#theory x{ node{}; &edge/1: node, head }.&edge(Z+Z) { z, Y : p(Y,Y+Y)} :- p(Z).")));
    CPPUNIT_ASSERT_EQUAL(std::string("#theory x{node{};&edge/1:node,head}.#false:-p(Z);#range(#Range0,Z,Z);not &edge(#Range0){(z),(Y): p(Y,#Range1),#range(#Range1,Y,Y)}."), rewrite(parse("#theory x{ node{}; &edge/1: node, head }.&edge(Z..Z) { z, Y : p(Y,Y..Y)} :- p(Z).")));
}

void TestProgram::test_defines() {
    CPPUNIT_ASSERT_EQUAL(std::string("p(1):-q."),  rewrite(parse("#const a=1.#const b=a.#const c=b.#const d=c.p(d):-q.")));
    CPPUNIT_ASSERT_EQUAL(std::string("p(2):-q."),  rewrite(parse("#const c=a+b.#const b=a.#const a=1.p(c):-q.")));
    CPPUNIT_ASSERT_EQUAL(std::string("p(1,2,3)."), rewrite(parse("#const x=1.#const y=1+x.#const z=1+y.p(x,y,z).")));
    CPPUNIT_ASSERT_EQUAL(std::string("a."),        rewrite(parse("#const a=b.a.")));
    CPPUNIT_ASSERT_EQUAL(std::string("a(b)."),     rewrite(parse("#const a=b.a(a).")));
    CPPUNIT_ASSERT_EQUAL(std::string("#project p(2):-[p(2)]."), rewrite(parse("#project p(x).#const x=2.")));
    CPPUNIT_ASSERT_EQUAL(std::string("#project x:-[x]."), rewrite(parse("#project x.#const x=2.")));
    CPPUNIT_ASSERT_EQUAL(std::string("#edge(2,y)."), rewrite(parse("#edge(x,y).#const x=2.")));
    CPPUNIT_ASSERT_EQUAL(std::string("#heuristic x[2@2,sign]:-[x]."), rewrite(parse("#heuristic x. [x@x,sign]#const x=2.")));
    CPPUNIT_ASSERT_EQUAL(std::string("#heuristic p(2)[2@2,sign]:-[p(2)]."), rewrite(parse("#heuristic p(x). [x@x,sign]#const x=2.")));
    CPPUNIT_ASSERT_EQUAL(std::string("#theory x{node{};&edge/1:node,{<<},node,head}.#false:-not &edge(2){(2),(Y): p(2,Y)}<<(2)."), rewrite(parse("#theory x{ node{}; &edge/1: node, {<<}, node, head }.&edge(z) { z, Y : p(z,Y)} << z. #const z=2.")));
}

void TestProgram::test_check() {
    CPPUNIT_ASSERT(check(parse("p(X):-q(X).")));
    CPPUNIT_ASSERT(!check(parse("p(X,Y,Z):-q(X).")));
    CPPUNIT_ASSERT_EQUAL(std::string(
        "-:1:1-16: error: unsafe variables in:\n"
        "  p(X,Y,Z):-[#inc_base];q(X).\n"
        "-:1:5-6: note: 'Y' is unsafe\n"
        "-:1:7-8: note: 'Z' is unsafe\n"), message());
    CPPUNIT_ASSERT(check(parse("p(X):-p(Y),X=Y+Y.")));
    CPPUNIT_ASSERT(check(parse("p(X):-p(Y),Y+Y=X.")));
    CPPUNIT_ASSERT(!check(parse("p(X):-p(Y),Y!=X.")));
    CPPUNIT_ASSERT(check(parse("p(X):-p(Y),p(1..Y)")));
    CPPUNIT_ASSERT(!check(parse("p(X):-X=#sum{Y,Z:p(Y)}.")));
    // body aggregates
    CPPUNIT_ASSERT(check(parse("p(X):-X=#sum{Y,Z:p(Y,Z)}.")));
    CPPUNIT_ASSERT_EQUAL(std::string(
        "-:1:7-23: error: unsafe variables in:\n"
        "  X=#sum{Y,Z:p(Y)}\n"
        "-:1:16-17: note: 'Z' is unsafe\n"), message());
    CPPUNIT_ASSERT(check(parse(":-p(Y),1#count{X:q(X)}Y.")));
    CPPUNIT_ASSERT(check(parse(":-p(Y),1{p(X):q(Y)}Y.")));
    CPPUNIT_ASSERT(check(parse(":-p(Y),p(X):q(X,Y).")));
    // head aggregates
    CPPUNIT_ASSERT(check(parse("1#count{X:p(X),q(X,Y)}Y:-r(Y).")));
    CPPUNIT_ASSERT(check(parse("1{p(X):q(X,Y)}Y:-r(Y).")));
    CPPUNIT_ASSERT(check(parse("p(X):q(X,Y):-r(Y).")));
    CPPUNIT_ASSERT(check(parse("#project p(X).")));
    CPPUNIT_ASSERT(!check(parse("#project p(X+X).")));
    CPPUNIT_ASSERT(check(parse("#edge (x,y).")));
    CPPUNIT_ASSERT(!check(parse("#edge (X,Y).")));
    CPPUNIT_ASSERT(check(parse("#edge (X,Y):p(X,Y).")));
    CPPUNIT_ASSERT(check(parse("#heuristic p(X). [2@X,sign]")));
    CPPUNIT_ASSERT(!check(parse("#heuristic p(X). [2@Y,sign]")));
    CPPUNIT_ASSERT(check(parse("#heuristic p(X) : p(Y). [2@Y,sign]")));
    CPPUNIT_ASSERT(check(parse("#theory x{ node{}; &edge/1: node, head }.&edge(Z) { X, Y : p(X,Y)}:-p(Z).")));
    CPPUNIT_ASSERT(check(parse("#theory x{ node{}; &edge/1: node, any }.&edge(Z) { X, Y : p(X,Y)}:-p(Z).")));
    CPPUNIT_ASSERT(!check(parse("#theory x{ node{}; &edge/1: node, body }.&edge(Z) { X, Y : p(X,Y)}:-p(Z).")));
    CPPUNIT_ASSERT(!check(parse("#theory x{ node{}; &edge/1: node, directive }.&edge(Z) { X, Y : p(X,Y)}:-p(Z).")));
    CPPUNIT_ASSERT(check(parse("#theory x{ node{}; &edge/1: node, body }.:-&edge(Z) { X, Y : p(X,Y)},p(Z).")));
    CPPUNIT_ASSERT(check(parse("#theory x{ node{}; &edge/1: node, any }.:-&edge(Z) { X, Y : p(X,Y)},p(Z).")));
    CPPUNIT_ASSERT(!check(parse("#theory x{ node{}; &edge/1: node, head }.:-&edge(Z) { X, Y : p(X,Y)},p(Z).")));
    CPPUNIT_ASSERT(!check(parse("#theory x{ node{}; &edge/1: node, directive }.:-&edge(Z) { X, Y : p(X,Y)},p(Z).")));
    CPPUNIT_ASSERT(check(parse("#theory x{ node{}; &edge/1: node, directive }.&edge(z) { X, Y : p(X,Y)}.")));
    CPPUNIT_ASSERT(!check(parse("#theory x{ node{}; &edge/1: node, head }.&edge(ZZ) { X, Y : p(X,Y)}:-p(Z).")));
    CPPUNIT_ASSERT(!check(parse("#theory x{ node{}; &edge/1: node, head }.&edge(Z) { XX, Y : p(X,Y)}:-p(Z).")));
    CPPUNIT_ASSERT(!check(parse("#theory x{ node{}; &edge/1: node, head }.&edge(Z) { XX, Y : p(X,Y)}.")));
    CPPUNIT_ASSERT(!check(parse("#theory x{ node{}; &edge/1: node, head }.&edge(z) { XX, Y : p(X,Y)}.")));
}

void TestProgram::test_projection() {
    CPPUNIT_ASSERT_EQUAL(std::string("x:-q;#p_p(#p).#p_p(#p):-p(#P0)."), rewrite(parse("x:-p(_),q.")));
    CPPUNIT_ASSERT_EQUAL(std::string("x:-#p_p(#p,#b(X),f);q.#p_p(#p,#b(#X1),f):-p(#P0,#X1,f)."), rewrite(parse("x:-q;p(_,X,f).")));
    CPPUNIT_ASSERT_EQUAL(std::string("x:-#p_p(#p);q.y:-#p_p(#p);q.#p_p(#p):-p(#P0)."), rewrite(parse("x:-q,p(_).y:-q,p(_).")));
    CPPUNIT_ASSERT_EQUAL(std::string("x:-#p_p(f(1,#p),2);q.y:-#p_p(f(#b(X),#p),g(#b(X)));q.#p_p(f(1,#p),2):-p(f(1,#P0),2).#p_p(f(#b(#X0),#p),g(#b(#X2))):-p(f(#X0,#P1),g(#X2))."), rewrite(parse("x:-q,p(f(1,_),2).y:-q,p(f(X,_),g(X)).")));
    CPPUNIT_ASSERT_EQUAL(std::string("x:-not #p_p(#p).#p_p(#p):-p(#P0)."), rewrite(parse("x:-not p(_).")));
    CPPUNIT_ASSERT_EQUAL(std::string("x:-#p_p(#p,1);q.y:-#p_p(1,#p);q.#p_p(#p,1):-p(#P0,1).#p_p(1,#p):-p(1,#P0)."), rewrite(parse("x:-q;p(_,1).y:-q;p(1,_).")));
    // NOTE: projection disabled for now
    CPPUNIT_ASSERT_EQUAL(std::string("x:-1<=#count{0,y:p(#Anon0),y}."), rewrite(parse("x:-1{y:p(_)}.")));
    CPPUNIT_ASSERT_EQUAL(std::string("x:-1<=#count{0,y:not #p_p(#p),y}.#p_p(#p):-p(#P0)."), rewrite(parse("x:-1{y:not p(_)}.")));
    CPPUNIT_ASSERT_EQUAL(std::string("#project p(#Anon0):-[p(#Anon0)]."), rewrite(parse("#project p(_).")));
    CPPUNIT_ASSERT_EQUAL(std::string("#edge(X,#Anon0)."), rewrite(parse("#edge (X,_).")));
    CPPUNIT_ASSERT_EQUAL(std::string("#heuristic p(#Anon0)[#Anon1@#Anon2,sign]:-[p(#Anon0)]."), rewrite(parse("#heuristic p(_). [_@_,sign]")));
    CPPUNIT_ASSERT_EQUAL(std::string("#theory x{node{};&edge/2:node,directive}.#false:-not &edge(#Anon0,X){(X),(Y): p(#Anon1,Y)}."), rewrite(parse("#theory x{ node{}; &edge/2: node, directive }.&edge(_,X) { X, Y : p(_,Y)}.")));
}

void TestProgram::test_statements() {
    CPPUNIT_ASSERT_EQUAL(std::string("#project p:-[p]."), rewrite(parse("#project p/0.")));
    CPPUNIT_ASSERT_EQUAL(std::string("#project p(X0):-[p(X0)]."), rewrite(parse("#project p/1.")));
    CPPUNIT_ASSERT_EQUAL(std::string("#project p(X0,X1,X2):-[p(X0,X1,X2)]."), rewrite(parse("#project p/3.")));
    CPPUNIT_ASSERT_EQUAL(std::string("#project p:-[p]."), rewrite(parse("#project p.")));
    CPPUNIT_ASSERT_EQUAL(std::string("#project p(X,Y):-[p(X,Y)]."), rewrite(parse("#project p(X,Y).")));
    CPPUNIT_ASSERT_EQUAL(std::string("#edge(a,b)."), rewrite(parse("#edge (a,b).")));
    CPPUNIT_ASSERT_EQUAL(std::string("#heuristic a[1@2,sign]:-[a]."), rewrite(parse("#heuristic a. [1@2,sign]")));
    CPPUNIT_ASSERT_EQUAL(std::string("#heuristic a[1@2,level]:-[a]."), rewrite(parse("#heuristic a. [1@2,level]")));
    CPPUNIT_ASSERT_EQUAL(std::string("#heuristic a[1@0,level]:-[a];c."), rewrite(parse("#heuristic a : c. [1,level]")));
}

void TestProgram::test_theory() {
    CPPUNIT_ASSERT_EQUAL(std::string("#theory x{}."), rewrite(parse("#theory x{  }.")));
    CPPUNIT_ASSERT_EQUAL(std::string("#theory x{node{};&edge/0:node,directive}."), rewrite(parse("#theory x{ node{}; &edge/0: node, directive }.")));
    CPPUNIT_ASSERT_EQUAL(std::string("#theory x{node{};&edge/0:node,directive}.#false:-not &edge{(X),(Y): p(X,Y)}."), rewrite(parse("#theory x{ node{}; &edge/0: node, directive }.&edge { X, Y : p(X,Y)}.")));
    CPPUNIT_ASSERT_EQUAL(std::string("#theory x{node{};&edge/0:node,directive}.#false:-not &edge{(X),(Y): p(X,Y)}."), rewrite(parse("#theory x{ node{}; &edge/0: node, directive }.&edge { X, Y : p(X,Y)}.")));
    std::string theory =
        "#theory csp{\n"
        "    term {\n"
        "        < : 0,binary,left;\n"
        "        + : 1,binary,left;\n"
        "        - : 1,binary,left;\n"
        "        * : 2,binary,left;\n"
        "        / : 2,binary,left;\n"
        "        ^ : 3,binary,right;\n"
        "        - : 4,unary\n"
        "    };\n"
        "    &linear/0: term, any\n"
        "}.\n";
    std::string parsed = "#theory csp{term{< :0,binary,left,+ :1,binary,left,- :1,binary,left,* :2,binary,left,/ :2,binary,left,^ :3,binary,right,- :4,unary};&linear/0:term,any}.";
    CPPUNIT_ASSERT_EQUAL(parsed, rewrite(parse(theory)));
    CPPUNIT_ASSERT_EQUAL(parsed+"#false:-not &linear{(((1)<(((2)+(3))-(((4)*(5))/((6)^((-(7))^(8))))))<(9))}.", rewrite(parse(theory + "&linear { 1 < 2+3-4*5/6^ -7^8 < 9 }.")));
}

TestProgram::~TestProgram() { }

// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestProgram);

} } } // namespace Test Input Gringo

