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

#include "gringo/graph.hh"
#include "gringo/utility.hh"

#include "tests/tests.hh"

namespace Gringo { namespace Test {

using namespace Gringo::IO;

namespace {

typedef Graph<std::string> G;

std::ostream &operator<<(std::ostream &out, G::SCCVec const &x) {
    auto f = [](std::ostream &out, G::NodeVec const x) {
        auto f = [](std::ostream &out, G::Node const *x) { out << x->data; };
        out << "[";
        print_comma(out, x, ",", f);
        out << "]";
    };
    out << "[";
    print_comma(out, x, ",", f);
    out << "]";
    return out;
}

std::string to_string(G::SCCVec const &x) {
    std::ostringstream out;
    out << x;
    return out.str();
}

G graph(std::initializer_list<std::pair<std::string, std::string>> edges)
{
    G g;
    std::map<std::string, G::Node*> map;
    for (auto &x : edges) {
        auto &a = map[x.first];
        if (!a) { a = &g.insertNode(x.first); }
        auto &b = map[x.second];
        if (!b) { b = &g.insertNode(x.second); }
        a->insertEdge(*b);
    }
    return g;
}

} // namespace

TEST_CASE("graph", "[base]") {
    SECTION("test_tarjan") {
        REQUIRE("[[b,c],[d],[a]]" == to_string(graph({{"a","b"},{"a","d"},{"b","c"},{"c","b"},{"d","b"}}).tarjan()));
        REQUIRE("[[h,i],[f,g],[d,c,b,e],[a]]" == to_string(graph({{"a","b"},{"b","c"},{"c","h"},{"c","d"},{"d","e"},{"e","f"},{"e","b"},{"e","c"},{"f","g"},{"g","f"},{"h","i"},{"i","h"}}).tarjan()));
        REQUIRE("[[c,d],[a,b]]" == to_string(graph({{"a","b"},{"a","c"},{"a","d"},{"b","a"},{"c","d"},{"d","c"}}).tarjan()));
        REQUIRE("[[i],[n],[r3],[r4,e,r1,p,r2,s]]" == to_string(graph({{"r1","e"},{"r2","p"},{"r2","i"},{"r3","n"},{"r3","i"},{"r4","s"},{"p","r1"},{"s","r2"},{"s","r3"},{"e","r4"}}).tarjan()));
        G g{graph({{"a","b"},{"a","d"},{"b","c"},{"c","b"},{"d","b"}})};
        REQUIRE("[[b,c],[d],[a]]" == to_string(g.tarjan()));
        REQUIRE("[[b,c],[d],[a]]" == to_string(g.tarjan()));
        REQUIRE("[[b,c],[d],[a]]" == to_string(g.tarjan()));
    }
}

} } // namespace Test Gringo

