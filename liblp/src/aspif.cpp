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

#include <potassco/aspif.h>
#include <potassco/theory_data.h>
#include <ostream>
#include <vector>
#include <cstring>
#if defined(_MSC_VER)
#pragma warning (disable : 4996)
#endif
namespace Potassco {
/////////////////////////////////////////////////////////////////////////////////////////
// AspifInput
/////////////////////////////////////////////////////////////////////////////////////////
struct AspifInput::ParseData {
	std::vector<Atom_t>      atoms;
	std::vector<WeightLit_t> bodyLits;
	std::vector<Lit_t>       lits;
	std::vector<char>        name;
};
AspifInput::AspifInput(AbstractProgram& out) : out_(out), data_(0) {}
AspifInput::~AspifInput() {  }

bool AspifInput::doAttach(bool& inc) {
	if (!match("asp ")) { return false; }
	require(matchPos() == 1, "unsupported major version");
	require(matchPos() == 0, "unsupported minor version");
	matchPos("revision number expected");
	while (match(" ", false)) { ; }
	inc  = match("incremental", false);
	out_.initProgram(inc);
	return require(stream()->get() == '\n', "invalid extra characters in problem line");
}
bool AspifInput::doParse() {
#define CR(r) Directive_t::r
	ParseData data;
	data_ = &data;
	out_.beginStep();
	for (unsigned rt; (rt = matchPos(Directive_t::eMax, "rule type or 0 expected")) != 0; ) {
		switch (rt) {
			default: require(false, "unrecognized rule type");
			{case CR(Rule):
				data.atoms.clear();
				Head_t ht = static_cast<Head_t>(matchPos(Head_t::eMax, "invalid head type"));
				for (unsigned n = matchPos("number of head atoms expected"); n--; ) {
					data.atoms.push_back(matchAtom());
				}
				Body_t  bt = static_cast<Body_t>(matchPos(Body_t::eMax, "invalid body type"));
				Weight_t B = Body_t::hasBound(bt) ? matchInt() : Body_t::BOUND_NONE;
				data.bodyLits.clear();
				for (unsigned n = matchPos("number of body literals expected"); n--;) {
					WeightLit_t w = {matchLit(), 1};
					if (Body_t::hasWeights(bt)) { w.weight = (int)matchPos(INT_MAX, "non-negative weight expected!"); }
					data.bodyLits.push_back(w);
				}
				out_.rule(toHead(toSpan(data.atoms), ht), toBody(toSpan(data.bodyLits), B, bt));
				break;}
			{case CR(Minimize):
				Weight_t prio = matchInt();
				data.bodyLits.clear();
				for (unsigned n = matchPos("number of body literals expected"); n--; ) {
					WeightLit_t w;
					w.lit    = matchLit();
					w.weight = matchInt();
					data.bodyLits.push_back(w);
				}
				out_.minimize(prio, toSpan(data.bodyLits));
				break;}
			case CR(Project): 
				data.atoms.clear();
				for (unsigned n = matchPos("number of atoms expected"); n--; ) {
					data.atoms.push_back(matchAtom());
				}
				out_.project(toSpan(data.atoms));
				break;
			{case CR(Output): 
				StringSpan name = matchString();
				out_.output(name, matchLits(*stream(), data.lits));
				break;}
			case CR(External): 
				if (Atom_t atom = matchAtom()) {
					Value_t val = static_cast<Value_t>(matchPos(Value_t::eMax, "value expected"));
					out_.external(atom, val);
				}
				break;
			case CR(Assume):
				out_.assume(matchLits(*stream(), data.lits));
				break;
			{case CR(Heuristic):
				Heuristic_t type = static_cast<Heuristic_t>(matchPos(Heuristic_t::eMax, "invalid heuristic modifier"));	
				Atom_t      atom = matchAtom();
				int         bias = matchInt();
				unsigned    prio = matchPos(INT_MAX, "invalid heuristic priority");
				out_.heuristic(atom, type, bias, prio, matchLits(*stream(), data.lits));
				break;}
			{case CR(Edge): 
				unsigned start = matchPos(INT_MAX, "invalid edge, start node expected");
				unsigned end   = matchPos(INT_MAX, "invalid edge, end node expected");
				out_.acycEdge((int)start, (int)end, matchLits(*stream(), data.lits));
				break;}
			case CR(Theory): matchTheory(matchPos()); break;
			case CR(Comment): skipLine(); break;
		}
	}
#undef CR
	out_.endStep();
	data_ = 0;
	return true;
}
StringSpan AspifInput::matchString() {
	data_->name.resize(matchPos("non-negative string length expected"));
	stream()->get();
	if (int len = (int)data_->name.size()) {
		require(stream()->copy(&data_->name[0], len) == len, "invalid string");
	}
	return toSpan(data_->name);
}
AtomSpan AspifInput::matchTermList() {
	data_->atoms.clear();
	for (unsigned n = matchPos(); n--;) { data_->atoms.push_back(matchPos()); }
	return toSpan(data_->atoms);
}
void AspifInput::matchTheory(unsigned rt) {
	Id_t tId = matchPos();
	switch (rt) {
		default: require(false, "unrecognized theory directive type");
		case Theory_t::Number: 
			out_.theoryTerm(tId, matchInt());
			break;
		case Theory_t::Symbol:
			out_.theoryTerm(tId, matchString());
			break;
		case Theory_t::Compound: {
			int type = matchInt(Tuple_t::eMin, INT_MAX, "unrecognized compound term type");
			out_.theoryTerm(tId, type, matchTermList());
			break; 
		}
		case Theory_t::Element: {
			AtomSpan ids = matchTermList();
			out_.theoryElement(tId, ids, matchLits(*stream(), data_->lits));
			break;
		}
		case Theory_t::Atom: // fall through
		case Theory_t::AtomWithGuard: {
			Id_t termId = matchPos();
			AtomSpan ids = matchTermList();
			if (rt == Theory_t::Atom) {
				out_.theoryAtom(tId, termId, ids);
			}
			else {
				Id_t opId = matchPos();
				out_.theoryAtom(tId, termId, ids, opId, matchPos());
			}
			break; 
		}
	}
}

int readAspif(std::istream& in, AbstractProgram& out, ErrorHandler err) {
	AspifInput reader(out);
	return readProgram(in, reader, err);
}
/////////////////////////////////////////////////////////////////////////////////////////
// AspifOutput
/////////////////////////////////////////////////////////////////////////////////////////
AspifOutput::AspifOutput(std::ostream& os) : os_(os) {
}

AspifOutput& AspifOutput::startDir(Directive_t r) {
	os_ << static_cast<unsigned>(r);
	return *this;
}
AspifOutput& AspifOutput::add(int x) {
	os_ << " " << x;
	return *this;
}
AspifOutput& AspifOutput::add(const WeightLitSpan& lits) {
	os_ << " " << size(lits);
	for (const WeightLit_t* x = begin(lits); x != end(lits); ++x) {
		os_ << " " << lit(*x) << " " << weight(*x);
	}
	return *this;
}
AspifOutput& AspifOutput::add(const LitSpan& lits) {
	os_ << " " << size(lits);
	for (const Lit_t* x = begin(lits); x != end(lits); ++x) {
		os_ << " " << lit(*x);
	}
	return *this;
}
AspifOutput& AspifOutput::add(const AtomSpan& atoms) {
	os_ << " " << size(atoms);
	for (const Atom_t* x = begin(atoms); x != end(atoms); ++x) { os_ << " " << *x; }
	return *this;
}
AspifOutput& AspifOutput::add(const StringSpan& str) {
	os_ << " " << size(str) << " ";
	os_.write(begin(str), size(str));
	return *this;
}
AspifOutput& AspifOutput::endDir() {
	os_ << "\n";
	return *this;
}
void AspifOutput::initProgram(bool inc) {
	os_ << "asp 1 0 0";
	if (inc) os_ << " incremental";
	os_ << "\n";
}
void AspifOutput::rule(const HeadView& head, const BodyView& body) {
	startDir(Directive_t::Rule).add(static_cast<int>(type(head))).add(span(head));
	if (body.type == Body_t::Normal) {
		add(static_cast<int>(Body_t::Normal)).add(static_cast<int>(size(body)));
		for (const WeightLit_t* x = begin(body); x != end(body); ++x) {
			add(lit(*x));
		}
	}
	else {
		add(static_cast<int>(Body_t::Sum)).add(static_cast<int>(body.bound)).add(span(body));
	}
	endDir();
}
void AspifOutput::minimize(Weight_t prio, const WeightLitSpan& lits) {
	startDir(Directive_t::Minimize).add(prio).add(lits).endDir();
}
void AspifOutput::output(const StringSpan& str, const LitSpan& cond) {
	startDir(Directive_t::Output).add(str).add(cond).endDir();
}
void AspifOutput::external(Atom_t a, Value_t v) {
	startDir(Directive_t::External).add(static_cast<int>(a)).add(static_cast<int>(v)).endDir();
}
void AspifOutput::assume(const LitSpan& lits) {
	startDir(Directive_t::Assume).add(lits).endDir();
}
void AspifOutput::project(const AtomSpan& atoms) {
	startDir(Directive_t::Project).add(atoms).endDir();
}
void AspifOutput::acycEdge(int s, int t, const LitSpan& cond) {
	startDir(Directive_t::Edge).add(s).add(t).add(cond).endDir();
}
void AspifOutput::heuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, const LitSpan& cond) {
	startDir(Directive_t::Heuristic).add(static_cast<int>(t)).add(static_cast<int>(a))
		.add(bias).add(static_cast<int>(prio)).add(cond)
		.endDir();
}
void AspifOutput::theoryTerm(Id_t termId, int number) {
	startDir(Directive_t::Theory).add(Theory_t::Number).add(termId).add(number).endDir();
}
void AspifOutput::theoryTerm(Id_t termId, const StringSpan& name) {
	startDir(Directive_t::Theory).add(Theory_t::Symbol).add(termId).add(name).endDir();
}
void AspifOutput::theoryTerm(Id_t termId, int cId, const IdSpan& args) {
	startDir(Directive_t::Theory).add(Theory_t::Compound).add(termId).add(cId).add(args).endDir();	
}
void AspifOutput::theoryElement(Id_t elementId, const IdSpan& terms, const LitSpan& cond) {
	startDir(Directive_t::Theory).add(Theory_t::Element).add(elementId).add(terms).add(cond).endDir();
}
void AspifOutput::theoryAtom(Id_t atomOrZero, Id_t termId, const IdSpan& elements) {
	startDir(Directive_t::Theory).add(Theory_t::Atom).add(atomOrZero).add(termId).add(elements).endDir();
}
void AspifOutput::theoryAtom(Id_t atomOrZero, Id_t termId, const IdSpan& elements, Id_t op, Id_t rhs) {
	startDir(Directive_t::Theory).add(Theory_t::AtomWithGuard).add(atomOrZero).add(termId).add(elements).add(op).add(rhs).endDir();
}

void AspifOutput::beginStep() {
}
void AspifOutput::endStep() {
	os_ << "0\n";
}
}
