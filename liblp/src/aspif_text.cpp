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
namespace Potassco {
AspifTextInput::AspifTextInput(AbstractProgram* out) : out_(out), strStart_(0), strPos_(0) {}
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
	out_->beginStep();
	if (!parseStatements()) { return false; }
	out_->endStep();
	BasicStack().swap(data_);
	return true;
}

bool AspifTextInput::parseStatements() {
	require(out_ != 0, "output not set");
	for (char c; (c = peek(true)) != 0;) {
		data_.clear();
		if      (c == '.') { match("."); }
		else if (c == '#') { if (!matchDirective()) break; }
		else if (c == '%') { skipLine(); }
		else               { matchRule(c); }
	}
	data_.clear();
	return true;
}

void AspifTextInput::matchRule(char c) {
	Head_t ht = Head_t::Disjunctive;
	Body_t bt = Body_t::Normal;
	if (c != ':') {
		if (c == '{') { match("{"); ht = Head_t::Choice; matchAtoms(";,"); match("}"); }
		else          { matchAtoms(";|"); }
	}
	else { data_.push(0u); }
	if (match(":-", false)) {
		c = peek(true);
		if (!StreamType::isDigit(c) && c != '-') {
			matchLits();
		}
		else {
			Weight_t bound = matchInt();
			matchAgg();
			data_.push(bound);
			bt = Body_t::Sum;
		}
	}
	else {
		data_.push(0u);
	}
	match(".");
	if (bt == Body_t::Normal) {
		LitSpan  body = data_.popSpan<Lit_t>(data_.pop<uint32_t>());
		AtomSpan head = data_.popSpan<Atom_t>(data_.pop<uint32_t>());
		out_->rule(ht, head, body);
	}
	else {
		typedef WeightLitSpan WLitSpan;
		Weight_t bound = data_.pop<Weight_t>();
		WLitSpan body = data_.popSpan<WeightLit_t>(data_.pop<uint32_t>());
		AtomSpan head = data_.popSpan<Atom_t>(data_.pop<uint32_t>());
		out_->rule(ht, head, bound, body);
	}
}

bool AspifTextInput::matchDirective() {
	if (match("#minimize", false)) {
		matchAgg();
		Weight_t prio = match("@", false) ? matchInt() : 0;
		match(".");
		out_->minimize(prio, data_.popSpan<WeightLit_t>(data_.pop<uint32_t>()));
	}
	else if (match("#project", false)) {
		uint32_t n = 0;
		if (match("{", false) && !match("}", false)) {
			matchAtoms(",");
			match("}");
			n = data_.pop<uint32_t>();
		}
		match(".");
		out_->project(data_.popSpan<Atom_t>(n));
	}
	else if (match("#output", false)) {
		matchTerm();
		matchCondition();
		match(".");
		LitSpan   cond = data_.popSpan<Lit_t>(data_.pop<uint32_t>());
		StringSpan str = data_.popSpan<char>(data_.pop<uint32_t>());
		out_->output(str, cond);
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
		uint32_t n = 0;
		if (match("{", false) && !match("}", false)) {
			matchLits();
			match("}");
			n = data_.pop<uint32_t>();
		}
		match(".");
		out_->assume(data_.popSpan<Lit_t>(n));
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
		out_->heuristic(a, static_cast<Heuristic_t>(h), v, static_cast<unsigned>(p), data_.popSpan<Lit_t>(data_.pop<uint32_t>()));
	}
	else if (match("#edge", false)) {
		int s, t;
		match("("), s = matchInt(), match(","), t = matchInt(), match(")");
		matchCondition();
		match(".");
		out_->acycEdge(s, t, data_.popSpan<Lit_t>(data_.pop<uint32_t>()));
	}
	else if (match("#step", false)) {
		require(incremental(), "#step requires incremental program");
		match(".");
		return false;
	}
	else if (match("#incremental", false)) {
		match(".");
	}
	else {
		require(false, "unrecognized directive");
	}
	return true;
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
		return require(false, data_.popSpan<char>(data_.pop<uint32_t>()).first);
	}
}
void AspifTextInput::matchAtoms(const char* seps) {
	for (uint32_t n = 0;;) {
		data_.push(matchId());
		++n;
		if (!std::strchr(seps, stream()->peek())) {
			data_.push(n);
			break;
		}
		stream()->get();
		skipws();
	}
}
void AspifTextInput::matchLits() {
	uint32_t n = 1;
	do {
		data_.push(matchLit());
	} while (match(",", false) && ++n);
	data_.push(n);
}
void AspifTextInput::matchCondition() {
	if (match(":", false)) { matchLits(); }
	else                   { data_.push(0u); }
}
void AspifTextInput::matchAgg() {
	uint32_t n = 0;
	if (match("{") && !match("}", false)) {
		do {
			WeightLit_t wl = {matchLit(), 1};
			if (match("=", false)) { wl.weight = matchInt(); }
			data_.push(wl);
		}
		while (++n && match(",", false));
		match("}");
	}
	data_.push(n);
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
	strStart_ = strPos_ = data_.top();
}
void AspifTextInput::endString() {
	data_.push(uint32_t(strPos_ - strStart_));
}
void AspifTextInput::push(char c) {
	if (strPos_ == data_.top()) { data_.push(0); }
	*(char*)data_.get(strPos_++) = c;
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
