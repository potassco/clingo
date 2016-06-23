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

TEST_CASE("parse_term", "[clingo]") {
    MessageVec messages;
    Logger logger = [&messages](WarningCode code, char const *msg) { messages.emplace_back(code, msg); };
    REQUIRE(parse_term("10+1").num() == 11);
    REQUIRE_THROWS(parse_term("10+", logger));
    //REQUIRE(messages == (MessageVec{{WarningCode::Runtime, "<string>:1:5: error: syntax error, unexpected <EOF>\n"}}));
    messages.clear();
    REQUIRE_THROWS(parse_term("10+a", logger));
    REQUIRE(messages.size() == 0);
};

TEST_CASE("solving", "[clingo]") {
    SECTION("with control") {
        MessageVec messages;
        ModelVec models;
        Control ctl{{"0"}, [&messages](WarningCode code, char const *msg) { messages.emplace_back(code, msg); }, 20};
        SECTION("solve") {
            static int n = 0;
            if (++n < 3) { // workaround for some bug with catch
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
                REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
                REQUIRE(models == ModelVec({{},{Id("a")}}));
                REQUIRE(messages.empty());
            }
        }
        SECTION("statistics") {
            ctl.add("pigeon", {"p", "h"}, "1 {p(P,H) : P=1..p}1 :- H=1..h."
                                          "1 {p(P,H) : H=1..h}1 :- P=1..p.");
            ctl.ground({{"pigeon", {Number(6), Number(5)}}});
            REQUIRE(ctl.solve(MCB(models)).is_unsatisfiable());
            std::vector<std::string> keys_root;
            auto stats = ctl.statistics();
            std::copy(stats.keys().begin(), stats.keys().end(), std::back_inserter(keys_root));
            std::sort(keys_root.begin(), keys_root.end());
            std::vector<std::string> keys_check = { ".ctx", ".lp", ".solver", ".solvers", ".summary" };
            REQUIRE(keys_root == keys_check);
            REQUIRE(stats["solver"].type() == StatisticsType::Array);
            REQUIRE(stats["solvers"].type() == StatisticsType::Map);
            REQUIRE(stats["summary.time_cpu"].type() == StatisticsType::Value);
            REQUIRE(stats["summary.time_cpu"] >= 0.0);
            REQUIRE(stats["solver"].size() == 1);
            REQUIRE(stats["solver"][size_t(0)]["conflicts"] > 0);
            int nloop = 0;
            for (auto s : stats["solver"]) {
                REQUIRE(s["conflicts"] > 0);
                ++nloop;
            }
            REQUIRE(nloop == 1);
        }
        SECTION("configuration") {
            auto conf = ctl.configuration();
            std::vector<std::string> keys_root;
            std::copy(conf.keys().begin(), conf.keys().end(), std::back_inserter(keys_root));
            std::sort(keys_root.begin(), keys_root.end());
            std::vector<std::string> keys_check = { "asp.", "configuration", "learn_explicit", "sat_prepro", "share", "solve.", "solver.", "stats", "tester." };
            REQUIRE(keys_root == keys_check);
            REQUIRE(conf["solve"]["models"].is_value());
            conf["solve"]["models"] = "2";
            REQUIRE(conf["solve"].decription() == S("Solve Options"));
            REQUIRE(conf["solve"]["models"].is_assigned());
            REQUIRE(conf["solve"]["models"].value() == "2");
            REQUIRE(conf["solver"].is_array());
            REQUIRE(conf["solver"].size() == 1);
            int nloop = 0;
            for (auto s : conf["solver"]) {
                s["heuristic"] = "berkmin,100";
                ++nloop;
            }
            REQUIRE(nloop == 1);
            CHECK(conf["solver"][size_t(0)]["heuristic"].value() == "berkmin,100");
            CHECK(conf["solver"]["heuristic"].value() == "berkmin,100");
            ctl.add("base", {}, "{a; b; c}.");
            ctl.ground({{"base", {}}});
            REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
            REQUIRE(models.size() == 2);
        }
        SECTION("backend") {
            // NOTE: ground has to be called before using the backend
            auto backend = ctl.backend();
            literal_t a = backend.add_atom(), b = backend.add_atom();
            backend.rule(true, {atom_t(a)}, {});
            backend.rule(false, {atom_t(b)}, {-a});
            ctl.solve(MCB(models));
            REQUIRE(models == (ModelVec{{},{}}));
            backend.assume({a});
            ctl.solve(MCB(models));
            REQUIRE(models == (ModelVec{{}}));
            backend.minimize(1, {{a,1},{b,1}});
            ctl.solve(MCB(models));
            REQUIRE(ctl.statistics()["summary.costs"].size() == 1);
            REQUIRE(ctl.statistics()["summary.costs"][size_t(0)] == 1);
            REQUIRE(ctl.statistics()["summary.step"] == 2);
            // Note: I don't have a good idea how to test this one
            // void heuristic(atom_t atom, HeuristicType type, int bias, unsigned priority, LitSpan condition);
        }
        SECTION("backend-project") {
            ctl.configuration()["solve.project"] = "1";
            auto backend = ctl.backend();
            atom_t a = backend.add_atom(), b = backend.add_atom();
            backend.rule(true, {a, b}, {});
            backend.project({a});
            ctl.solve(MCB(models));
            REQUIRE(models == (ModelVec{{},{}}));
        }
        SECTION("backend-external") {
            auto backend = ctl.backend();
            atom_t a = backend.add_atom();
            backend.external(a, ExternalType::Free);
            ctl.solve(MCB(models));
            REQUIRE(models == (ModelVec{{},{}}));
            backend.external(a, ExternalType::Release);
            ctl.solve(MCB(models));
            REQUIRE(models == (ModelVec{{}}));
        }
        SECTION("backend-acyc") {
            auto backend = ctl.backend();
            atom_t a = backend.add_atom(), b = backend.add_atom();
            backend.rule(true, {a, b}, {});
            backend.acyc_edge(1, 2, {literal_t(a)});
            backend.acyc_edge(2, 1, {literal_t(b)});
            ctl.solve(MCB(models));
            REQUIRE(models == (ModelVec{{},{},{}}));
        }
        SECTION("backend-weight-rule") {
            auto backend = ctl.backend();
            atom_t a = backend.add_atom(), b = backend.add_atom();
            backend.rule(true, {a, b}, {});
            backend.weight_rule(false, {}, 1, {{literal_t(a),1}, {literal_t(b),1}});
            ctl.solve(MCB(models));
            REQUIRE(models == (ModelVec{{}}));
        }
        SECTION("optimize") {
            ctl.add("base", {}, "2 {a; b; c; d}.\n"
                                ":- a, b.\n"
                                ":- a, c.\n"
                                "#minimize {2@2:a; 3@2:b; 4@2:c; 5@2:d}.\n"
                                "#minimize {3@1:a; 4@1:b; 5@1:c; 2@1:d}.\n");
            ctl.ground({{"base", {}}});
            SymbolVector model;
            CostVector optimum;
            REQUIRE(ctl.solve([&optimum, &model](Model m) {
                model = m.atoms();
                optimum = m.cost();
                return true;
            }).is_satisfiable());
            REQUIRE(model == SymbolVector({ Id("a"), Id("d") }));
            REQUIRE(optimum == (CostVector{7,5}));
            REQUIRE(messages.empty());
        }
        SECTION("async") {
            ctl.add("base", {}, "{a}.");
            ctl.ground({{"base", {}}});
            auto async = ctl.solve_async(MCB(models));
            REQUIRE(async.get().is_satisfiable());
            REQUIRE(models == ModelVec({{},{Id("a")}}));
            REQUIRE(messages.empty());
        }
        SECTION("model") {
            ctl.add("base", {}, "a. $x $= 1. #show b.");
            ctl.ground({{"base", {}}});
            REQUIRE(ctl.solve([](Model m) {
                REQUIRE(m.type() == ModelType::StableModel);
                REQUIRE(m.atoms(ShowType::Atoms) == (SymbolVector{Id("a")}));
                REQUIRE(m.atoms(ShowType::Terms) == (SymbolVector{Id("b")}));
                REQUIRE(m.atoms(ShowType::CSP) == (SymbolVector{Function("$", {Id("x"), Number(1)})}));
                REQUIRE(m.atoms(ShowType::Shown) == (SymbolVector{Function("$", {Id("x"), Number(1)}), Id("a"), Id("b")}));
                REQUIRE(m.atoms(ShowType::Atoms | ShowType::Complement).size() == 0);
                REQUIRE( m.contains(Id("a")));
                REQUIRE(!m.contains(Id("b")));
                return true;
            }).is_satisfiable());
            REQUIRE(messages.empty());
        }
        SECTION("model-add-clause") {
            ctl.add("base", {}, "1{a;b}1.");
            ctl.ground({{"base", {}}});
            int n = 0;
            auto ret = ctl.solve([&n](Model m) {
                REQUIRE(m.type() == ModelType::StableModel);
                REQUIRE(m.context().thread_id() == 0);
                ++n;
                char const *name = m.contains(Id("a")) ? "b" : "a";
                m.context().add_clause({{Id(name), false}});
                return true;
            });
            REQUIRE(ret.is_satisfiable());
            REQUIRE(n == 1);
            REQUIRE(messages.empty());
        }
        SECTION("model-cautious") {
            ctl.configuration()["solve.enum_mode"] = "cautious";
            ctl.add("base", {}, "a.");
            ctl.ground({{"base", {}}});
            int n = 0;
            auto ret = ctl.solve([&n](Model m) {
                REQUIRE(m.type() == ModelType::CautiousConsequences);
                REQUIRE(m.context().thread_id() == 0);
                ++n;
                return true;
            });
            REQUIRE(ret.is_satisfiable());
            REQUIRE(n == 1);
            REQUIRE(messages.empty());
        }
        SECTION("model-cost") {
            ctl.configuration()["solve.opt_mode"] = "optN";
            ctl.add("base", {}, "{a}. #minimize { 1:a }.");
            ctl.ground({{"base", {}}});
            int n = 0;
            auto ret = ctl.solve([&n](Model m) {
                REQUIRE(m.type() == ModelType::StableModel);
                REQUIRE(m.context().thread_id() == 0);
                if (m.optimality_proven()) {
                    REQUIRE(m.number() == 1);
                    REQUIRE(m.cost() == (CostVector{0}));
                    ++n;
                }
                return true;
            });
            REQUIRE(ret.is_satisfiable());
            REQUIRE(n == 1);
            REQUIRE(messages.empty());
        }
        SECTION("assumptions") {
            ctl.add("base", {}, "{a;b;c}.");
            ctl.ground({{"base", {}}});
            REQUIRE(ctl.solve(MCB(models), {{Id("a"), false}, {Id("b"), true}}).is_satisfiable());
            REQUIRE(models == (ModelVec{{Id("a")}, {Id("a"), Id("c")}}));
            REQUIRE(messages.empty());
        }
        SECTION("theory-atoms") {
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
            REQUIRE(ctl.solve(MCB(models), {{Id("p"), false}, {Id("q"), false}}).is_satisfiable());
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
            REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
            REQUIRE(messages.empty());
            auto atoms = ctl.symbolic_atoms();
            Symbol p1 = Function("p", {Number(1)}), p2 = Function("p", {Number(2)}), p3 = Function("p", {Number(3)}), q = Id("q");
            REQUIRE( atoms.find(p1)->is_fact()); REQUIRE(!atoms.find(p1)->is_external());
            REQUIRE(!atoms.find(p2)->is_fact()); REQUIRE(!atoms.find(p2)->is_external());
            REQUIRE(!atoms.find(p3)->is_fact()); REQUIRE( atoms.find(p3)->is_external());
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
            ctl.assign_external(Function("query", {Number(0)}), TruthValue::True);
            REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
            REQUIRE(models == (ModelVec{{Function("query", {Number(0)})}}));
            REQUIRE(messages.empty());

            ctl.ground({{"acid", {Number(1)}}});
            ctl.release_external(Function("query", {Number(0)}));
            ctl.assign_external(Function("query", {Number(1)}), TruthValue::Free);
            REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
            REQUIRE(models == (ModelVec{{}, {Function("query", {Number(1)})}}));
            REQUIRE(messages.empty());
        }
        SECTION("solve_iter") {
            static int n = 0;
            if (++n < 3) { // workaround for some bug with catch
                ctl.add("base", {}, "a.");
                ctl.ground({{"base", {}}});
                {
                    auto iter = ctl.solve_iteratively();
                    MCB mcb(models);
                    SECTION("c++") { for (auto m : iter) { mcb(m); }; }
                    SECTION("java") { while (Model m = iter.next()) { mcb(m); }; }
                    REQUIRE(models == ModelVec{{Id("a")}});
                    REQUIRE(iter.get().is_satisfiable());
                }
                REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
                REQUIRE(models == ModelVec{{Id("a")}});
                REQUIRE(messages.empty());
            }
        }
        SECTION("logging") {
            ctl.add("base", {}, "a(1+a).");
            ctl.ground({{"base", {}}});
            REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
            REQUIRE(models == ModelVec{{}});
            REQUIRE(messages == MessageVec({{WarningCode::OperationUndefined, "<block>:1:3-6: info: operation undefined:\n  (1+a)\n"}}));
        }
        SECTION("use_enum_assumption") {
            ctl.add("base", {}, "{p;q}.");
            ctl.ground({{"base", {}}});
            ctl.use_enum_assumption(false);
            REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
            REQUIRE(models.size() == 4);
            REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
            REQUIRE(models.size() <= 4);
        }
        SECTION("use_enum_assumption") {
            ctl.add("base", {}, "{p;q}.");
            ctl.ground({{"base", {}}});
            ctl.use_enum_assumption(false);
            REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
            REQUIRE(models.size() == 4);
            REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
            REQUIRE(models.size() <= 4);
        }
        SECTION("cleanup") {
            ctl.add("base", {}, "{a}. :- a.");
            ctl.ground({{"base", {}}});
            auto dom = ctl.symbolic_atoms();
            REQUIRE(dom.find(Id("a")) != dom.end());
            REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
            REQUIRE(models.size() == 1);
            ctl.cleanup();
            REQUIRE(dom.find(Id("a")) == dom.end());
            REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
            REQUIRE(models.size() == 1);
        }
        SECTION("const") {
            ctl.add("base", {}, "#const a=10.");
            REQUIRE(ctl.has_const("a"));
            REQUIRE(!ctl.has_const("b"));
            REQUIRE(ctl.get_const("a") == Number(10));
            REQUIRE(ctl.get_const("b") == Id("b"));
        }
        SECTION("async and cancel") {
            static int n = 0;
            if (++n < 3) { // workaround for some bug with catch
                ctl.add("pigeon", {"p", "h"}, "1 {p(P,H) : P=1..p}1 :- H=1..h."
                                              "1 {p(P,H) : H=1..h}1 :- P=1..p.");
                ctl.ground({{"pigeon", {Number(21), Number(20)}}});
                int fcalled = 0;
                SolveResult fret;
                FinishCallback fh = [&fret, &fcalled](SolveResult ret) { ++fcalled; fret = ret; };
                auto async = ctl.solve_async(nullptr, fh);
                REQUIRE(!async.wait(0.01));
                SECTION("interrupt") { ctl.interrupt(); }
                SECTION("cancel") { async.cancel(); }
                auto ret = async.get();
                REQUIRE((ret.is_unknown() && ret.is_interrupted()));
                REQUIRE(fcalled == 1);
                REQUIRE(fret == ret);
            }
        }
        SECTION("ground callback") {
            ctl.add("base", {}, "a(@f(X)):- X=1..2. b(@f()).");
            ctl.ground({{"base", {}}}, [&messages](Location loc, char const *name, SymbolSpan args, SymbolSpanCallback report) {
                if (strcmp(name, "f") == 0 && args.size() == 1) {
                    Symbol front = *args.begin();
                    report({Number(front.num() + 1), Number(front.num() + 10)});
                    report({Number(front.num() + 20)});
                }
                else {
                    std::ostringstream oss;
                    oss << "invalid call: " << name << "/" << args.size() << " at " << loc;
                    messages.emplace_back(WarningCode::OperationUndefined, oss.str());
                }
            });
            REQUIRE(ctl.solve(MCB(models)).is_satisfiable());
            REQUIRE(models == ModelVec({{Function("a", {Number(2)}), Function("a", {Number(3)}), Function("a", {Number(11)}), Function("a", {Number(12)}), Function("a", {Number(21)}), Function("a", {Number(22)})}}));
            REQUIRE(messages == MessageVec({{WarningCode::OperationUndefined, "invalid call: f/0 at <block>:1:22-26"}}));
        }
        SECTION("ground callback fail") {
            ctl.add("base", {}, "a(@f()).");
            REQUIRE_THROWS_AS(ctl.ground({{"base", {}}}, [](Location, char const *, SymbolSpan, SymbolSpanCallback) { throw std::runtime_error("fail"); }), std::runtime_error);
        }
    }
}

} } // namespace Test Clingo
