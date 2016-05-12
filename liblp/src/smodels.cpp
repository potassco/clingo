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
#include <potassco/smodels.h>
#include <ostream>
#include <vector>
#include <string>
#include <cstring>
#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER > 1500) || (defined(_LIBCPP_VERSION))
#include <unordered_map>
typedef std::unordered_map<std::string, Potassco::Id_t> StrMap;
#else
#if defined(_MSC_VER)
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif
typedef std::tr1::unordered_map<std::string, Potassco::Id_t> StrMap;
#endif
namespace Potassco {

enum SmodelsRule {
	End         = 0,
	Basic       = 1, Cardinality = 2, Choice   = 3,
	Generate    = 4, Weight      = 5, Optimize = 6, 
	Disjunctive = 8,
	ClaspIncrement = 90, ClaspAssignExt = 91, ClaspReleaseExt = 92
};

int isSmodelsRule(const HeadView& head, const BodyView& body) {
	if (size(head) == 0) { return End; }
	int ret = Basic;
	if (head.type == Head_t::Choice || size(head) > 1) {
		ret = head.type == Head_t::Choice ? Choice : Disjunctive;
	}
	if (body.type == Body_t::Normal) {
		return ret;
	}
	else if (ret != Basic) {
		return End;
	}
	return body.type == Body_t::Sum ? Weight : Cardinality;
}
AtomTable::~AtomTable() {}
/////////////////////////////////////////////////////////////////////////////////////////
// SmodelsInput
/////////////////////////////////////////////////////////////////////////////////////////
struct SmodelsInput::SymTab : public AtomTable{
	SymTab(AbstractProgram& o) : out(&o) {}
	virtual void add(Atom_t id, const StringSpan& name, bool output) {
		atoms.insert(StrMap::value_type(std::string(Potassco::begin(name), Potassco::end(name)), id));
		if (output) {
			Lit_t lit = static_cast<Lit_t>(id);
			out->output(name, toSpan(&lit, 1));
		}
	}
	virtual Atom_t find(const StringSpan& name) {
		temp.assign(Potassco::begin(name), Potassco::end(name));
		StrMap::const_iterator it = atoms.find(temp);
		return it != atoms.end() ? it->second : 0;
	}
	struct Heuristic {
		std::string atom;
		Heuristic_t type;
		int         bias;
		unsigned    prio;
		Lit_t       cond;
	};
	StrMap           atoms;
	std::string      temp;
	AbstractProgram* out;
};
struct SmodelsInput::NodeTab {
	Id_t add(const StringSpan& n) {
		return nodes.insert(StrMap::value_type(std::string(begin(n), end(n)), (Id_t)nodes.size())).first->second;
	}
	StrMap nodes;
};
SmodelsInput::SmodelsInput(AbstractProgram& out, const Options& opts, AtomTable* syms) : out_(out), atoms_(syms), nodes_(0), opts_(opts), delSyms_(false) {}
SmodelsInput::~SmodelsInput() { if (delSyms_) delete atoms_; delete nodes_; }
void SmodelsInput::doReset() {}
bool SmodelsInput::doAttach(bool& inc) {
	char n = stream()->peek();
	if (BufferedStream::isDigit(n) && ((inc = (n == '9')) == false || opts_.claspExt)) {
		out_.initProgram(inc);
		return true;
	}
	return false;
}

bool SmodelsInput::doParse() {
	out_.beginStep();
	if (readRules() && readSymbols() && readCompute("B+", true) && readCompute("B-", false) && readExtra()) {
		out_.endStep();
		return true;
	}
	return false;
}

static BodyView matchBody(BufferedStream& str, std::vector<WeightLit_t>& lits, Body_t type) {
	Weight_t bound = Body_t::BOUND_NONE;
	lits.clear();
	if (type == Body_t::Sum) { bound = (Weight_t)matchPos(str); }
	unsigned len = matchPos(str);
	unsigned neg = matchPos(str);
	if (type == Body_t::Count) { bound = (Weight_t)matchPos(str); }
	for (; len--;) {
		WeightLit_t w ={static_cast<Lit_t>(matchAtom(str)), 1};
		if (neg) { w.lit *= -1; --neg; }
		lits.push_back(w);
	}
	if (type == Body_t::Sum) {
		for (unsigned i = 0, len = (unsigned)lits.size(); i != len; ++i) {
			lits[i].weight = (Weight_t)matchPos(str, "non-negative weight expected");
		}
	}
	return toBody(toSpan(lits), bound, type);
}

bool SmodelsInput::readRules() {
	std::vector<WeightLit_t> bodyLits;
	std::vector<Atom_t>      atoms;
	Weight_t minPrio = 0;
	for (unsigned rt; (rt = matchPos("rule type expected")) != 0;) {
		switch (rt) {
			default: require(false, "unrecognized rule type");
			case Cardinality: // fall through
			case Weight:      // fall through
			case Basic:
				if (Atom_t a = matchAtom()) {
					Body_t t = rt == Basic ? Body_t::Normal : (rt == Weight ? Body_t::Sum : Body_t::Count);
					BodyView body = matchBody(*stream(), bodyLits, t);
					out_.rule(toHead(a), body);
				}
				break;
			case Disjunctive: // fall through
			case Choice:      // n a1..an normal body
				if (unsigned n = matchPos()) {
					atoms.clear();
					while (n--) { atoms.push_back(matchAtom()); }
					BodyView body = matchBody(*stream(), bodyLits, Body_t::Normal);
					out_.rule(toHead(toSpan(atoms), rt == Choice ? Head_t::Choice : Head_t::Disjunctive), body);
				}
				else { require(false, "positive head size expected"); }
				break;
			case Optimize:
				require(matchBody(*stream(), bodyLits, Body_t::Sum).bound == 0, "unrecognized type of optimize rule");
				out_.minimize(minPrio++, toSpan(bodyLits));
				break;
			case ClaspIncrement:
				require(opts_.claspExt && matchPos() == 0, "unrecognized rule type");
				break;
			case ClaspAssignExt:
			case ClaspReleaseExt:
				require(opts_.claspExt, "unrecognized rule type");
				if (Atom_t a = matchAtom()) {
					Value_t v = Value_t::Release;
					if (rt == ClaspAssignExt) {
						v = static_cast<Value_t>((matchPos(2, "0..2 expected") ^ 3) - 1);
					}
					out_.external(a, v);
				}
				break;
		}
	}
	return true;
}

bool SmodelsInput::readSymbols() {
	std::string name;
	if (opts_.cEdge && !nodes_) { nodes_ = new NodeTab; }
	if (opts_.cHeuristic && !atoms_) { atoms_ = new SymTab(out_); delSyms_ = true; }
	StringSpan n0, n1;
	SymTab::Heuristic heu;
	std::vector<SymTab::Heuristic> doms;
	for (Lit_t atom; (atom = (Lit_t)matchPos()) != 0;) {
		name.clear();
		stream()->get();
		for (char c; (c = stream()->get()) != '\n';) { 
			require(c != 0, "atom name expected!");
			name += c;
		}
		const char* n = name.c_str();
		bool filter = false;
		if (opts_.cEdge && matchEdgePred(n, n0, n1) > 0) {
			Id_t s = nodes_->add(n0);
			Id_t t = nodes_->add(n1);
			out_.acycEdge(static_cast<int>(s), static_cast<int>(t), toSpan(&atom, 1));
			filter = opts_.filter;
		}
		else if (opts_.cHeuristic && matchDomHeuPred(n, n0, heu.type, heu.bias, heu.prio) > 0) {
			heu.cond = atom;
			heu.atom.assign(Potassco::begin(n0), Potassco::end(n0));
			doms.push_back(heu);
			filter = opts_.filter;
		}
		if      (atoms_)  { atoms_->add(atom, toSpan(name), !filter); }
		else if (!filter) { out_.output(toSpan(name), toSpan(&atom, 1)); }
	}
	for (std::vector<SymTab::Heuristic>::const_iterator it = doms.begin(), end = doms.end(); it != end; ++it) {
		if (Atom_t x = atoms_->find(toSpan(it->atom))) {
			out_.heuristic(x, it->type, it->bias, it->prio, toSpan(&it->cond, 1));
		}
	}
	if (!incremental()) { 
		delete nodes_;
		if (delSyms_) delete atoms_;
		nodes_ = 0;
		atoms_ = 0;
	}
	return true;
}

bool SmodelsInput::readCompute(const char* comp, bool val) {
	require(match(comp) && stream()->get() == '\n', "compute statement expected");
	WeightLit_t w ={0, 1};
	HeadView F = toHead(toSpan<Atom_t>());
	BodyView u = toBody(toSpan(&w, 1));
	while ((w.lit = (Lit_t)matchPos()) != 0) {
		if (val) { w.lit = -w.lit; }
		out_.rule(F, u);
	}
	return true;
}

bool SmodelsInput::readExtra() {
	if (match("E")) {
		for (Atom_t atom; (atom = matchPos()) != 0;) {
			out_.external(atom, Value_t::Free);
		}
	}
	matchPos("number of models expected");
	return true;
}

int readSmodels(std::istream& in, AbstractProgram& out, ErrorHandler err, const SmodelsInput::Options& opts) {
	SmodelsInput reader(out, opts);
	return readProgram(in, reader, err);
}
/////////////////////////////////////////////////////////////////////////////////////////
// SmodelsOutput
/////////////////////////////////////////////////////////////////////////////////////////
static inline WeightLit_t convert(const WeightLit_t& x) {
	if (x.weight >= 0) { return x; }
	WeightLit_t ret = {-x.lit, -x.weight};
	return ret;
}
SmodelsOutput::SmodelsOutput(std::ostream& os, bool ext) : os_(os), sec_(0), ext_(ext), inc_(false) {}
SmodelsOutput& SmodelsOutput::startRule(int rt) { os_ << rt; return *this; }
SmodelsOutput& SmodelsOutput::add(unsigned i)   { os_ << " " << i; return *this; }
SmodelsOutput& SmodelsOutput::add(const HeadView& head) {
	if (head.type == Head_t::Choice || size(head) > 1) { add((unsigned)size(head)); }
	for (const Atom_t* x = begin(head); x != end(head); ++x) { add(*x); }
	return *this;
}
SmodelsOutput& SmodelsOutput::add(int rt, const WeightLitSpan& body, unsigned bnd) {
	unsigned neg = 0;
	for (const WeightLit_t* x = begin(body); x != end(body); ++x) { 
		neg += static_cast<unsigned>(lit(convert(*x)) < 0);
	}
	if (rt == Weight) { add(bnd); }
	add((unsigned)size(body)).add(neg);
	if (rt == Cardinality)  { add(bnd); }
	unsigned n[2] = {neg, ((unsigned)size(body)) - neg};
	for (unsigned i = 0, k; i != 2; ++i) {
		if ((k = n[i]) == 0) { continue; }
		for (const WeightLit_t* x = begin(body); x != end(body); ++x) { 
			if (unsigned(lit(convert(*x)) > 0) == i) { add(atom(*x)); if (++k == n[i]) break; }
		}
	}
	if (rt == Weight || rt == Optimize) {
		for (unsigned i = 0, k; i != 2; ++i) {
			if ((k = n[i]) == 0) { continue; }
			for (const WeightLit_t* x = begin(body); x != end(body); ++x) { 
				WeightLit_t p = convert(*x);
				if (unsigned(lit(p) > 0) == i) { 
					add((unsigned)weight(p));
					if (++k == n[i]) break;
				}
			}
		}
	}
	return *this;
}
SmodelsOutput& SmodelsOutput::endRule() {
	os_ << "\n";
	return *this;
}
void SmodelsOutput::require(bool cnd, const char* msg) const {
	if (!cnd) { throw std::logic_error(msg); }
}
void SmodelsOutput::initProgram(bool b) { 
	inc_ = b;
	require(!inc_ || ext_, "incremental programs not supported in smodels format");
}
void SmodelsOutput::beginStep() {
	if (ext_ && inc_) { startRule(ClaspIncrement).add(0).endRule(); }
	sec_ = 0;
}
void SmodelsOutput::rule(const HeadView& head, const BodyView& body) {
	require(sec_ == 0, "adding rules after symbols not supported");
	SmodelsRule rt = (SmodelsRule)isSmodelsRule(head, body);
	require(rt != End, "unsupported rule type");
	startRule(rt).add(head).add(rt, span(body), static_cast<unsigned>(body.bound)).endRule();
}
void SmodelsOutput::minimize(Weight_t, const WeightLitSpan& lits) {
	startRule(Optimize).add(0).add(Optimize, lits, unsigned(0)).endRule();
}
void SmodelsOutput::output(const StringSpan& str, const LitSpan& cond) {
	require(sec_ <= 1, "adding symbols after compute not supported");
	require(size(cond) == 1 && lit(*begin(cond)) > 0, "general output directive not supported in smodels format");
	if (sec_ == 0) { startRule(End).endRule(); sec_ = 1; }
	os_ << unsigned(cond[0]) << " ";
	os_.write(begin(str), size(str));
	os_ << "\n";
}
void SmodelsOutput::external(Atom_t a, Value_t t) {
	require(ext_, "external directive not supported in smodels format");
	if (t != Value_t::Release) {
		startRule(ClaspAssignExt).add(a).add((unsigned(t)^3)-1).endRule();
	}
	else {
		startRule(ClaspReleaseExt).add(a).endRule();
	}
}
void SmodelsOutput::assume(const LitSpan& lits) {
	require(sec_ < 2, "at most one compute statement supported in smodels format");
	while (sec_ != 2) { startRule(End).endRule(); ++sec_; }
	os_ << "B+\n";
	for (const Lit_t* x = begin(lits); x != end(lits); ++x) {
		if (lit(*x) > 0) { os_ << atom(*x) << "\n"; }
	}
	os_ << "0\nB-\n";
	for (const Lit_t* x = begin(lits); x != end(lits); ++x) {
		if (lit(*x) < 0) { os_ << atom(*x) << "\n"; }
	}
	os_ << "0\n";
}
void SmodelsOutput::project(const AtomSpan&) {
	require(false, "projection directive not supported in smodels format");
}
void SmodelsOutput::acycEdge(int, int, const LitSpan&) {
	require(false, "edge directive not supported in smodels format");
}
void SmodelsOutput::heuristic(Atom_t, Heuristic_t, int, unsigned, const LitSpan&) {
	require(false, "heuristic directive not supported in smodels format");
}
void SmodelsOutput::endStep() {
	if (sec_ < 2) { SmodelsOutput::assume(Potassco::toSpan<Lit_t>()); }
	os_ << "1\n";
}
}
