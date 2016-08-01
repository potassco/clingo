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
#include <functional>
#if defined(_MSC_VER)
#pragma warning (disable : 4996)
#endif
namespace Potassco {
/////////////////////////////////////////////////////////////////////////////////////////
// AspifInput
/////////////////////////////////////////////////////////////////////////////////////////
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
	BasicStack data;
	data_ = &data;
	out_.beginStep();
	for (unsigned rt; (rt = matchPos(Directive_t::eMax, "rule type or 0 expected")) != 0; ) {
		data.clear();
		switch (rt) {
			default: require(false, "unrecognized rule type");
			{case CR(Rule):
				Head_t  ht = static_cast<Head_t>(matchPos(Head_t::eMax, "invalid head type"));
				uint32_t n = matchAtoms();
				Body_t  bt = static_cast<Body_t>(matchPos(Body_t::eMax, "invalid body type"));
				if (bt == Body_t::Normal) {
					LitSpan body = data.popSpan<Lit_t>(matchLits());
					out_.rule(ht, data.popSpan<Atom_t>(n), body);
				}
				else {
					Weight_t bound = matchInt();
					WeightLitSpan body = data.popSpan<WeightLit_t>(matchWLits(0));
					out_.rule(ht, data.popSpan<Atom_t>(n), bound, body);
				}
				break;}
			{case CR(Minimize):
				Weight_t prio = matchInt();
				out_.minimize(prio, data.popSpan<WeightLit_t>(matchWLits(INT_MIN)));
				break;}
			case CR(Project):
				out_.project(data.popSpan<Atom_t>(matchAtoms()));
				break;
			{case CR(Output): 
				uint32_t len = matchString();
				LitSpan lits = data.popSpan<Lit_t>(matchLits());
				out_.output(data.popSpan<char>(len), lits);
				break;}
			case CR(External): 
				if (Atom_t atom = matchAtom()) {
					Value_t val = static_cast<Value_t>(matchPos(Value_t::eMax, "value expected"));
					out_.external(atom, val);
				}
				break;
			case CR(Assume):
				out_.assume(data.popSpan<Lit_t>(matchLits()));
				break;
			{case CR(Heuristic):
				Heuristic_t type = static_cast<Heuristic_t>(matchPos(Heuristic_t::eMax, "invalid heuristic modifier"));	
				Atom_t      atom = matchAtom();
				int         bias = matchInt();
				unsigned    prio = matchPos(INT_MAX, "invalid heuristic priority");
				out_.heuristic(atom, type, bias, prio, data.popSpan<Lit_t>(matchLits()));
				break;}
			{case CR(Edge): 
				unsigned start = matchPos(INT_MAX, "invalid edge, start node expected");
				unsigned end   = matchPos(INT_MAX, "invalid edge, end node expected");
				out_.acycEdge((int)start, (int)end, data.popSpan<Lit_t>(matchLits()));
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

uint32_t AspifInput::matchAtoms() {
	uint32_t len = matchPos("number of atoms expected");
	for (Atom_t* ptr = data_->makeSpan<Atom_t>(len), *end = ptr + len; ptr != end;) { *ptr++ = matchAtom(); }
	return len;
}
uint32_t AspifInput::matchLits() {
	uint32_t len = matchPos("number of literals expected");
	for (Lit_t* ptr = data_->makeSpan<Lit_t>(len), *end = ptr + len; ptr != end;) { *ptr++ = matchLit(); }
	return len;
}
uint32_t AspifInput::matchWLits(int32_t minW) {
	uint32_t len = matchPos("number of literals expected");
	for (WLit_t* ptr = data_->makeSpan<WeightLit_t>(len), *end = ptr + len; ptr != end;) { *ptr++ = matchWLit(minW); }
	return len;
}
uint32_t AspifInput::matchTermList() {
	uint32_t len = matchPos("number of terms expected");
	for (Id_t* ptr = data_->makeSpan<Id_t>(len), *end = ptr + len; ptr != end;) { *ptr++ = matchPos(); }
	return len;
}
uint32_t AspifInput::matchString() {
	uint32_t len = matchPos("non-negative string length expected");
	stream()->get();
	require(stream()->copy(data_->makeSpan<char>(len), (int)len) == (int)len, "invalid string");
	return len;
}

void AspifInput::matchTheory(unsigned rt) {
	Id_t tId = matchPos();
	switch (rt) {
		default: require(false, "unrecognized theory directive type");
		case Theory_t::Number: 
			out_.theoryTerm(tId, matchInt());
			break;
		case Theory_t::Symbol:
			out_.theoryTerm(tId, data_->popSpan<char>(matchString()));
			break;
		case Theory_t::Compound: {
			int type = matchInt(Tuple_t::eMin, INT_MAX, "unrecognized compound term type");
			out_.theoryTerm(tId, type, data_->popSpan<Id_t>(matchTermList()));
			break; 
		}
		case Theory_t::Element: {
			uint32_t nt = matchTermList();
			LitSpan lits = data_->popSpan<Lit_t>(matchLits());
			out_.theoryElement(tId, data_->popSpan<Id_t>(nt), lits);
			break;
		}
		case Theory_t::Atom: // fall through
		case Theory_t::AtomWithGuard: {
			Id_t termId = matchPos();
			uint32_t nt = matchTermList();
			if (rt == Theory_t::Atom) {
				out_.theoryAtom(tId, termId, data_->popSpan<Id_t>(nt));
			}
			else {
				Id_t opId = matchPos();
				out_.theoryAtom(tId, termId, data_->popSpan<Id_t>(nt), opId, matchPos());
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
void AspifOutput::rule(Head_t ht, const AtomSpan& head, const LitSpan& body) {
	startDir(Directive_t::Rule).add(static_cast<int>(ht)).add(head)
		.add(static_cast<int>(Body_t::Normal)).add(body)
		.endDir();
}
void AspifOutput::rule(Head_t ht, const AtomSpan& head, Weight_t bound, const WeightLitSpan& body) {
	startDir(Directive_t::Rule).add(static_cast<int>(ht)).add(head)
		.add(static_cast<int>(Body_t::Sum)).add(static_cast<int>(bound)).add(body)
		.endDir();
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
