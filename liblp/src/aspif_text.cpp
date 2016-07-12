// 
// Copyright (c) 2016, Benjamin Kaufmann
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
#include <potassco/aspif_text.h>
#include <cctype>
#include <cstring>
#include <vector>
namespace Potassco {

struct AspifTextInput::ParseData {
	std::vector<Atom_t>      atoms_;
	std::vector<Lit_t>       lits_;
	std::vector<WeightLit_t> wLits_;
	std::vector<char>        str_;
	AtomSpan      atoms() const { return toSpan(atoms_); }
	LitSpan       lits()  const { return toSpan(lits_); }
	WeightLitSpan wLits() const { return toSpan(wLits_); }
	StringSpan    str()   const { 
		return toSpan(&str_[0], str_.size() - 1); 
	}
	void clear() { atoms_.clear(); lits_.clear(); wLits_.clear(); str_.clear(); }
};

AspifTextInput::AspifTextInput(AbstractProgram* out) : out_(out), data_(0) {}
bool AspifTextInput::doAttach(bool& inc) {
	char n = peek(true);
	if (out_ && (!n || std::islower(static_cast<unsigned char>(n)) || std::strchr(".#%{:", n))) {
		while (n == '%') {
			skipLine();
			n = peek(true);
		}
		inc = match("#incremental", false) && require(match("."), "unrecognized directive");
		out_->initProgram(inc);
		return true;
	}
	return false;
}

bool AspifTextInput::doParse() {
	ParseData data;
	data_ = &data;
	out_->beginStep();
	if (!parseStatements()) { return false; }
	out_->endStep();
	return true;
}

bool AspifTextInput::parseStatements() {
	require(out_ != 0, "output not set");
	for (char c; (c = peek(true)) != 0;) {
		data_->clear();
		if      (c == '.') { match("."); }
		else if (c == '#') { matchDirective(); }
		else if (c == '%') { skipLine(); }
		else               { matchRule(c); }
	}
	data_->clear();
	return true;
}

void AspifTextInput::matchRule(char c) {
	HeadView H; H.type = Head_t::Disjunctive;
	BodyView B; B.type = Body_t::Normal; B.bound = Body_t::BOUND_NONE;
	if (c != ':') {
		if (c == '{') { match("{"); H.type = Head_t::Choice; matchAtoms(";,"); match("}"); }
		else          { matchAtoms(";|"); }
	}
	if (match(":-", false)) {
		c = peek(true);
		if (!StreamType::isDigit(c) && c != '-') {
			matchLits();
		}
		else {
			B.bound = matchInt();
			matchAgg();
			B.type = Body_t::Sum;
		}
	}
	match(".");
	H.atoms = data_->atoms();
	if (B.type == Body_t::Normal) {
		LitSpan body = data_->lits();
		for (LitSpan::iterator it = Potassco::begin(body), end = Potassco::end(body); it != end; ++it) {
			WeightLit_t wl = {*it, 1};
			data_->wLits_.push_back(wl);
		}
	}
	B.lits = data_->wLits();
	out_->rule(H, B);
}

void AspifTextInput::matchDirective() {
	if (match("#minimize", false)) {
		matchAgg();
		Weight_t prio = match("@", false) ? matchInt() : 0;
		match(".");
		out_->minimize(prio, data_->wLits());
	}
	else if (match("#project", false)) {
		if (match("{", false) && !match("}", false)) {
			matchAtoms(",");
			match("}");
		}
		match(".");
		out_->project(data_->atoms());
	}
	else if (match("#output", false)) {
		matchTerm();
		matchCondition();
		match(".");
		out_->output(data_->str(), data_->lits());
	}
	else if (match("#external", false)) {
		Atom_t  a = matchId();
		Value_t v = Value_t::False;
		match(".");
		if (match("[", false)) {
			if      (match("true", false))    { v = Value_t::True; }
			else if (match("free", false))    { v = Value_t::Free; }
			else if (match("release", false)) { v = Value_t::Release; }
			else                              { match("false"); }
			match("]");
		}
		out_->external(a, v);
	}
	else if (match("#assume", false)) {
		if (match("{", false) && !match("}", false)) {
			matchLits();
			match("}");
		}
		match(".");
		out_->assume(data_->lits());
	}
	else if (match("#heuristic", false)) {
		Atom_t a = matchId();
		matchCondition();
		match(".");
		match("[");
		int v = matchInt();
		int p = 0;
		if (match("@", false)) { p = matchInt(); require(p >= 0, "positive priority expected"); }
		match(",");
		int h = -1;
		for (unsigned x = 0; x <= static_cast<unsigned>(Heuristic_t::eMax); ++x) {
			if (match(toString(static_cast<Heuristic_t>(x)), false)) {
				h = static_cast<int>(x);
				break;
			}
		}
		require(h >= 0, "unrecognized heuristic modification");
		skipws();
		match("]");
		out_->heuristic(a, static_cast<Heuristic_t>(h), v, static_cast<unsigned>(p), data_->lits());
	}
	else if (match("#edge", false)) {
		int s, t;
		match("("), s = matchInt(), match(","), t = matchInt(), match(")");
		matchCondition();
		match(".");
		out_->acycEdge(s, t, data_->lits());
	}
	else if (match("#step", false)) {
		require(incremental(), "#step requires incremental program");
		match(".");
		out_->endStep();
		out_->beginStep();
	}
	else if (match("#incremental", false)) {
		match(".");
	}
	else {
		require(false, "unrecognized directive");
	}
}

void AspifTextInput::skipws() {
	stream()->skipWs();
}
bool AspifTextInput::match(const char* term, bool req) {
	if (ProgramReader::match(term, false)) { skipws(); return true; }
	else if (!req) { return false; }
	else {
		startString();
		push('\'');
		while (*term) { push(*term++); }
		term = "' expected";
		while (*term) { push(*term++); }
		push('\0');
		endString();
		return require(false, &data_->str_[0]);
	}
}
void AspifTextInput::matchAtoms(const char* seps) {
	for (;;) {
		data_->atoms_.push_back(matchId());
		if (!std::strchr(seps, stream()->peek())) {
			break;
		}
		stream()->get();
		skipws();
	}
}
void AspifTextInput::matchLits() {
	do {
		data_->lits_.push_back(matchLit());
	} while (match(",", false));
}
void AspifTextInput::matchCondition() {
	if (match(":", false)) { matchLits(); }
}
void AspifTextInput::matchAgg() {
	if (match("{") && !match("}", false)) {
		do {
			WeightLit_t wl = {matchLit(), 1};
			if (match("=", false)) { wl.weight = matchInt(); }
			data_->wLits_.push_back(wl);
		}
		while (match(",", false));
		match("}");
	}
}

Lit_t AspifTextInput::matchLit() {
	int s = match("not ", false) ? -1 : 1;
	return static_cast<Lit_t>(matchId()) * s;
}

int AspifTextInput::matchInt() {
	int i = ProgramReader::matchInt();
	skipws();
	return i;
}
Atom_t AspifTextInput::matchId() {
	char c = stream()->get();
	char n = stream()->peek();
	require(std::islower(static_cast<unsigned char>(c)) != 0, "<id> expected");
	require(std::islower(static_cast<unsigned char>(n)) == 0, "<pos-integer> expected");
	if (c == 'x' && (BufferedStream::isDigit(n) || n == '_')) {
		if (n == '_') { stream()->get(); }
		int i = matchInt();
		require(i > 0, "<pos-integer> expected");
		return static_cast<Atom_t>(i);
	}
	else {
		skipws();
		return static_cast<Atom_t>(c - 'a') + 1;
	}
}
void AspifTextInput::startString() {
	data_->str_.clear();
}
void AspifTextInput::endString() {
	data_->str_.push_back(0);
}
void AspifTextInput::push(char c) {
	data_->str_.push_back(c);
}

void AspifTextInput::matchTerm() {
	startString();
	char c = stream()->peek();
	if (std::islower(static_cast<unsigned char>(c)) != 0 || c == '_') {
		do { push(stream()->get()); } while (std::isalnum(static_cast<unsigned char>(c = stream()->peek())) != 0 || c == '_');
		skipws();
		if (match("(", false)) {
			push('(');
			for (;;) {
				matchAtomArg();
				if (!match(",", false)) break;
				push(',');
			}
			match(")");
			push(')');
		}
	}
	else if (c == '"') { matchStr(); }
	else { require(false, "<term> expected"); }
	skipws();
	endString();
}
void AspifTextInput::matchAtomArg() {
	char c;
	for (int p = 0; (c = stream()->peek()) != 0; ) {
		if (c == '"') {
			matchStr();
		}
		else {
			if      (c == ')' && --p < 0) { break; }
			else if (c == ',' &&  p == 0) { break; }
			p += int(c == '(');
			push(stream()->get());
			skipws();
		}
	}
}
void AspifTextInput::matchStr() {
	match("\""), push('"');
	bool quoted = false;
	for (char c; (c = stream()->peek()) != 0 && (c != '\"' || quoted);) {
		quoted = !quoted && c == '\\';
		push(stream()->get());
	}
	match("\""), push('"');
}
}
