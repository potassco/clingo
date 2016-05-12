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
#include "common.h"
#include <potassco/aspif.h>
#include <potassco/theory_data.h>
#include <sstream>
#include <cstring>
namespace Potassco {
namespace Test {
namespace Aspif {

static void finalize(std::stringstream& str) {
	str << "0\n";
}
static void rule(std::ostream& os, const Rule& r) {
	os << (unsigned)Directive_t::Rule << " " << r.ht << " ";
	os << r.head.size();
	for (auto x : r.head) { os << " " << x; }
	os << " " << (unsigned)r.bt << " ";
	if (r.bnd != Body_t::BOUND_NONE) {
		os << r.bnd << " ";
	}
	os << r.body.size();
	for (auto&& x : r.body) { 
		os << " " << lit(x);
		if (r.bt == Body_t::Sum) {
			os << " " << weight(x);
		}
	}
	os << "\n";
}

class ReadObserver : public Test::ReadObserver {
public:
	virtual void rule(const HeadView& head, const BodyView& body) override {
		rules.push_back({head.type, {begin(head), end(head)}, body.type, body.bound, {begin(body), end(body)}});
	}
	virtual void minimize(Weight_t prio, const WeightLitSpan& lits) override {
		min.push_back({prio, {begin(lits), end(lits)}});
	}
	virtual void project(const AtomSpan& atoms) override {
		projects.insert(projects.end(), begin(atoms), end(atoms));
	}
	virtual void output(const StringSpan& str, const LitSpan& cond) override {
		shows.push_back({{begin(str), end(str)}, {begin(cond), end(cond)}});
	}

