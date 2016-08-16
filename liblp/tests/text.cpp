// 
// Copyright (c) 2016, Benjamin Kaufmann
// 
// This file is part of Potassco. See http://potassco.sourceforge.net/
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 

#include "catch.hpp"
#include "common.h"
#include <potassco/aspif_text.h>
#include <potassco/aspif.h>
#include <sstream>
namespace Potassco {
namespace Test {
namespace Text {

bool read(AspifTextInput& in, std::stringstream& str) {
	return in.accept(str) && in.parse();
}
TEST_CASE("Text reader ", "[text]") {
	std::stringstream input, output;
	AspifOutput out(output);
	AspifTextInput prg(&out);

	SECTION("read empty") {
		REQUIRE(read(prg, input));
		REQUIRE(output.str() == "asp 1 0 0\n0\n");
	}
	SECTION("read fact") {
		input << "x1.";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("1 0 1 1 0 0") != std::string::npos);
	}
	SECTION("read basic rule") {
		input << "x1 :- not   x2.";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("1 0 1 1 0 1 -2") != std::string::npos);
	}
	SECTION("read choice rule") {
		input << "{x1} :- not x2.\n";
		input << "{x2, x3}.";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("1 1 1 1 0 1 -2") != std::string::npos);
		REQUIRE(output.str().find("1 1 2 2 3 0 0") != std::string::npos);
	}
	SECTION("read disjunctive rule") {
		input << "x1 | x2 :- not x3.";
		input << "x1 ; x2 :- not x4.";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("1 0 2 1 2 0 1 -3") != std::string::npos);
		REQUIRE(output.str().find("1 0 2 1 2 0 1 -4") != std::string::npos);
	}
	SECTION("read weight rule") {
		input << "x1 :- 2 {x2, x3=2, not x4 = 3, x5}.";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("1 0 1 1 1 2 4 2 1 3 2 -4 3 5 1") != std::string::npos);
	}
	SECTION("read alternative atom names") {
		input << "a :- not b, x_3.";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("1 0 1 1 0 2 -2 3") != std::string::npos);
	}
	SECTION("read integrity constraint") {
		input << ":- x1, not x2.";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("1 0 0 0 2 1 -2") != std::string::npos);
	}
	SECTION("read minimize constraint") {
		input << "#minimize {x1, x2, x3}.\n";
		input << "#minimize {not x1=2, x4, not x5 = 3}@1.\n";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("2 0 3 1 1 2 1 3 1") != std::string::npos);
		REQUIRE(output.str().find("2 1 3 -1 2 4 1 -5 3") != std::string::npos);
	}
	SECTION("read project") {
		input << "#project {a,x2}.";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("3 2 1 2") != std::string::npos);
	}
	SECTION("read output") {
		input << "#output foo (1, \"x1, x2\") : x1, x2.";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("4 15 foo(1,\"x1, x2\") 2 1 2") != std::string::npos);
	}
	SECTION("read external") {
		input << "#external x1.\n";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("5 1 2") != std::string::npos);
	}
	SECTION("read external with value") {
		input << "#external x2. [true]\n";
		input << "#external x3. [false]\n";
		input << "#external x4. [free]\n";
		input << "#external x5. [release]\n";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("5 2 1") != std::string::npos);
		REQUIRE(output.str().find("5 3 2") != std::string::npos);
		REQUIRE(output.str().find("5 4 0") != std::string::npos);
		REQUIRE(output.str().find("5 5 3") != std::string::npos);
	}
	SECTION("read external with unknown value") {
		input << "#external x2. [open]\n";
		REQUIRE_THROWS(read(prg, input));
	}
	SECTION("read assume") {
		input << "#assume {a, not x2}.";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("6 2 1 -2") != std::string::npos);
	}
	SECTION("read heuristic") {
		input << "#heuristic x1. [1, level]";
		input << "#heuristic x2 : x1. [2@1, true]";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("7 0 1 1 0 0") != std::string::npos);
		REQUIRE(output.str().find("7 4 2 2 1 1 1") != std::string::npos);
	}
	SECTION("read edge") {
		input << "#edge (1,2) : x1.";
		input << "#edge (2,1).";
		REQUIRE(read(prg, input));
		REQUIRE(output.str().find("8 1 2 1 1") != std::string::npos);
		REQUIRE(output.str().find("8 2 1 0") != std::string::npos);
	}
	SECTION("read incremental") {
		input << "#incremental.\n";
		input << "{x1}.\n";
		input << "#step.\n";
		input << "{x2}.\n";
		REQUIRE(read(prg, input));
		REQUIRE(prg.parse());
		REQUIRE(output.str() == 
			"asp 1 0 0 incremental\n"
			"1 1 1 1 0 0\n"
			"0\n"
			"1 1 1 2 0 0\n"
			"0\n");
	}
}


}}}
