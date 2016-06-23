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

#include "tests/term_helper.hh"
#include "tests/tests.hh"
#include "tests/term_helper.hh"
#include "tests/output/solver_helper.hh"

namespace Gringo { namespace Output { namespace Test {

using namespace Gringo::Test;

// {{{ definition of auxiliary functions

namespace {

std::string iground(std::string in, int last = 3) {
    std::stringstream ss;
    Gringo::Test::TestGringoModule module;
    Potassco::TheoryData td;
    Output::OutputBase out(td, {}, ss, OutputFormat::INTERMEDIATE);
    Input::Program prg;
    Defines defs;
    Scripts scripts(module);
    Input::NongroundProgramBuilder pb(scripts, prg, out, defs);
    Input::NonGroundParser parser(pb);
    parser.pushStream("-", gringo_make_unique<std::stringstream>(in), module.logger);
    Models models;
    parser.parse(module.logger);
    prg.rewrite(defs, module.logger);
    prg.check(module.logger);
    //std::cerr << prg;
    // TODO: think about passing params to toGround already...
    if (!module.logger.hasError()) {
        out.init(true);
        {
            Ground::Parameters params;
            params.add("base", {});
            out.beginStep();
            prg.toGround(out.data, module.logger).ground(params, scripts, out, true, module.logger);
            out.reset();
        }
        for (int i=1; i < last; ++i) {
            Ground::Parameters params;
            params.add("step", {NUM(i)});
            out.beginStep();
            prg.toGround(out.data, module.logger).ground(params, scripts, out, true, module.logger);
            out.reset();
        }
        {
            Ground::Parameters params;
            params.add("last", {});
            out.beginStep();
            prg.toGround(out.data, module.logger).ground(params, scripts, out, true, module.logger);
            out.reset();
        }
    }
    return ss.str();
}

} // namespace

// }}}
// {{{ definition of TestIncremental

TEST_CASE("output-incremental", "[output]") {
    SECTION("assign") {
        REQUIRE(
            "asp 1 0 0 incremental\n"
            "0\n"
            "1 0 1 1 0 0\n"
            "4 8 p(1,0,0) 0\n"
            "0\n"
            "1 0 1 2 0 0\n"
            "4 8 p(2,0,0) 0\n"
            "0\n"
            "0\n" == iground(
                "#program base."
                "#program step(k)."
                "p(k,N,Y) :- N = #sum { Y }, Y = #sum { }."
                "#program last."));
        REQUIRE(
            "asp 1 0 0 incremental\n"
            "0\n"
            "1 0 1 1 0 0\n"
            "4 8 p(1,2,1) 0\n"
            "0\n"
            "1 0 1 2 0 0\n"
            "4 8 p(2,4,2) 0\n"
            "0\n"
            "0\n" == iground(
                "#program base."
                "#program step(k)."
                "p(k,N,Y) :- N = #sum { 2*Y }, Y = #sum { k }."
                "#program last."));
    }
    SECTION("projection") {
        REQUIRE(
            "asp 1 0 0 incremental\n"
            "1 1 1 1 0 0\n"
            "1 1 1 2 0 0\n"
            "1 1 1 3 0 0\n"
            "1 0 1 4 0 1 1\n"
            "1 0 1 4 0 1 2\n"
            "1 0 1 4 0 1 3\n"
            "4 4 p(1) 1 1\n"
            "4 4 p(2) 1 2\n"
            "4 4 p(3) 1 3\n"
            "0\n"
            "1 0 1 5 0 1 4\n"
            "1 1 1 6 0 1 5\n"
            "4 4 q(1) 1 6\n"
            "0\n"
            "1 0 1 7 0 1 5\n"
            "1 1 1 8 0 1 7\n"
            "4 4 q(2) 1 8\n"
            "0\n"
            "1 0 1 9 0 1 7\n"
            "0\n" == iground(
                "#program base."
                "{p(1..3)}."
                "#program step(k)."
                "{q(k)} :- p(_)."
                "#program last."));
    }

    SECTION("lp") {
        REQUIRE(
            "asp 1 0 0 incremental\n"
            "4 4 base 1 -1\n"
            "0\n"
            "1 0 1 2 0 0\n"
            "4 4 r(1) 0\n"
            "4 4 p(1) 1 -1\n"
            "4 4 q(1) 1 -1\n"
            "0\n"
            "1 0 1 3 0 0\n"
            "4 4 r(2) 0\n"
            "4 4 p(2) 1 -1\n"
            "4 4 q(2) 1 -1\n"
            "0\n"
            "4 4 last 1 -1\n"
            "0\n" == iground(
                "#program base."
                "#show base."
                "#program step(k)."
                "#show p(k)."
                "#show q(k)."
                "r(k)."
                "#program last."
                "#show last."));
        REQUIRE(
            "asp 1 0 0 incremental\n"
            "1 0 1 1 0 0\n"
            "4 4 base 0\n"
            "0\n"
            "1 0 1 2 0 0\n"
            "4 7 step(1) 0\n"
            "0\n"
            "1 0 1 3 0 0\n"
            "4 7 step(2) 0\n"
            "0\n"
            "1 0 1 4 0 0\n"
            "4 4 last 0\n"
            "0\n" == iground(
                "#program base."
                "base."
                "#program step(k)."
                "step(k)."
                "#program last."
                "last."));
    }
    SECTION("csp") {
        REQUIRE(
            "asp 1 0 0 incremental\n"
            "4 6 base=1 0\n"
            "0\n"
            "1 0 1 1 0 0\n"
            "1 1 1 2 0 1 3\n"
            "1 1 1 3 0 1 4\n"
            "1 1 1 4 0 1 5\n"
            "1 1 1 5 0 0\n"
            "1 1 1 6 0 1 7\n"
            "1 1 1 7 0 1 8\n"
            "1 1 1 8 0 0\n"
            "1 1 1 9 0 1 10\n"
            "1 1 1 10 0 0\n"
            "4 4 up=0 1 2\n"
            "4 4 up=1 2 3 -2\n"
            "4 4 up=2 2 4 -3\n"
            "4 4 up=3 2 5 -4\n"
            "4 4 up=4 1 -5\n"
            "4 4 lo=1 1 6\n"
            "4 4 lo=2 2 7 -6\n"
            "4 4 lo=3 2 8 -7\n"
            "4 4 lo=4 1 -8\n"
            "4 4 ib=1 1 9\n"
            "4 4 ib=2 2 10 -9\n"
            "4 4 ib=3 1 -10\n"
            "4 7 step(1) 0\n"
            "0\n"
            "1 0 1 11 0 0\n"
            "1 0 0 0 1 -5\n"
            "1 0 0 0 1 6\n"
            "1 0 0 0 2 -9 10\n"
            "4 7 step(2) 0\n"
            "0\n"
            "4 6 last=1 0\n"
            "0\n" == iground(
                "#program base."
                "$base$=1."
                "#program step(k)."
                "step(k)."
                "0 $<= $up $<= 5-k."
                "k $<= $lo $<= 4."
                "1 $<= $ib $<= 3."
                "$ib $= 1; $ib $=3 :- k == 2."
                "#program last."
                "$last$=1."));
    }

    SECTION("projectionBug") {
        REQUIRE(
            "asp 1 0 0 incremental\n"
            "1 1 1 1 0 0\n"
            "1 0 1 2 0 1 1\n"
            "4 6 p(0,0) 1 1\n"
            "0\n"
            "1 0 1 3 0 1 2\n"
            "1 1 1 4 0 1 3\n"
            "1 0 1 5 0 1 4\n"
            "4 6 p(1,1) 1 4\n"
            "0\n"
            "1 0 1 6 0 1 3\n"
            "1 0 1 7 0 1 5\n"
            "1 1 1 8 0 1 7\n"
            "1 0 1 9 0 1 8\n"
            "4 6 p(2,2) 1 8\n"
            "0\n"
            "1 0 1 10 0 1 6\n"
            "1 0 1 11 0 1 7\n"
            "1 0 1 12 0 1 9\n"
            "1 1 1 13 0 0\n"
            "1 0 1 10 0 1 13\n"
            "1 1 1 14 0 1 10\n"
            "1 1 1 15 0 1 11\n"
            "1 1 1 16 0 1 12\n"
            "4 6 p(1,0) 1 13\n"
            "4 4 r(0) 1 14\n"
            "4 4 r(1) 1 15\n"
            "4 4 r(2) 1 16\n"
            "0\n" == iground(
                "#program base."
                "{p(0,0)}."
                "#program step(k)."
                "{p(k,k)} :- p(_,k-1)."
                "#program last."
                "{p(1,0)}."
                "{r(X)} :- p(_,X)."
                ));
    }

    SECTION("mapping") {
        Mapping m;
        m.add(1,0);
        m.add(2,1);
        m.add(4,2);
        m.add(5,3);

        REQUIRE(InvalidId == m.get(0));
        REQUIRE(Id_t(0) == m.get(1));
        REQUIRE(Id_t(1) == m.get(2));
        REQUIRE(InvalidId == m.get(3));
        REQUIRE(Id_t(2) == m.get(4));
        REQUIRE(Id_t(3) == m.get(5));
        REQUIRE(InvalidId == m.get(6));

        REQUIRE(Id_t(0) == m.bound(0));
        REQUIRE(Id_t(0) == m.bound(1));
        REQUIRE(Id_t(1) == m.bound(2));
        REQUIRE(Id_t(2) == m.bound(3));
        REQUIRE(Id_t(2) == m.bound(4));
        REQUIRE(Id_t(3) == m.bound(5));
        REQUIRE(Id_t(4) == m.bound(6));
    }
}

} } } // namespace Test Output Gringo
