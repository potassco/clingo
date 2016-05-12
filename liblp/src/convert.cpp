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
#include <potassco/convert.h>
#include <algorithm>
#include <cstring>
#include <vector>
#include <map>
#include <stdio.h>
#include <string>
#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER > 1500) || (defined(_LIBCPP_VERSION))
#include <unordered_map>
typedef std::unordered_map<Potassco::Atom_t, const char*> SymTab;
#else
#if defined(_MSC_VER)
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif
typedef std::tr1::unordered_map<Potassco::Atom_t, const char*> SymTab;
#endif

#if defined(_MSC_VER)
#pragma warning (disable : 4996)
#define snprintf _snprintf
#endif
namespace Potassco {
/////////////////////////////////////////////////////////////////////////////////////////
// SmodelsConvert::SmData
/////////////////////////////////////////////////////////////////////////////////////////
struct SmodelsConvert::SmData {
	struct Atom {
		Atom() : smId(0), head(0), show(0), extn(0) {}
		operator Atom_t() const { return smId; }
		unsigned smId :  28;// corresponding smodels atom
		unsigned head :  1; // atom occurs in a head of a rule
		unsigned show :  1; // atom has a name
		unsigned extn :  2; // value if atom is external
	};
	struct Heuristic {
		Atom_t      atom;
		Heuristic_t type;
		int         bias;
		unsigned    prio;
		unsigned    cond;
	};
	struct Symbol {
		unsigned     atom : 31;
		unsigned     hash :  1;
		const char*  name;
		bool operator<(const Symbol& rhs) const { return atom < rhs.atom; }
	};
	typedef std::vector<Atom>           AtomMap;
	typedef std::vector<Atom_t>         AtomVec;
	typedef std::vector<WeightLit_t>    BodyVec;
	typedef std::vector<Heuristic>      HeuVec;
	typedef std::map<Weight_t, BodyVec> MinMap;
	typedef std::vector<Symbol>         OutVec;
	SmData() : next_(2) {}
	~SmData() {
		flushStep();
		for (SymTab::iterator it = symTab_.begin(), end = symTab_.end(); it != end; ++it) {
			delete [] it->second;
		}
	}
	HeadView mapHead(const HeadView& h);
	BodyView mapBody(const BodyView& b);
	Atom_t newAtom()   { return next_++; }
	Atom_t falseAtom() { return 1; }
	bool   mapped(Atom_t a) const {
		return a < atoms_.size() && atoms_[a].smId != 0;
	}
	Atom&  mapAtom(Atom_t a) {
		if (mapped(a)) { return atoms_[a]; }
		if (a >= atoms_.size()) { atoms_.resize(a + 1); }
		atoms_[a].smId = next_++;
		return atoms_[a];
	}
	Lit_t  mapLit(Lit_t in) {
		Lit_t x = static_cast<Lit_t>(mapAtom(atom(in)));
		return in < 0 ? -x : x;
	}
	Atom_t mapHeadAtom(Atom_t a) {
		Atom& x = mapAtom(a);
		x.head = 1;
		return x;
	}
	const char* addOutput(Atom_t atom, const StringSpan&, bool addHash);
	void addMinimize(Weight_t prio, const WeightLitSpan& lits) {
		BodyVec& body = minimize_[prio];
		body.reserve(body.size() + size(lits));
		for (const WeightLit_t* it = begin(lits); it != end(lits); ++it) {
			WeightLit_t x = *it;
			if (weight(x) < 0) {
				x.lit = -x.lit;
				x.weight = -x.weight;
			}
			body.push_back(x);
		}
	}
	void addExternal(Atom_t a, Value_t v) {
		Atom& ma = mapAtom(a);
		if (!ma.head) {
			ma.extn = static_cast<unsigned>(v);
			extern_.push_back(a);
		}
	}
	void addHeuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, Atom_t cond) {
		Heuristic h = {a, t, bias, prio, cond};
		heuristic_.push_back(h);
	}
	void flushStep() {
		minimize_.clear();
		AtomVec().swap(extern_);
		HeuVec().swap(heuristic_);
		for (; !output_.empty(); output_.pop_back()) {
			if (!output_.back().hash) { delete [] output_.back().name; }
		}
	}
	AtomMap atoms_;    // maps input atoms to output atoms
	MinMap  minimize_; // maps priorities to minimize statements
	AtomVec head_;     // active rule head
	BodyVec body_;     // active rule body
	AtomVec extern_;   // external atoms
	HeuVec  heuristic_;// list of heuristic modifications not yet processed
	SymTab  symTab_;
	OutVec  output_;   // list of output atoms not yet processed
	Atom_t  next_;     // next unused output atom
};
HeadView SmodelsConvert::SmData::mapHead(const HeadView& h) {
	head_.clear();
	for (const Atom_t* x = begin(h); x != end(h); ++x) {
		head_.push_back(mapHeadAtom(*x));
	}
	if (head_.empty() && h.type != Head_t::Choice) {
		head_.push_back(falseAtom());
	}
	return toHead(toSpan(head_), h.type);	
}
BodyView SmodelsConvert::SmData::mapBody(const BodyView& body) {
	body_.clear();
	for (const WeightLit_t* x = begin(body); x != end(body); ++x) {
		WeightLit_t w = {mapLit(lit(*x)), x->weight};
		body_.push_back(w);
	}
	return toBody(toSpan(body_), body.bound, body.type);
}
const char* SmodelsConvert::SmData::addOutput(Atom_t atom, const StringSpan& str, bool addHash) {
	char* n = new char[str.size + 1];
	*std::copy(begin(str), end(str), n) = 0;
	Symbol s; s.atom = atom; s.name = n; s.hash = 0;
	if (addHash && symTab_.insert(SymTab::value_type(atom, s.name)).second) {
		s.hash = 1;
	}
	output_.push_back(s);
	return s.name;
}
/////////////////////////////////////////////////////////////////////////////////////////
// SmodelsConvert
/////////////////////////////////////////////////////////////////////////////////////////
SmodelsConvert::SmodelsConvert(AbstractProgram& out, bool ext) : out_(out), data_(new SmData), ext_(ext) {}
SmodelsConvert::~SmodelsConvert() {
	delete data_;
}
Lit_t SmodelsConvert::get(Lit_t in) const {
	return data_->mapLit(in);
}
unsigned SmodelsConvert::maxAtom() const {
	return data_->next_ - 1;
}
const char* SmodelsConvert::getName(Atom_t a) const { 
	SymTab::iterator it = data_->symTab_.find(a);
	return it != data_->symTab_.end() ? it->second : 0; 
}
Atom_t SmodelsConvert::makeAtom(const LitSpan& cond, bool named) {
	Atom_t id = 0;
	if (size(cond) != 1 || *begin(cond) < 0 || (data_->mapAtom(atom(*begin(cond))).show && named)) {
		// aux :- cond.
		Atom_t aux = (id = data_->newAtom());
		data_->body_.clear();
		for (const Lit_t* x = begin(cond); x != end(cond); ++x) {
			WeightLit_t w = {data_->mapLit(*x), 1};
			data_->body_.push_back(w);
		}
		out_.rule(toHead(aux), toBody(toSpan(data_->body_)));
	}
	else {
		SmData::Atom& ma = data_->mapAtom(atom(*begin(cond)));
		ma.show = static_cast<unsigned>(named);
		id = ma.smId;
	}
	return id;
}
void SmodelsConvert::initProgram(bool inc) {
	out_.initProgram(inc);
}
void SmodelsConvert::beginStep() {
	out_.beginStep();
}
void SmodelsConvert::assume(const LitSpan&) {
	throw std::logic_error("assumption directive not supported!");
}
void SmodelsConvert::project(const AtomSpan&) {
	throw std::logic_error("projection directive not supported!");
}
void SmodelsConvert::rule(const HeadView& head, const BodyView& body) {
	HeadView mHead = data_->mapHead(head);
	if (size(mHead) == 0) {
		return;
	}
	BodyView mBody = data_->mapBody(body);
	if (isSmodelsRule(mHead, mBody)) {
		out_.rule(mHead, mBody);
		return;
	}
	Atom_t aux = data_->newAtom();
	out_.rule(toHead(aux), mBody);
	WeightLit_t auxLit = {static_cast<Lit_t>(aux), 1};
	out_.rule(mHead, toBody(toSpan(&auxLit, 1)));
}

