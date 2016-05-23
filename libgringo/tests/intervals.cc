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

#include "gringo/intervals.hh"
#include "gringo/utility.hh"

#include "tests/tests.hh"

namespace Gringo { namespace Test {

using IS = IntervalSet<int>;
using ES = enum_interval_set<int>;
using LB = IS::LBound;
using RB = IS::RBound;
using IV = IS::Interval;
using S = std::string;

namespace {

LB lb(int x, bool y) { return {x, y}; };
RB rb(int x, bool y) { return {x, y}; };
std::string print(IS const &x) {
    auto f = [](std::ostream &out, IV const &x) {
        out
            << (x.left.inclusive ? "[" : "(")
            << x.left.bound
            << ","
            << x.right.bound
            << (x.right.inclusive ? "]" : ")");
    };
    std::ostringstream out;
    out << "{";
    print_comma(out, x.vec, ",", f);
    out << "}";
    return out.str();
}

std::string print(ES const &x) {
    std::ostringstream out;
    out << "{";
    if (!x.empty()) {
        int a = *x.begin();
        int b = *x.begin() - 1;
        for (auto y : x) {
            if (b+1 != y) {
                if (a <= b) { out << "[" << a << "," << b << "],"; }
                a = y;
            }
            b = y;
        }
        out << "[" << a << "," << b << "]";
    }
    out << "}";
    return out.str();
}

}

TEST_CASE("intervals", "[base]") {
    SECTION("boundLess") {
        REQUIRE(!(lb(2,true)  < lb(1,true)));
        REQUIRE(!(lb(2,true)  < lb(1,false)));
        REQUIRE(!(lb(2,false) < lb(1,true)));
        REQUIRE(!(lb(2,false) < lb(1,false)));

        REQUIRE( (lb(1,true)  < lb(2,true)));
        REQUIRE( (lb(1,true)  < lb(2,false)));
        REQUIRE( (lb(1,false) < lb(2,true)));
        REQUIRE( (lb(1,false) < lb(2,false)));

        REQUIRE(!(lb(1,true)  < lb(1,true)));
        REQUIRE( (lb(1,true)  < lb(1,false)));
        REQUIRE(!(lb(1,false) < lb(1,true)));
        REQUIRE(!(lb(1,false) < lb(1,false)));
        // same for RB
        REQUIRE(!(rb(2,true)  < rb(1,true)));
        REQUIRE(!(rb(2,true)  < rb(1,false)));
        REQUIRE(!(rb(2,false) < rb(1,true)));
        REQUIRE(!(rb(2,false) < rb(1,false)));

        REQUIRE( (rb(1,true)  < rb(2,true)));
        REQUIRE( (rb(1,true)  < rb(2,false)));
        REQUIRE( (rb(1,false) < rb(2,true)));
        REQUIRE( (rb(1,false) < rb(2,false)));

        REQUIRE(!(rb(1,true)  < rb(1,true)));
        REQUIRE(!(rb(1,true)  < rb(1,false)));
        REQUIRE( (rb(1,false) < rb(1,true)));
        REQUIRE(!(rb(1,false) < rb(1,false)));
    }

    SECTION("boundLessEq") {
        REQUIRE(!(lb(2,true)  <= lb(1,true)));
        REQUIRE(!(lb(2,true)  <= lb(1,false)));
        REQUIRE(!(lb(2,false) <= lb(1,true)));
        REQUIRE(!(lb(2,false) <= lb(1,false)));

        REQUIRE( (lb(1,true)  <= lb(2,true)));
        REQUIRE( (lb(1,true)  <= lb(2,false)));
        REQUIRE( (lb(1,false) <= lb(2,true)));
        REQUIRE( (lb(1,false) <= lb(2,false)));

        REQUIRE( (lb(1,true)  <= lb(1,true)));
        REQUIRE( (lb(1,true)  <= lb(1,false)));
        REQUIRE(!(lb(1,false) <= lb(1,true)));
        REQUIRE( (lb(1,false) <= lb(1,false)));
        // same for RB
        REQUIRE(!(rb(2,true)  <= rb(1,true)));
        REQUIRE(!(rb(2,true)  <= rb(1,false)));
        REQUIRE(!(rb(2,false) <= rb(1,true)));
        REQUIRE(!(rb(2,false) <= rb(1,false)));

        REQUIRE( (rb(1,true)  <= rb(2,true)));
        REQUIRE( (rb(1,true)  <= rb(2,false)));
        REQUIRE( (rb(1,false) <= rb(2,true)));
        REQUIRE( (rb(1,false) <= rb(2,false)));

        REQUIRE( (rb(1,true)  <= rb(1,true)));
        REQUIRE(!(rb(1,true)  <= rb(1,false)));
        REQUIRE( (rb(1,false) <= rb(1,true)));
        REQUIRE( (rb(1,false) <= rb(1,false)));
    }

    SECTION("boundBefore") {
        // before with gap
        // {
        //  }
        REQUIRE((lb(1,true)  < rb(2,true)));
        REQUIRE((lb(1,true)  < rb(2,false)));
        REQUIRE((lb(1,false) < rb(2,true)));
        REQUIRE((lb(1,false) < rb(2,false)));
        // [        {           {
        // ] - gap  ) - no gap  ) - no gap
        REQUIRE( (lb(1,true)  < rb(1,true)));
        REQUIRE(!(lb(1,true)  < rb(1,false)));
        REQUIRE(!(lb(1,false) < rb(1,true)));
        REQUIRE(!(lb(1,false) < rb(1,false)));
        //  {
        // }
        REQUIRE(!(lb(2,true)  < rb(1,true)));
        REQUIRE(!(lb(2,true)  < rb(1,false)));
        REQUIRE(!(lb(2,false) < rb(1,true)));
        REQUIRE(!(lb(2,false) < rb(1,false)));
        // the other way round
        // }
        //  {
        REQUIRE((rb(1,true)  < lb(2,true)));
        REQUIRE((rb(1,true)  < lb(2,false)));
        REQUIRE((rb(1,false) < lb(2,true)));
        REQUIRE((rb(1,false) < lb(2,false)));
        // }           ]           )
        // [ - no gap  { - no gap  ( - gap
        REQUIRE(!(rb(1,true)  < lb(1,true)));
        REQUIRE(!(rb(1,true)  < lb(1,false)));
        REQUIRE(!(rb(1,false) < lb(1,true)));
        REQUIRE( (rb(1,false) < lb(1,false)));
        //  }
        // {
        REQUIRE(!(rb(2,true)  < lb(1,true)));
        REQUIRE(!(rb(2,true)  < lb(1,false)));
        REQUIRE(!(rb(2,false) < lb(1,true)));
        REQUIRE(!(rb(2,false) < lb(1,false)));
    }

    SECTION("add") {
        IS x;
        x.add(1,true,1,true);
        REQUIRE("{[1,1]}" == print(x));
        x.add(3,true,2,true);
        REQUIRE("{[1,1]}" == print(x));
        x.add(3,true,4,false);
        REQUIRE("{[1,1],[3,4)}" == print(x));
        x.add(2,false,3,false);
        REQUIRE("{[1,1],(2,4)}" == print(x));
        x.add(1,false,2,false);
        REQUIRE("{[1,2),(2,4)}" == print(x));
        x.add(2,true,2,true);
        REQUIRE("{[1,4)}" == print(x));
        x.add(4,true,5,true);
        REQUIRE("{[1,5]}" == print(x));
        x.add(8,false,9,true);
        REQUIRE("{[1,5],(8,9]}" == print(x));
        x.add(11,false,12,true);
        REQUIRE("{[1,5],(8,9],(11,12]}" == print(x));
        x.add(13,false,14,true);
        REQUIRE("{[1,5],(8,9],(11,12],(13,14]}" == print(x));
        x.add(10,true,11,false);
        REQUIRE("{[1,5],(8,9],[10,11),(11,12],(13,14]}" == print(x));
        x.add(9,true,11,true);
        REQUIRE("{[1,5],(8,12],(13,14]}" == print(x));
        x.add(0,true,13,false);
        REQUIRE("{[0,13),(13,14]}" == print(x));
        x.add(-1,true,42,false);
        REQUIRE("{[-1,42)}" == print(x));
        x.add(42,true,43,true);
        REQUIRE("{[-1,43]}" == print(x));
    }

    SECTION("remove") {
        IS x;
        x.add(1,true,50,true);
        REQUIRE("{[1,50]}" == print(x));
        x.remove(1,true,2,true);
        REQUIRE("{(2,50]}" == print(x));
        x.remove(49,false,50,true);
        REQUIRE("{(2,49]}" == print(x));
        x.remove(5,true,6,false);
        REQUIRE("{(2,5),[6,49]}" == print(x));
        x.remove(8,false,9,true);
        REQUIRE("{(2,5),[6,8],(9,49]}" == print(x));
        IS a(x), b(x), c(x);
        a.remove(2,false,13,true);
        REQUIRE("{(13,49]}" == print(a));
        b.remove(4,false,8,true);
        REQUIRE("{(2,4],(9,49]}" == print(b));
        c.remove(4,false,13,true);
        REQUIRE("{(2,4],(13,49]}" == print(c));
    }

    SECTION("contains") {
        IS x;
        x.add(5,true,10,false);
        x.add(1,true,4,false);
        REQUIRE( x.contains({{5,true},{10,false}}));
        REQUIRE(!x.contains({{5,true},{10,true}}));
        REQUIRE( x.contains({{7,true},{8,true}}));
        REQUIRE( x.contains({{5,false},{6,false}}));
    }

    SECTION("intersects") {
        IS x;
        x.add(1,true,4,false);
        x.add(5,true,10,false);
        x.add(11,false,12,false);
        REQUIRE( x.intersects({{5,true},{10,false}}));
        REQUIRE( x.intersects({{5,true},{10,true}}));
        REQUIRE( x.intersects({{7,true},{8,true}}));
        REQUIRE(!x.intersects({{10,true},{11,true}}));
        REQUIRE( x.intersects({{10,true},{12,true}}));
        REQUIRE( x.intersects({{2,true},{7,true}}));
        REQUIRE( x.intersects({{4,true},{7,true}}));
        REQUIRE(!x.intersects({{0,true},{1,false}}));
        REQUIRE( x.intersects({{0,true},{1,true}}));
        REQUIRE(!x.intersects({{12,true},{13,true}}));
    }

    SECTION("intersect") {
        IS x, y;
        x.add({{2,true},{8,false}});
        x.add({{9,false},{13,true}});

        y.add({{1,true},{3,true}});
        y.add({{4,false},{5,true}});
        y.add({{7,true},{10,false}});
        y.add({{11,true},{12,false}});
        REQUIRE("{[2,3],(4,5],[7,8),(9,10),[11,12)}" == print(x.intersect(y)));
        REQUIRE("{[2,3],(4,5],[7,8),(9,10),[11,12)}" == print(y.intersect(x)));
    }

    SECTION("difference") {
        IS x, y;
        x.add({{2,true},{8,false}});
        x.add({{9,false},{13,true}});

        y.add({{1,true},{3,true}});
        y.add({{4,false},{5,true}});
        y.add({{7,true},{10,false}});
        y.add({{11,true},{12,false}});
        REQUIRE("{(3,4],(5,7),[10,11),[12,13]}" == print(x.difference(y)));
        REQUIRE("{[1,2),[8,9]}" == print(y.difference(x)));
    }

    SECTION("enum_empty") {
        ES x;
        REQUIRE(x.empty());
        REQUIRE(x.begin() == x.end());
    }

    SECTION("enum_add") {
        ES x;
        x.add(1,2);
        REQUIRE("{[1,1]}" == print(x));
        x.add(3,3);
        REQUIRE("{[1,1]}" == print(x));
        x.add(3,4);
        REQUIRE("{[1,1],[3,3]}" == print(x));
        x.add(2,3);
        REQUIRE("{[1,3]}" == print(x));
        x.add(4,6);
        REQUIRE("{[1,5]}" == print(x));
        x.add(9,10);
        REQUIRE("{[1,5],[9,9]}" == print(x));
        x.add(12,13);
        REQUIRE("{[1,5],[9,9],[12,12]}" == print(x));
        x.add(14,16);
        REQUIRE("{[1,5],[9,9],[12,12],[14,15]}" == print(x));
        x.add(10,11);
        REQUIRE("{[1,5],[9,10],[12,12],[14,15]}" == print(x));
        x.add(9,12);
        REQUIRE("{[1,5],[9,12],[14,15]}" == print(x));
        x.add(0,14);
        REQUIRE("{[0,15]}" == print(x));
        x.add(-1,42);
        REQUIRE("{[-1,41]}" == print(x));
        x.add(42,44);
        REQUIRE("{[-1,43]}" == print(x));
    }

    SECTION("enum_contains") {
        ES x;
        x.add(5,10);
        x.add(1,4);
        REQUIRE( x.contains(5,10));
        REQUIRE(!x.contains(5,11));
        REQUIRE( x.contains(7,9));
        REQUIRE( x.contains(6,6));
    }

    SECTION("enum_remove") {
        ES x;
        x.add(1,51);
        REQUIRE("{[1,50]}" == print(x));
        x.remove(1,3);
        REQUIRE("{[3,50]}" == print(x));
        x.remove(50,51);
        REQUIRE("{[3,49]}" == print(x));
        x.remove(5,6);
        REQUIRE("{[3,4],[6,49]}" == print(x));
        x.remove(9,10);
        REQUIRE("{[3,4],[6,8],[10,49]}" == print(x));
        ES a(x), b(x), c(x);
        a.remove(3,14);
        REQUIRE("{[14,49]}" == print(a));
        b.remove(5,9);
        REQUIRE("{[3,4],[10,49]}" == print(b));
        c.remove(5,14);
        REQUIRE("{[3,4],[14,49]}" == print(c));
    }

    SECTION("enum_intersects") {
        ES x;
        x.add(1,3);
        x.add(5,9);
        x.add(12,13);
        REQUIRE( x.intersects(5,9));
        REQUIRE( x.intersects(5,10));
        REQUIRE( x.intersects(7,8));
        REQUIRE(!x.intersects(10,12));
        REQUIRE( x.intersects(10,13));
        REQUIRE( x.intersects(2,7));
        REQUIRE( x.intersects(4,7));
        REQUIRE(!x.intersects(0,0));
        REQUIRE( x.intersects(0,2));
        REQUIRE(!x.intersects(13,14));
    }

}

} } // namespace Test Gringo

