//
// Copyright (c) 2015, Benjamin Kaufmann
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
#include "clingo.hh"
#include <vector>
#include <iostream>

using namespace Clingo;

using AtomVec = std::vector<Symbol>;
using ModelVec = std::vector<AtomVec>;
using MessageVec = std::vector<std::pair<MessageCode, std::string>>;
using SymList = std::initializer_list<Symbol>;

struct MCB {
    MCB(ModelVec &models) : models(models) { }
    bool operator()(Model m) {
        models.emplace_back();
        for (auto sym : m.atoms(ShowType::All)) {
            models.back().emplace_back(sym);
        }
        std::sort(models.back().begin(), models.back().end());
        return true;
    }
    ModelVec &models;
};

class LCB {
public:
    LCB(MessageVec &messages) : messages_(messages) { }
    void operator()(MessageCode code, char const *msg) { messages_.emplace_back(code, msg); }
private:
    MessageVec &messages_;
};

TEST_CASE("c-interface", "[clingo]") {
    using S = std::string;

    SECTION("symbol") {
        std::vector<Symbol> args;
        // numbers
        auto sym = Num(42);
        REQUIRE(42 == sym.num());
        args.emplace_back(sym);
        // inf
        sym = Inf();
        REQUIRE(SymbolType::Inf == sym.type());
        args.emplace_back(sym);
        // sup
        sym = Sup();
        REQUIRE(SymbolType::Sup == sym.type());
        args.emplace_back(sym);
        // str
        sym = Str("x");
        REQUIRE(S("x") == sym.string());
        args.emplace_back(sym);
        // id
        sym = Id("x", true);
        REQUIRE(SymbolType::Fun == sym.type());
        REQUIRE(sym.sign());
        REQUIRE(S("x") == sym.name());
        args.emplace_back(sym);
        // fun
        sym = Fun("f", args);
        REQUIRE(SymbolType::Fun == sym.type());
        REQUIRE(!sym.sign());
        REQUIRE(S("f") == sym.name());
        REQUIRE("f(42,#inf,#sup,\"x\",-x)" == sym.toString());
        REQUIRE((args.size() == sym.args().size() && std::equal(args.begin(), args.end(), sym.args().begin())));
        REQUIRE_THROWS(sym.num());
        // comparison
        auto a = Num(1), b = Num(2);
        REQUIRE(a < b);
        REQUIRE_FALSE(a < a);
        REQUIRE_FALSE(b < a);
        REQUIRE(b > a);
        REQUIRE_FALSE(a > a);
        REQUIRE_FALSE(a > b);
        REQUIRE(a <= a);
        REQUIRE(a <= b);
        REQUIRE_FALSE(b <= a);
        REQUIRE(a >= a);
        REQUIRE(b >= a);
        REQUIRE_FALSE(a >= b);
        REQUIRE(a == a);
        REQUIRE_FALSE(a == b);
        REQUIRE(a != b);
        REQUIRE_FALSE(a != a);
        REQUIRE(a.hash() == a.hash());
        REQUIRE(a.hash() != b.hash());
    }
    SECTION("with module") {
        Module mod;
        SECTION("with control") {
            MessageVec messages;
            Logger logger = [&messages](MessageCode code, char const *msg) { messages.emplace_back(code, msg); };
            Control ctl{mod.create_control({"test_libclingo"}, logger, 20)};
            SECTION("solve") {
                ctl.add("base", {}, "a.");
                ctl.ground({{"base", {}}});
                ModelVec models;
                REQUIRE(ctl.solve({}, MCB(models)).sat());
                REQUIRE(models == ModelVec{{Id("a")}});
                REQUIRE(messages.empty());
            }
            SECTION("solve_iter") {
                ctl.add("base", {}, "a.");
                ctl.ground({{"base", {}}});
                {
                    auto iter = ctl.solve_iter();
                    ModelVec models;
                    MCB mcb(models);
                    while (Model m = iter.next()) { mcb(m); }
                    REQUIRE(models == ModelVec{{Id("a")}});
                    REQUIRE(iter.get().sat());
                }
                ModelVec models;
                REQUIRE(ctl.solve({}, MCB(models)).sat());
                REQUIRE(models == ModelVec{{Id("a")}});
                REQUIRE(messages.empty());
            }
            SECTION("logging") {
                ctl.add("base", {}, "a(1+a).");
                ctl.ground({{"base", {}}});
                ModelVec models;
                REQUIRE(ctl.solve({}, MCB(models)).sat());
                REQUIRE(models == ModelVec{{}});
                REQUIRE(messages == MessageVec({{MessageCode::OperationUndefined, "<block>:1:3-6: info: operation undefined:\n  (1+a)\n"}}));
            }
            SECTION("ground callback") {
                ctl.add("base", {}, "a(@f(X)):- X=1..2. b(@f()).");
                ctl.ground({{"base", {}}}, [&messages](Location loc, char const *name, SymSpan args, SymSpanCallback report) {
                    if (strcmp(name, "f") == 0 && args.size() == 1) {
                        Symbol front = *args.begin();
                        report({Num(front.num() + 1), Num(front.num() + 10)});
                        report({Num(front.num() + 20)});
                    }
                    else {
                        std::ostringstream oss;
                        oss << "invalid call: " << name << "/" << args.size() << " at " << loc;
                        messages.emplace_back(MessageCode::OperationUndefined, oss.str());
                    }
                });
                ModelVec models;
                REQUIRE(ctl.solve({}, MCB(models)).sat());
                REQUIRE(models == ModelVec({{Fun("a", {Num(2)}), Fun("a", {Num(3)}), Fun("a", {Num(11)}), Fun("a", {Num(12)}), Fun("a", {Num(21)}), Fun("a", {Num(22)})}}));
                REQUIRE(messages == MessageVec({{MessageCode::OperationUndefined, "invalid call: f/0 at <block>:1:22-26"}}));
            }
            SECTION("ground callback fail") {
                ctl.add("base", {}, "a(@f()).");
                REQUIRE_THROWS_AS(ctl.ground({{"base", {}}}, [](Location, char const *, SymSpan, SymSpanCallback) { throw std::runtime_error("fail"); }), std::runtime_error);
            }
        }
    }
}