	virtual void external(Atom_t a, Value_t v) override {
		externals.push_back({a, v});
	}
	virtual void assume(const LitSpan& lits) override {
		assumes.insert(assumes.end(), begin(lits), end(lits));
	}
	Vec<Rule> rules;
	Vec<std::pair<int, Vec<WeightLit_t> > > min;
	Vec<std::pair<std::string, Vec<Lit_t> > > shows;
	Vec<std::pair<Atom_t, Value_t> > externals;
	Vec<Atom_t> projects;
	Vec<Lit_t>  assumes;
};

static int compareRead(std::stringstream& input, ReadObserver& observer, const Rule* rules, const std::pair<unsigned, unsigned>& subset) {
	for (unsigned i = 0; i != subset.second; ++i) { rule(input, rules[subset.first + i]); }
	finalize(input);
	readAspif(input, observer);
	if (observer.rules.size() != subset.second) {
		return (int)observer.rules.size();
	}
	for (unsigned i = 0; i != subset.second; ++i) {
		if (!(rules[subset.first + i] == observer.rules[i])) {
			return i;
		}
	}
	return subset.second;
}

TEST_CASE("Intermediate Format Reader ", "[aspif]") {
	std::stringstream input;
	ReadObserver observer;
	input << "asp 1 0 0\n";
	SECTION("read empty") {
		finalize(input);
		REQUIRE(readAspif(input, observer) == 0);
		REQUIRE(observer.nStep == 1);
		REQUIRE(observer.incremental == false);
	}
	SECTION("read empty rule") {
		rule(input, {Head_t::Disjunctive, {}, Body_t::Normal, Body_t::BOUND_NONE, {}});
		finalize(input);
		REQUIRE(readAspif(input, observer) == 0);
		REQUIRE(observer.rules.size() == 1);
		REQUIRE(observer.rules[0].head.empty());
		REQUIRE(observer.rules[0].body.empty());
	}
	SECTION("read rules") {
		Rule rules[] = {
			{Head_t::Disjunctive, {1}, Body_t::Normal, Body_t::BOUND_NONE, {{-2, 1}, {3, 1}, {-4, 1}}},
			{Head_t::Disjunctive, {1, 2, 3}, Body_t::Normal, Body_t::BOUND_NONE, {{5, 1}, {-6, 1}}},
			{Head_t::Disjunctive, {}, Body_t::Normal, Body_t::BOUND_NONE, {{1, 1}, {2, 1}}},
			{Head_t::Choice, {1, 2, 3}, Body_t::Normal, Body_t::BOUND_NONE, {{5, 1}, {-6, 1}}},
			// weight
			{Head_t::Disjunctive, {1}, Body_t::Sum, 1, {{2, 1}, {-3, 2}, {-4, 3}, {5, 1}}},
			{Head_t::Disjunctive, {2}, Body_t::Sum, 1, {{3, 1}, {-4, 1}, {5, 1}}},
			// mixed
			{Head_t::Choice, {1, 2}, Body_t::Sum, 1, {{2, 1}, {-3, 2}, {-4, 3}, {5, 1}}},
			{Head_t::Disjunctive, {}, Body_t::Sum, 1, {{2, 1}, {-3, 2}, {-4, 3}, {5, 1}}},
			// negative weights
			{Head_t::Disjunctive, {1}, Body_t::Sum, 1, {{2, 1}, {-3, -2}, {-4, 3}, {5, 1}}}
		};
		using Pair = std::pair<unsigned, unsigned>;
		Pair basic(0, 4);
		Pair weight(4, 2);
		Pair mixed(6, 2);
		Pair neg(8, 1);
		SECTION("simple rules with normal bodies") {
			REQUIRE(compareRead(input, observer, rules, basic) == basic.second);
		}
		SECTION("read rules with weight body") {
			REQUIRE(compareRead(input, observer, rules, weight) == weight.second);
		}
		SECTION("read mixed rules") {
			REQUIRE(compareRead(input, observer, rules, mixed) == mixed.second);
		}
		SECTION("negative weights not allowed in weight rule") {
			REQUIRE_THROWS(compareRead(input, observer, rules, neg));
		}
	}
	SECTION("read minimize rule") {
		input << (unsigned)Directive_t::Minimize << " -1 3 4 5 6 1 3 2\n";
		input << (unsigned)Directive_t::Minimize << " 10 3 4 -52 -6 36 3 -20\n";
		finalize(input);
		REQUIRE(readAspif(input, observer) == 0);
		REQUIRE(observer.min.size() == 2);
		const auto& mr1 = observer.min[0];
		const auto& mr2 = observer.min[1];
		REQUIRE(mr1.first == -1);
		REQUIRE(mr2.first == 10);
		auto lits = Vec<WeightLit_t>{{4, 5}, {6, 1}, {3, 2}};
		REQUIRE(mr1.second == lits);
		lits = Vec<WeightLit_t>{{4, -52}, {-6, 36}, {3, -20}};
		REQUIRE(mr2.second == lits);
	}
	SECTION("read output") {
		input << (unsigned)Directive_t::Output << " 1 a 1 1\n";
		input << (unsigned)Directive_t::Output << " 10 Hallo Welt 2 1 -2\n";
		finalize(input);
		REQUIRE(readAspif(input, observer) == 0);
		REQUIRE(observer.shows.size() == 2);
		const auto& s1 = observer.shows[0];
		const auto& s2 = observer.shows[1];
		REQUIRE(s1.first == "a");
		REQUIRE(s1.second == Vec<Lit_t>({1}));
		REQUIRE(s2.first == "Hallo Welt");
		REQUIRE(s2.second == Vec<Lit_t>({1, -2}));
	}
	SECTION("read projection") {
		input << (unsigned)Directive_t::Project << " 3 1 2 987232\n";
		input << (unsigned)Directive_t::Project << " 1 17\n";
		finalize(input);
		REQUIRE(readAspif(input, observer) == 0);
		REQUIRE(observer.projects == Vec<Atom_t>({1, 2, 987232, 17}));
	}
	SECTION("read external") {
		std::pair<Atom_t, Value_t> exp[] = {
			{1, Value_t::Free},
			{2, Value_t::True},
			{3, Value_t::False},
			{4, Value_t::Release}
		};
		for (auto&& e : exp) {
			input << (unsigned)Directive_t::External << " " << e.first << " " << (unsigned)e.second << "\n";
		}
		finalize(input);
		REQUIRE(readAspif(input, observer) == 0);
		REQUIRE(std::distance(begin(exp), end(exp)) == observer.externals.size());
		REQUIRE(std::equal(begin(exp), end(exp), observer.externals.begin()) == true);
	}
	SECTION("read assumptions") {
		input << (unsigned)Directive_t::Assume << " 2 1 987232\n";
		input << (unsigned)Directive_t::Assume << " 1 -2\n";
		finalize(input);
		REQUIRE(readAspif(input, observer) == 0);
		REQUIRE(observer.assumes == Vec<Lit_t>({1, 987232, -2}));
	}
	SECTION("read edges") {
		input << (unsigned)Directive_t::Edge << " 0 1 2 1 -2\n";
		input << (unsigned)Directive_t::Edge << " 1 0 1 3\n";
		finalize(input);
		REQUIRE(readAspif(input, observer) == 0);
		REQUIRE(observer.edges.size() == 2);
		REQUIRE(observer.edges[0].s == 0);
		REQUIRE(observer.edges[0].t == 1);
		REQUIRE(observer.edges[0].cond == Vec<Lit_t>({1, -2}));
		REQUIRE(observer.edges[1].s == 1);
		REQUIRE(observer.edges[1].t == 0);
		REQUIRE(observer.edges[1].cond == Vec<Lit_t>({3}));
	}
	SECTION("read heuristic") {
		Heuristic exp[] = {
			{1, Heuristic_t::Sign, -1, 1, {10}},
			{2, Heuristic_t::Level, 10, 3, {-1, 10}},
			{1, Heuristic_t::Init, 20, 1, {}},
			{1, Heuristic_t::Factor, 2, 2, {}}
		};
		for (auto&& r : exp) {
			input << (unsigned)Directive_t::Heuristic << " " << (unsigned)r.type << " " << r.atom << " " << r.bias
				<< " " << r.prio << " " << r.cond.size();
			for (auto&& p : r.cond) { input << " " << p; }
			input << "\n";
		}
		finalize(input);
		REQUIRE(readAspif(input, observer) == 0);
		REQUIRE(observer.heuristics.size() == 4);
		REQUIRE(std::equal(std::begin(exp), std::end(exp), observer.heuristics.begin()) == true);
	}
	SECTION("ignore comments") {
		input << (unsigned)Directive_t::Comment << "Hello World" << "\n";
		finalize(input);
		REQUIRE(readAspif(input, observer) == 0);
	}
}
TEST_CASE("Intermediate Format Reader supports incremental programs", "[aspif]") {
	std::stringstream input;
	ReadObserver observer;
	input << "asp 1 0 0 incremental\n";
	SECTION("read empty steps") {
		finalize(input);
		finalize(input);
		REQUIRE(readAspif(input, observer) == 0);
		REQUIRE(observer.incremental == true);
		REQUIRE(observer.nStep == 2);
	}
	SECTION("read rules in each steps") {
		rule(input, {Head_t::Disjunctive, {1,2}, Body_t::Normal, Body_t::BOUND_NONE, {}});
		finalize(input);
		rule(input, {Head_t::Disjunctive, {3, 4}, Body_t::Normal, Body_t::BOUND_NONE, {}});
		finalize(input);
		REQUIRE(readAspif(input, observer) == 0);
		REQUIRE(observer.incremental == true);
		REQUIRE(observer.nStep == 2);
		REQUIRE(observer.rules.size() == 2);
	}
}

TEST_CASE("Intermediate Format Reader requires current version", "[aspif]") {
	std::stringstream input;
	ReadObserver observer;
	input << "asp 1 2 0 incremental\n";
	finalize(input);
	REQUIRE_THROWS(readAspif(input, observer));
}
TEST_CASE("Intermediate Format Reader requires incremental tag for incremental programs", "[aspif]") {
	std::stringstream input;
	ReadObserver observer;
	input << "asp 1 0 0\n";
	finalize(input);
	finalize(input);
	REQUIRE_THROWS(readAspif(input, observer));
}

TEST_CASE("Test AspifOutput", "[aspif]") {
	std::stringstream out;
	AspifOutput writer(out);
	ReadObserver observer;
	writer.initProgram(false);
	writer.beginStep();
	SECTION("Writer writes rules") {
		Rule rules[] = {
			{Head_t::Disjunctive, {1}, Body_t::Normal, Body_t::BOUND_NONE, {{-2, 1}, {3, 1}, {-4, 1}}},
			{Head_t::Disjunctive, {1, 2, 3}, Body_t::Normal, Body_t::BOUND_NONE, {{5, 1}, {-6, 1}}},
			{Head_t::Disjunctive, {}, Body_t::Normal, Body_t::BOUND_NONE, {{1, 1}, {2, 1}}},
			{Head_t::Choice, {1, 2, 3}, Body_t::Normal, Body_t::BOUND_NONE, {{5, 1}, {-6, 1}}},
			// weight
			{Head_t::Disjunctive, {1}, Body_t::Sum, 1, {{2, 1}, {-3, 2}, {-4, 3}, {5, 1}}},
			{Head_t::Disjunctive, {2}, Body_t::Sum, 1, {{3, 1}, {-4, 1}, {5, 1}}},
			// mixed
			{Head_t::Choice, {1, 2}, Body_t::Sum, 1, {{2, 1}, {-3, 2}, {-4, 3}, {5, 1}}},
			{Head_t::Disjunctive, {}, Body_t::Sum, 1, {{2, 1}, {-3, 2}, {-4, 3}, {5, 1}}},
		};
		for (auto&& r : rules) {
			HeadView h = {r.ht, toSpan(r.head)};
			BodyView b = {r.bt, r.bnd, toSpan(r.body)};
			writer.rule(h, b);
		}
		writer.endStep();
		readAspif(out, observer);
		for (auto&& r : rules) {
			REQUIRE(std::find(observer.rules.begin(), observer.rules.end(), r) != observer.rules.end());
		}
	}
	SECTION("Writer writes minimize") {
		auto m1 = Vec<WeightLit_t>{{1, -2}, {-3, 2}, {4, 1}};
		auto m2 = Vec<WeightLit_t>{{-10, 1}, {-20, 2}};
		writer.minimize(1, toSpan(m1));
		writer.minimize(-2, toSpan(m2));
		writer.endStep();
		readAspif(out, observer);
		REQUIRE(observer.min.size() == 2);
		REQUIRE(observer.min[0].first == 1);
		REQUIRE(observer.min[1].first == -2);
		REQUIRE(observer.min[0].second == m1);
		REQUIRE(observer.min[1].second == m2);
	}
	SECTION("Writer writes output") {
		std::pair<std::string, Vec<Lit_t> > exp[] ={
			{"Hallo", {1, -2, 3}},
			{"Fact", {}}
		};
		for (auto&& s : exp) { writer.output(toSpan(s.first), toSpan(s.second)); }
		writer.endStep();
		readAspif(out, observer);
		for (auto&& s : exp) { REQUIRE(std::find(observer.shows.begin(), observer.shows.end(), s) != observer.shows.end()); }
	}
	SECTION("Writer writes external") {
		std::pair<Atom_t, Value_t> exp[] ={
			{1, Value_t::Free},
			{2, Value_t::True},
			{3, Value_t::False},
			{4, Value_t::Release}
		};
		for (auto&& e : exp) {
			writer.external(e.first, e.second);
		}
		writer.endStep();
		readAspif(out, observer);
		for (auto&& e : exp) {
			REQUIRE(std::find(observer.externals.begin(), observer.externals.end(), e) != observer.externals.end());
		}
	}
	SECTION("Writer writes assumptions") {
		Lit_t a[] ={1, 987232, -2};
		writer.assume(toSpan(a, 2));
		writer.assume(toSpan(a+2, 1));
		writer.endStep();
		readAspif(out, observer);
		REQUIRE(observer.assumes.size() == 3);
		REQUIRE(std::equal(a, a+3, observer.assumes.begin()) == true);
	}
	SECTION("Writer writes projection") {
		Atom_t a[] ={1, 987232, 2};
		writer.project(toSpan(a, 2));
		writer.project(toSpan(a+2, 1));
		writer.endStep();
		readAspif(out, observer);
		REQUIRE(observer.projects.size() == 3);
		REQUIRE(std::equal(a, a+3, observer.projects.begin()) == true);
	}
	SECTION("Writer writes acyc edges") {
		Edge exp[] ={
			{0, 1, {1, -2}},
			{1, 0, {3}}
		};
		for (auto&& e : exp) { writer.acycEdge(e.s, e.t, toSpan(e.cond)); }
		writer.endStep();
		readAspif(out, observer);
		REQUIRE(observer.edges.size() == std::distance(std::begin(exp), std::end(exp)));
		REQUIRE(std::equal(std::begin(exp), std::end(exp), observer.edges.begin()) == true);
	}
	SECTION("Writer writes heuristics") {
		Heuristic exp[] ={
			{1, Heuristic_t::Sign, -1, 1, {10}},
			{2, Heuristic_t::Level, 10, 3, {-1, 10}},
			{1, Heuristic_t::Init, 20, 1, {}},
			{1, Heuristic_t::Factor, 2, 2, {}}
		};
		for (auto&& h : exp) {
			writer.heuristic(h.atom, h.type, h.bias, h.prio, toSpan(h.cond));
		}
		writer.endStep();
		readAspif(out, observer);
		REQUIRE(std::equal(std::begin(exp), std::end(exp), observer.heuristics.begin()) == true);
	}
}
TEST_CASE("TheoryData", "[aspif]") {
	TheoryData data;
	SECTION("Destruct invalid term") {
		data.addTerm(10, "Foo");
		data.reset();
	}
	SECTION("Term 0 is ok") {
		data.addTerm(0, 0);
		REQUIRE(data.hasTerm(0));
		REQUIRE(data.getTerm(0).type() == Theory_t::Number);
	}

	SECTION("Visit theory") {
		using Potassco::toSpan;
		Id_t tId = 0, s[3], n[5], o[2], f[3], e[4], args[2];
		// primitives
		data.addTerm(n[0] = tId++, 1); // (number 1)
		data.addTerm(n[1] = tId++, 2); // (number 2)
		data.addTerm(n[2] = tId++, 3); // (number 3)
		data.addTerm(n[3] = tId++, 4); // (number 4)
		data.addTerm(s[0] = tId++, toSpan("x")); // (string x)
		data.addTerm(s[1] = tId++, toSpan("z")); // (string z)
		// compounds
		data.addTerm(o[0] = tId++, toSpan("*")); // (operator *)
		data.addTerm(f[0] = tId++, s[0], toSpan(n, 1));   // (function x(1))
		data.addTerm(f[1] = tId++, s[0], toSpan(n+1, 1)); // (function x(2))
		data.addTerm(f[2] = tId++, s[0], toSpan(n+2, 1)); // (function x(3))
		args[0] = n[0]; args[1] = f[0];
		data.addTerm(e[0] = tId++, o[0], toSpan(args, 2)); // (1 * x(1))
		args[0] = n[1]; args[1] = f[1];
		data.addTerm(e[1] = tId++, o[0], toSpan(args, 2)); // (2 * x(2))
		args[0] = n[2]; args[1] = f[2];
		data.addTerm(e[2] = tId++, o[0], toSpan(args, 2)); // (3 * x(3))
		args[0] = n[3]; args[1] = s[1];
		data.addTerm(e[3] = tId++, o[0], toSpan(args, 2)); // (4 * z)
		// elements
		Id_t elems[4];
		data.addElement(elems[0] = 0, toSpan(&e[0], 1), 0u); // (element 1*x(1):)
		data.addElement(elems[1] = 1, toSpan(&e[1], 1), 0u); // (element 2*x(2):)
		data.addElement(elems[2] = 2, toSpan(&e[2], 1), 0u); // (element 3*x(3):)
		data.addElement(elems[3] = 3, toSpan(&e[3], 1), 0u); // (element 4*z:)
		
		// atom
		data.addTerm(s[2] = tId++, toSpan("sum"));       // (string sum)
		data.addTerm(o[1] = tId++, toSpan(">="));        // (string >=)
		data.addTerm(n[4] = tId++, 42);                  // (number 42)
		data.addAtom(1, s[2], toSpan(elems, 4), o[1], n[4]); // (&sum { 1*x(1); 2*x(2); 3*x(3); 4*z     } >= 42)
		
		struct Visitor : public TheoryData::Visitor {
			void visit(const TheoryData& data, Id_t termId, const TheoryTerm& t) override {
				if (out.hasTerm(termId)) return;
				switch (t.type()) {
					case Potassco::Theory_t::Number: out.addTerm(termId, t.number()); break;
					case Potassco::Theory_t::Symbol: out.addTerm(termId, t.symbol()); break;
					case Potassco::Theory_t::Compound:
						data.accept(t, *this);
						if (t.isFunction()) { out.addTerm(termId, t.function(), t.terms()); }
						else                { out.addTerm(termId, t.tuple(), t.terms()); }
						break;
				}
			}
			void visit(const TheoryData& data, Id_t elemId, const TheoryElement& e) override {
				if (out.hasElement(elemId)) return;
				data.accept(e, *this);
				out.addElement(elemId, e.terms(), e.condition());
			}
			void visit(const TheoryData& data, const TheoryAtom& a) override {
				data.accept(a, *this);
				if (!a.guard()) { out.addAtom(a.atom(), a.term(), a.elements()); }
				else            { out.addAtom(a.atom(), a.term(), a.elements(), *a.guard(), *a.rhs()); }
			}
			TheoryData out;
		} th;
		data.accept(th);
		REQUIRE(data.numAtoms() == th.out.numAtoms());
		for (Id_t id = 0; id != tId; ++id) {
			REQUIRE(data.hasTerm(id) == th.out.hasTerm(id));
			REQUIRE(data.getTerm(id).type() == th.out.getTerm(id).type());
		}
		for (Id_t id = 0; id != 4; ++id) {
			REQUIRE(data.hasElement(id) == th.out.hasElement(id));
		}
	}
}

}}}
