// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright Roland Kaminski

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

#include "tests.hh"
#include <iostream>
#include <fstream>

namespace Clingo { namespace Test {

TEST_CASE("solving", "[clingo]") {
    SECTION("with module") {
        Module mod;
        SECTION("with control") {
            MessageVec messages;
            ModelVec models;
            Logger logger = [&messages](MessageCode code, char const *msg) { messages.emplace_back(code, msg); };
            Control ctl{mod.create_control({"test_libclingo", "0"}, logger, 20)};
            SECTION("solve") {
                SECTION("add") { ctl.add("base", {}, "{a}."); }
                SECTION("load") {
                    struct Temp {
                        Temp() : file(std::tmpnam(temp)) { }
                        ~Temp() { if (file) { std::remove(temp); } }
                        char temp[L_tmpnam];
                        char *file;
                    } t;
                    REQUIRE(t.file != nullptr);
                    std::ofstream(t.file) << "{a}.\n";
                    ctl.load(t.file);
                }
                ctl.ground({{"base", {}}});
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(models == ModelVec({{},{Id("a")}}));
                REQUIRE(messages.empty());
            }
            SECTION("statistics") {
                ctl.add("pigeon", {"p", "h"}, "1 {p(P,H) : P=1..p}1 :- H=1..h."
                                              "1 {p(P,H) : H=1..h}1 :- P=1..p.");
                ctl.ground({{"pigeon", {Num(6), Num(5)}}});
                REQUIRE(ctl.solve(MCB(models)).unsat());
                std::vector<std::string> keys_root;
                auto stats = ctl.statistics();
                std::copy(stats.keys().begin(), stats.keys().end(), std::back_inserter(keys_root));
                std::sort(keys_root.begin(), keys_root.end());
                std::vector<std::string> keys_check = { "costs", "ctx", "enumerated", "lp", "optimal", "result", "solver", "solvers", "step", "time_cpu", "time_sat", "time_solve", "time_total", "time_unsat" };
                REQUIRE(keys_root == keys_check);
                REQUIRE(stats["solver"].type() == StatisticsType::Array);
                REQUIRE(stats["solvers"].type() == StatisticsType::Map);
                REQUIRE(stats["time_cpu"].type() == StatisticsType::Value);
                REQUIRE(stats["time_cpu"] >= 0.0);
                REQUIRE(stats["solver"].size() == 1);
                REQUIRE(stats["solver"][size_t(0)]["conflicts"] > 0);
                int nloop = 0;
                for (auto s : stats["solver"]) {
                    REQUIRE(s["conflicts"] > 0);
                    ++nloop;
                }
                REQUIRE(nloop == 1);
            }
            SECTION("optimize") {
                ctl.add("base", {}, "2 {a; b; c; d}.\n"
                                    ":- a, b.\n"
                                    ":- a, c.\n"
                                    "#minimize {2@2:a; 3@2:b; 4@2:c; 5@2:d}.\n"
                                    "#minimize {3@1:a; 4@1:b; 5@1:c; 2@1:d}.\n");
                ctl.ground({{"base", {}}});
                SymbolVector model;
                OptimizationVector optimum;
                REQUIRE(ctl.solve([&optimum, &model](Model m) {
                    model = m.atoms();
                    optimum = m.optimization();
                    return true;
                }).sat());
                REQUIRE(model == SymbolVector({ Id("a"), Id("d") }));
                REQUIRE(optimum == (OptimizationVector{7,5}));
                REQUIRE(messages.empty());
            }
            SECTION("async") {
                ctl.add("base", {}, "{a}.");
                ctl.ground({{"base", {}}});
                ModelCallback mc = MCB(models);
                FinishCallback fc;
                auto async = ctl.solve_async(mc, fc);
                REQUIRE(async.get().sat());
                REQUIRE(models == ModelVec({{},{Id("a")}}));
                REQUIRE(messages.empty());
            }
            SECTION("model") {
                ctl.add("base", {}, "a. $x $= 1. #show b.");
                ctl.ground({{"base", {}}});
                REQUIRE(ctl.solve([](Model m) {
                    REQUIRE(m.atoms(ShowType::Atoms) == (SymbolVector{Id("a")}));
                    REQUIRE(m.atoms(ShowType::Terms) == (SymbolVector{Id("b")}));
                    REQUIRE(m.atoms(ShowType::CSP) == (SymbolVector{Fun("$", {Id("x"), Num(1)})}));
                    REQUIRE(m.atoms(ShowType::Shown) == (SymbolVector{Fun("$", {Id("x"), Num(1)}), Id("a"), Id("b")}));
                    REQUIRE(m.atoms(ShowType::Atoms | ShowType::Comp).size() == 0);
                    REQUIRE( m.contains(Id("a")));
                    REQUIRE(!m.contains(Id("b")));
                    return true;
                }).sat());
                REQUIRE(messages.empty());
            }
            SECTION("assumptions") {
                ctl.add("base", {}, "{a;b;c}.");
                ctl.ground({{"base", {}}});
                REQUIRE(ctl.solve(MCB(models), {{Id("a"), false}, {Id("b"), true}}).sat());
                REQUIRE(models == (ModelVec{{Id("a")}, {Id("a"), Id("c")}}));
                REQUIRE(messages.empty());
            }
            SECTION("theory atoms") {
                char const *theory =
                    "#theory t {\n"
                    "  group {\n"
                    "    + : 4, unary;\n"
                    "    - : 4, unary;\n"
                    "    ^ : 3, binary, right;\n"
                    "    * : 2, binary, left;\n"
                    "    + : 1, binary, left;\n"
                    "    - : 1, binary, left\n"
                    "  };\n"
                    "  &a/0 : group, head;\n"
                    "  &b/0 : group, {=}, group, directive\n"
                    "}.\n"
                    "{p; q}.\n"
                    "&a { 1,[1,a],f(1) : p, q }.\n";
                ctl.add("base", {}, theory);
                ctl.ground({{"base", {}}});
                auto atoms = ctl.theory_atoms();
                REQUIRE(atoms.size() == 1);
                int count = 0;
                for (auto atom : atoms) {
                    ++count;
                    REQUIRE(atom.to_string() == "&a{1,[1,a],f(1): p,q}");
                    REQUIRE(!atom.has_guard());
                    REQUIRE(atom.term().type() == TheoryTermType::Symbol);
                    REQUIRE(atom.term().name() == S("a"));
                    REQUIRE(atom.elements().size() == 1);
                    REQUIRE(atom.literal() > 0);
                    for (auto elem : atom.elements()) {
                        ++count;
                        REQUIRE(elem.condition_literal() > 0);
                        REQUIRE(elem.tuple().size() == 3);
                        auto it = elem.tuple().begin();
                        REQUIRE(it->type() == TheoryTermType::Number);
                        REQUIRE(it->number() == 1);
                        ++it;
                        REQUIRE(it->type() == TheoryTermType::List);
                        REQUIRE(it->arguments().size() == 2);
                        auto jt = it->arguments().begin();
                        REQUIRE(jt->type() == TheoryTermType::Number);
                        REQUIRE((*++jt).type() == TheoryTermType::Symbol);
                        ++it;
                        REQUIRE(it->type() == TheoryTermType::Function);
                        REQUIRE(it->name() == S("f"));
                        REQUIRE(elem.condition().size() == 2);
                        auto kt = elem.condition().begin();
                        REQUIRE(*kt++ > 0);
                        REQUIRE(*kt++ > 0);
                        REQUIRE(kt == elem.condition().end());
                    }
                }
                REQUIRE(count == 2);
                REQUIRE(ctl.solve(MCB(models), {{Id("p"), false}, {Id("q"), false}}).sat());
                REQUIRE(models == ModelVec({{Id("p"), Id("q")}}));
                REQUIRE(atoms.size() == 0);
                ctl.add("next", {}, "&b {} = 42.");
                ctl.ground({{"next", {}}});
                REQUIRE(atoms.size() == 1);
                auto atom = *atoms.begin();
                REQUIRE(atom.term().name() == S("b"));
                REQUIRE(atom.has_guard());
                REQUIRE(atom.guard().first == S("="));
                REQUIRE(atom.guard().second.number() == 42);
                REQUIRE(atom.literal() == 0);
            }
            SECTION("symbolic atoms") {
                ctl.add("base", {}, "p(1). {p(2)}. #external p(3). q.");
                ctl.ground({{"base", {}}});
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(messages.empty());
                auto atoms = ctl.symbolic_atoms();
                Symbol p1 = Fun("p", {Num(1)}), p2 = Fun("p", {Num(2)}), p3 = Fun("p", {Num(3)}), q = Id("q");
                REQUIRE( atoms.find(p1)->fact()); REQUIRE(!atoms.find(p1)->external());
                REQUIRE(!atoms.find(p2)->fact()); REQUIRE(!atoms.find(p2)->external());
                REQUIRE(!atoms.find(p3)->fact()); REQUIRE( atoms.find(p3)->external());
                REQUIRE(atoms.length() == 4);
                SymbolVector symbols;
                for (auto atom : atoms) { symbols.emplace_back(atom.symbol()); }
                std::sort(symbols.begin(), symbols.end());
                REQUIRE(symbols == SymbolVector({q, p1, p2, p3}));
                symbols.clear();
                for (auto it = atoms.begin(Signature("p", 1)); it; ++it) { symbols.emplace_back(it->symbol()); }
                std::sort(symbols.begin(), symbols.end());
                REQUIRE(symbols == SymbolVector({p1, p2, p3}));
            }
            SECTION("incremental") {
                ctl.add("base", {}, "#external query(0).");
                ctl.add("acid", {"k"}, "#external query(k).");
                ctl.ground({{"base", {}}});
                ctl.assign_external(Fun("query", {Num(0)}), TruthValue::True);
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(models == (ModelVec{{Fun("query", {Num(0)})}}));
                REQUIRE(messages.empty());

                ctl.ground({{"acid", {Num(1)}}});
                ctl.release_external(Fun("query", {Num(0)}));
                ctl.assign_external(Fun("query", {Num(1)}), TruthValue::Free);
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(models == (ModelVec{{}, {Fun("query", {Num(1)})}}));
                REQUIRE(messages.empty());
            }
            SECTION("solve_iter") {
                ctl.add("base", {}, "a.");
                ctl.ground({{"base", {}}});
                {
                    auto iter = ctl.solve_iter();
                    MCB mcb(models);
                    SECTION("c++") { for (auto m : iter) { mcb(m); } }
                    SECTION("java") { while (Model m = iter.next()) { mcb(m); } }
                    REQUIRE(models == ModelVec{{Id("a")}});
                    REQUIRE(iter.get().sat());
                }
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(models == ModelVec{{Id("a")}});
                REQUIRE(messages.empty());
            }
            SECTION("logging") {
                ctl.add("base", {}, "a(1+a).");
                ctl.ground({{"base", {}}});
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(models == ModelVec{{}});
                REQUIRE(messages == MessageVec({{MessageCode::OperationUndefined, "<block>:1:3-6: info: operation undefined:\n  (1+a)\n"}}));
            }
            SECTION("use_enum_assumption") {
                ctl.add("base", {}, "{p;q}.");
                ctl.ground({{"base", {}}});
                ctl.use_enum_assumption(false);
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(models.size() == 4);
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(models.size() <= 4);
            }
            SECTION("use_enum_assumption") {
                ctl.add("base", {}, "{p;q}.");
                ctl.ground({{"base", {}}});
                ctl.use_enum_assumption(false);
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(models.size() == 4);
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(models.size() <= 4);
            }
            SECTION("cleanup") {
                ctl.add("base", {}, "{a}. :- a.");
                ctl.ground({{"base", {}}});
                auto dom = ctl.symbolic_atoms();
                REQUIRE(dom.find(Id("a")) != dom.end());
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(models.size() == 1);
                ctl.cleanup();
                REQUIRE(dom.find(Id("a")) == dom.end());
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(models.size() == 1);
            }
            SECTION("const") {
                ctl.add("base", {}, "#const a=10.");
                REQUIRE(ctl.has_const("a"));
                REQUIRE(!ctl.has_const("b"));
                REQUIRE(ctl.get_const("a") == Num(10));
                REQUIRE(ctl.get_const("b") == Id("b"));
            }
            SECTION("async and cancel") {
                ctl.add("pigeon", {"p", "h"}, "1 {p(P,H) : P=1..p}1 :- H=1..h."
                                              "1 {p(P,H) : H=1..h}1 :- P=1..p.");
                ctl.ground({{"pigeon", {Num(21), Num(20)}}});
                ModelCallback mh;
                int fcalled = 0;
                SolveResult fret;
                FinishCallback fh = [&fret, &fcalled](SolveResult ret) { ++fcalled; fret = ret; };
                auto async = ctl.solve_async(mh, fh);
                REQUIRE(!async.wait(0.01));
                SECTION("interrupt") { ctl.interrupt(); }
                SECTION("cancel") { async.cancel(); }
                auto ret = async.get();
                REQUIRE((ret.unknown() && ret.interrupted()));
                REQUIRE(fcalled == 1);
                REQUIRE(fret == ret);
            }
            SECTION("ground callback") {
                ctl.add("base", {}, "a(@f(X)):- X=1..2. b(@f()).");
                ctl.ground({{"base", {}}}, [&messages](Location loc, char const *name, SymbolSpan args, SymbolSpanCallback report) {
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
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(models == ModelVec({{Fun("a", {Num(2)}), Fun("a", {Num(3)}), Fun("a", {Num(11)}), Fun("a", {Num(12)}), Fun("a", {Num(21)}), Fun("a", {Num(22)})}}));
                REQUIRE(messages == MessageVec({{MessageCode::OperationUndefined, "invalid call: f/0 at <block>:1:22-26"}}));
            }
            SECTION("ground callback fail") {
                ctl.add("base", {}, "a(@f()).");
                REQUIRE_THROWS_AS(ctl.ground({{"base", {}}}, [](Location, char const *, SymbolSpan, SymbolSpanCallback) { throw std::runtime_error("fail"); }), std::runtime_error);
            }
        }
    }
}

} } // namespace Test Clingo
