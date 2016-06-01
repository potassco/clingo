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

#include "catch.hpp"
#include "clingo.hh"
#include <vector>
#include <iostream>
#include <unordered_map>

using namespace Clingo;

using ModelVec = std::vector<SymVec>;
using MessageVec = std::vector<std::pair<MessageCode, std::string>>;

struct MCB {
    MCB(ModelVec &models) : models(models) {
        models.clear();
    }
    bool operator()(Model m) {
        models.emplace_back();
        for (auto sym : m.atoms(ShowType::All)) {
            models.back().emplace_back(sym);
        }
        std::sort(models.back().begin(), models.back().end());
        return true;
    }
    ~MCB() {
        std::sort(models.begin(), models.end());
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

class PigeonPropagator : public Propagator {
public:
    void init(PropagateInit &init) override {
        unsigned nHole = 0, nPig = 0, nWatch = 0, p, h;
        state_.clear();
        p2h_[0] = 0;
        for (auto it = init.symbolic_atoms().begin(Signature("place", 2)), ie = init.symbolic_atoms().end(); it != ie; ++it) {
            lit_t lit = init.map_literal(it->literal());
            p = it->symbol().args()[0].num();
            h = it->symbol().args()[1].num();
            p2h_[lit] = h;
            init.add_watch(lit);
            nHole = std::max(h, nHole);
            nPig  = std::max(p, nPig);
            ++nWatch;
        }
        assert(p2h_[0] == 0);
        for (unsigned i = 0, end = init.number_of_threads(); i != end; ++i) {
            state_.emplace_back(nHole + 1, 0);
            assert(state_.back().size() == nHole + 1);
        }
    }
    void propagate(PropagateControl &ctl, LitSpan changes) override {
        assert(ctl.thread_id() < state_.size());
        Hole2Lit& holes = state_[ctl.thread_id()];
        for (lit_t lit : changes) {
            lit_t& prev = holes[ p2h_[lit] ];
            if (prev == 0) { prev = lit; }
            else {
                if (!ctl.add_clause({-lit, -prev}) || !ctl.propagate()) {
                    return;
                }
                assert(false);
            }
        }
    }
    void undo(PropagateControl const &ctl, LitSpan undo) override {
        assert(ctl.thread_id() < state_.size());
        Hole2Lit& holes = state_[ctl.thread_id()];
        for (lit_t lit : undo) {
            unsigned hole = p2h_[lit];
            if (holes[hole] == lit) {
                holes[hole] = 0;
            }
        }
    }
private:
    using Lit2Hole = std::unordered_map<lit_t, unsigned>;
    using Hole2Lit = std::vector<lit_t>;
    using State    = std::vector<Hole2Lit>;

    Lit2Hole p2h_;
    State    state_;
};

class TestAssignment : public Propagator {
public:
    void init(PropagateInit &init) override {
        a_ = init.map_literal(init.symbolic_atoms().lookup(Id("a")).literal());
        b_ = init.map_literal(init.symbolic_atoms().lookup(Id("b")).literal());
        c_ = init.map_literal(init.symbolic_atoms().lookup(Id("c")).literal());
        init.add_watch(a_);
        init.add_watch(b_);
    }
    void propagate(PropagateControl &ctl, LitSpan changes) override {
        auto ass = ctl.assignment();
        count_+= changes.size();
        REQUIRE(ass.is_fixed(c_));
        REQUIRE(!ass.is_fixed(a_));
        REQUIRE(!ass.is_fixed(b_));
        REQUIRE(!ass.has_conflict());
        REQUIRE(ass.has_literal(a_));
        REQUIRE(ass.has_literal(b_));
        REQUIRE(!ass.has_literal(1000));
        auto decision = ass.decision(ass.decision_level());
        REQUIRE(ass.level(decision) == ass.decision_level());
        if (count_ == 1) {
            int a = changes[0];
            REQUIRE(changes.size() == 1);
            REQUIRE(!ass.is_fixed(a_));
            REQUIRE(ass.is_true(a));
            REQUIRE(ass.truth_value(a) == TruthValue::True);
            REQUIRE((ass.is_true(a_) ^ ass.is_true(b_)));
            REQUIRE(ass.level(a) == ass.decision_level());
        }
        if (count_ == 2) {
            REQUIRE(!ass.is_fixed(a_));
            REQUIRE(!ass.is_fixed(b_));
            REQUIRE(ass.is_true(a_));
            REQUIRE(ass.is_true(b_));
        }
    }
    void undo(PropagateControl const &, LitSpan undo) override {
        count_-= undo.size();
    }
private:
    lit_t a_;
    lit_t b_;
    lit_t c_;
    int count_ = 0;
};

class TestAddClause : public Propagator {
public:
    void init(PropagateInit &init) override {
        a_ = init.map_literal(init.symbolic_atoms().lookup(Id("a")).literal());
        b_ = init.map_literal(init.symbolic_atoms().lookup(Id("b")).literal());
        init.add_watch(a_);
        init.add_watch(b_);
    }
    void propagate(PropagateControl &ctl, LitSpan changes) override {
        count_+= changes.size();
        REQUIRE_FALSE((enable && count_ == 2 && ctl.add_clause({-a_, -b_}, type) && ctl.propagate()));
    }
    void undo(PropagateControl const &, LitSpan undo) override {
        count_-= undo.size();
    }
public:
    ClauseType type = ClauseType::Learnt;
    bool enable = true;
private:
    lit_t a_;
    lit_t b_;
    int count_ = 0;
};

TEST_CASE("propagator") {
    Module mod;
    MessageVec messages;
    ModelVec models;
    Logger logger = [&messages](MessageCode code, char const *msg) { messages.emplace_back(code, msg); };
    Control ctl{mod.create_control({"test_libclingo", "0"}, logger, 20)};
    SECTION("pigeon") {
        PigeonPropagator p;
        ctl.register_propagator(p, false);
        ctl.add("pigeon", {"h", "p"}, "1 { place(P,H) : H = 1..h } 1 :- P = 1..p.");
        auto place = [](int p, int h) { return Fun("place", {Num(p), Num(h)}); };
        SECTION("unsat") {
            ctl.ground({{"pigeon", {Num(5), Num(6)}}}, nullptr);
            ctl.solve(MCB(models));
            REQUIRE(models.empty());
        }
        SECTION("sat") {
            ctl.ground({{"pigeon", {Num(2), Num(2)}}}, nullptr);
            ctl.solve(MCB(models));
            REQUIRE(models == ModelVec({{place(1,1), place(2,2)}, {place(1,2), place(2,1)}}));
        }
    }
    SECTION("assignment") {
        TestAssignment p;
        ctl.register_propagator(p, false);
        ctl.add("base", {}, "{a; b}. c.");
        ctl.ground({{"base", {}}}, nullptr);
        ctl.solve(MCB(models));
        REQUIRE(models.size() == 4);
    }
    SECTION("add_clause") {
        TestAddClause p;
        ctl.register_propagator(p, false);
        SECTION("learnt") {
            p.type = ClauseType::Learnt;
            ctl.add("base", {}, "{a; b}.");
            ctl.ground({{"base", {}}}, nullptr);
            ctl.solve(MCB(models));
            REQUIRE(models.size() == 3);
            p.enable = false;
            ctl.solve(MCB(models));
            REQUIRE(models.size() >= 3);
        }
        SECTION("static") {
            p.type = ClauseType::Static;
            ctl.add("base", {}, "{a; b}.");
            ctl.ground({{"base", {}}}, nullptr);
            ctl.solve(MCB(models));
            REQUIRE(models.size() == 3);
            p.enable = false;
            ctl.solve(MCB(models));
            REQUIRE(models.size() == 3);
        }
        SECTION("volatile") {
            p.type = ClauseType::Volatile;
            ctl.add("base", {}, "{a; b}.");
            ctl.ground({{"base", {}}}, nullptr);
            ctl.solve(MCB(models));
            REQUIRE(models.size() == 3);
            p.enable = false;
            ctl.solve(MCB(models));
            REQUIRE(models.size() == 4);
        }
        SECTION("volatile static") {
            p.type = ClauseType::VolatileStatic;
            ctl.add("base", {}, "{a; b}.");
            ctl.ground({{"base", {}}}, nullptr);
            ctl.solve(MCB(models));
            REQUIRE(models.size() == 3);
            p.enable = false;
            ctl.solve(MCB(models));
            REQUIRE(models.size() == 4);
        }
    }
}

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
        REQUIRE("f(42,#inf,#sup,\"x\",-x)" == sym.to_string());
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
    SECTION("signature") {
        Signature a("a", 2, true);
        Signature b("a", 2, true);
        Signature c("a", 2, false);
        REQUIRE(a.name() == S("a"));
        REQUIRE(a.arity() == 2);
        REQUIRE(a.sign());
        REQUIRE(b == a);
        REQUIRE(c != a);
        REQUIRE(c < a);
        REQUIRE(c <= a);
        REQUIRE(a <= b);
        REQUIRE_FALSE(a <= c);
        REQUIRE(a > c);
        REQUIRE(a >= c);
        REQUIRE(a >= b);
        REQUIRE_FALSE(c >= a);
        REQUIRE(c.hash() != a.hash());
        REQUIRE(b.hash() == a.hash());
    }
    SECTION("with module") {
        Module mod;
        SECTION("with control") {
            MessageVec messages;
            ModelVec models;
            Logger logger = [&messages](MessageCode code, char const *msg) { messages.emplace_back(code, msg); };
            Control ctl{mod.create_control({"test_libclingo", "0"}, logger, 20)};
            SECTION("solve") {
                ctl.add("base", {}, "a.");
                ctl.ground({{"base", {}}});
                REQUIRE(ctl.solve(MCB(models)).sat());
                REQUIRE(models == ModelVec{{Id("a")}});
                REQUIRE(messages.empty());
            }
            SECTION("model") {
                ctl.add("base", {}, "a. $x $= 1. #show b.");
                ctl.ground({{"base", {}}});
                REQUIRE(ctl.solve([](Model m) {
                    REQUIRE(m.atoms(ShowType::Atoms) == (SymVec{Id("a")}));
                    REQUIRE(m.atoms(ShowType::Terms) == (SymVec{Id("b")}));
                    REQUIRE(m.atoms(ShowType::CSP) == (SymVec{Fun("$", {Id("x"), Num(1)})}));
                    REQUIRE(m.atoms(ShowType::Shown) == (SymVec{Fun("$", {Id("x"), Num(1)}), Id("a"), Id("b")}));
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
                REQUIRE( atoms.lookup(p1).fact()); REQUIRE(!atoms.lookup(p1).external());
                REQUIRE(!atoms.lookup(p2).fact()); REQUIRE(!atoms.lookup(p2).external());
                REQUIRE(!atoms.lookup(p3).fact()); REQUIRE( atoms.lookup(p3).external());
                REQUIRE(atoms.length() == 4);
                SymVec symbols;
                for (auto atom : atoms) { symbols.emplace_back(atom.symbol()); }
                std::sort(symbols.begin(), symbols.end());
                REQUIRE(symbols == SymVec({q, p1, p2, p3}));
                symbols.clear();
                for (auto it = atoms.begin(Signature("p", 1)); it; ++it) { symbols.emplace_back(it->symbol()); }
                std::sort(symbols.begin(), symbols.end());
                REQUIRE(symbols == SymVec({p1, p2, p3}));
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
                    while (Model m = iter.next()) { mcb(m); }
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
                REQUIRE(ctl.solve(MCB(models)).sat());
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