void SmodelsConvert::minimize(Weight_t prio, const WeightLitSpan& lits) {
	data_->addMinimize(prio, lits);
}
void SmodelsConvert::output(const StringSpan& str, const LitSpan& cond) {
	// create a unique atom for cond and set its name to str
	data_->addOutput(makeAtom(cond, true), str, true);
}

void SmodelsConvert::external(Atom_t a, Value_t v) {
	data_->addExternal(a, v);
}
void SmodelsConvert::heuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, const LitSpan& cond) {
	if (!ext_) { out_.heuristic(a, t, bias, prio, cond); }
	// create unique atom representing _heuristic(...)
	Atom_t heuPred = makeAtom(cond, true);
	data_->addHeuristic(a, t, bias, prio, heuPred);
}
void SmodelsConvert::acycEdge(int s, int t, const LitSpan& condition) {
	if (!ext_) { out_.acycEdge(s, t, condition); }
	char buf[80];
	int n = snprintf(buf, sizeof(buf), "_edge(%d,%d)", s, t);
	if (n > 0 && static_cast<std::size_t>(n) < sizeof(buf)) {
		data_->addOutput(makeAtom(condition, true), toSpan(buf, std::strlen(buf)), false);
	}
}

void SmodelsConvert::flush() {
	flushMinimize();
	flushExternal();
	flushHeuristic();
	flushSymbols();
	Lit_t f = -static_cast<Lit_t>(data_->falseAtom());
	out_.assume(toSpan(&f, 1));
	data_->flushStep();
}
void SmodelsConvert::endStep() {
	flush();
	out_.endStep();
}
void SmodelsConvert::flushMinimize() {
	for (SmData::MinMap::iterator it = data_->minimize_.begin(), end = data_->minimize_.end(); it != end; ++it) {
		BodyView mBody = data_->mapBody(toBody(toSpan(it->second), 0));
		out_.minimize(it->first, span(mBody));
	}
}
void SmodelsConvert::flushExternal() {
	BodyView T = toBody(toSpan<WeightLit_t>());
	for (SmData::AtomVec::const_iterator it = data_->extern_.begin(), end = data_->extern_.end(); it != end; ++it) {
		SmData::Atom& a = data_->mapAtom(*it);
		Value_t vt = static_cast<Value_t>(a.extn);
		if (!ext_) {
			Atom_t at = a;
			if (!a.head && vt == Value_t::True) {
				out_.rule(toHead(at), T);
			}
			else if (!a.head && vt == Value_t::Free) {
				out_.rule(toHead(at, Head_t::Choice), T);
			}
		}
		else {
			out_.external(a, vt);
		}
	}
}
void SmodelsConvert::flushHeuristic() {
	for (SmData::HeuVec::const_iterator it = data_->heuristic_.begin(), end = data_->heuristic_.end(); it != end; ++it) {
		const SmData::Heuristic& heu = *it;
		if (!data_->mapped(heu.atom)) { continue; }
		SmData::Atom& ma = data_->mapAtom(heu.atom);
		const char* name = ma.show ? getName(ma.smId) : 0;
		if (!name) {
			ma.show = 1;
			char buf[80];
			snprintf(buf, sizeof(buf), "_atom(%u)", ma.smId);
			name = data_->addOutput(ma, toSpan(buf, std::strlen(buf)), true);
		}
		std::string heuPred = "_heuristic(";
		heuPred += name;
		char buf[80];
		snprintf(buf, sizeof(buf), ",%s,%d,%u)", toString(heu.type), heu.bias, heu.prio);
		heuPred.append(buf);
		Lit_t c = static_cast<Lit_t>(heu.cond);
		out_.output(toSpan(heuPred), toSpan(&c, 1));
	}
}
void SmodelsConvert::flushSymbols() {
	std::sort(data_->output_.begin(), data_->output_.end());
	for (SmData::OutVec::const_iterator it = data_->output_.begin(), end = data_->output_.end(); it != end; ++it) {
		Lit_t x = static_cast<Lit_t>(it->atom);
		out_.output(toSpan(it->name, std::strlen(it->name)), toSpan(&x, 1));
	}
}

}
