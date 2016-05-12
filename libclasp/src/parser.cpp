// 
// Copyright (c) Benjamin Kaufmann
// 
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/ 
// 
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#include <clasp/parser.h>
#include <clasp/program_builder.h>
#include <clasp/logic_program.h>
#include <clasp/minimize_constraint.h>
#include <clasp/shared_context.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#ifdef _WIN32
#pragma warning (disable : 4996)
#endif
const char* clasp_format(char* buf, unsigned size, const char* fmt, ...) {
	if (size) { buf[0] = 0; --size; }
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, size, fmt, args);
	va_end(args);
	return buf;
}
const char* clasp_format_error(const char* fmt, ...) {
	static char buf[1024];
	buf[0] = 0;
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, 1023, fmt, args);
	va_end(args);
	return buf;
}
namespace Clasp {
ParseError::ParseError(unsigned a_line, const char* a_msg) : ClaspError(clasp_format_error("Parse Error: Line %u, %s", a_line, a_msg)), line(a_line) {}
/////////////////////////////////////////////////////////////////////////////////////////
// StreamSource
/////////////////////////////////////////////////////////////////////////////////////////
StreamSource::StreamSource(std::istream& is) : in_(is), pos_(0), line_(1) {
	underflow();
}

void StreamSource::underflow() {    
	pos_ = 0;
	buffer_[0] = 0;
	if (!in_) return;
	in_.read( buffer_, sizeof(buffer_)-1 );
	buffer_[in_.gcount()] = 0;
}

bool StreamSource::parseInt64(int64& val) {
	skipSpace();
	bool pos = match('+') || !match('-');
	int  d   = **this - '0';
	if (d < 0 || d > 9) { return false; }
	val      = 0;
	do {
		val *= 10;
		val += d;
		d    = *(++*this) - '0';
	} while (d >= 0 && d <= 9);
	val  = pos ? val : -val;
	return true;
}
bool StreamSource::parseInt(int& val) { 
	int64 x;
	return parseInt64(x)
		&&   x >= INT_MIN
		&&   x <= INT_MAX
		&&   (val=(int)x, true);
}
bool StreamSource::parseInt(int& val, int min, int max) { 
	int64 x;
	return parseInt64(x)
		&&   x >= min
		&&   x <= max
		&&   (val=(int)x, true);
}

bool StreamSource::matchEol() {
	if (match('\n')) {
		++line_;
		return true;
	}
	if (match('\r')) {
		match('\n');
		++line_;
		return true;
	}
	return false;
}

bool readLine(StreamSource& in, PodVector<char>::type& buf ) {
	char buffer[1024];
	bool eol = false;
	uint32 i;
	buf.clear();
	for (i = 0; *in && (eol = in.matchEol()) == false; ++in) {
		buffer[i] = *in;
		if (++i == 1024) {
			buf.insert(buf.end(), buffer, buffer+i);
			i = 0;
		}
	}
	buf.insert(buf.end(), buffer, buffer+i);
	buf.push_back('\0');
	return eol;
}

InputFormat Input_t::detectFormat(std::istream& in) {
	std::istream::int_type x = std::char_traits<char>::eof();
	while (in && (x = in.peek()) != std::char_traits<char>::eof() ) {
		unsigned char c = static_cast<unsigned char>(x);
		if (c >= '0' && c <= '9') return Problem_t::LPARSE;
		if (c == 'c' || c == 'p') return Problem_t::DIMACS;
		if (c == '*')             return Problem_t::OPB;
		if (c == ' ' || c == '\t') { in.get(); continue; }
		break;
	}
	char msg[] = "'c': Unrecognized input format!\n";
	msg[1]     = (char)(unsigned char)x;
	in && x != std::char_traits<char>::eof() 
		? throw ParseError(1, msg)
		: throw ParseError(0, "Bad input stream!\n");
}

bool Input_t::parseLparse(std::istream& prg, Asp::LogicProgram& api) {
	StreamSource input(prg);
	return DefaultLparseParser(api).parse(input);
}

bool Input_t::parseDimacs(std::istream& prg, SatBuilder& api) {
	StreamSource input(prg);
	return DimacsParser(api).parse(input);
}

bool Input_t::parseOPB(std::istream& prg, PBBuilder& api) {
	StreamSource input(prg);
	return OPBParser(api).parse(input);
}

StreamParser::StreamParser() : source_(0) {}
StreamParser::~StreamParser() {}
bool StreamParser::check(bool cond, const char* err) const { return cond || (source_->error(err), false); }
bool StreamParser::parse(StreamSource& prg) {
	source_ = &prg;
	return doParse();
}
// Skips lines starting with str
bool StreamParser::skipComments(const char* str) {
	while (match(*source_, str, false)) { skipLine(*source_); }
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
// LPARSE PARSING
/////////////////////////////////////////////////////////////////////////////////////////
LparseParser::LparseParser(Asp::LogicProgram& prg)
	: builder_(&prg)
	, active_(0) {
}
void LparseParser::setProgram(Asp::LogicProgram& prg) {
	builder_ = &prg;
}
Var LparseParser::parseAtom() {
	int r = -1;
	check(input()->parseInt(r, 1, (int)varMax), (r == -1 ? "Atom id expected!" : "Atom out of bounds"));
	return static_cast<Var>(r);
}
bool LparseParser::addRule(const Asp::Rule& r) const {
	builder_->addRule(r);
	return true;
}

bool LparseParser::doParse() {
	SingleOwnerPtr<Asp::Rule> active(new Asp::Rule());
	active_ = active.get();
	return parseRules()
		&& parseSymbolTable()
		&& parseComputeStatement()
		&& parseExtStatement()
		&& parseModels()
		&& endParse();
}

bool LparseParser::parseRules() {
	int rt = -1;
	while ( input()->skipWhite() && input()->parseInt(rt) && rt != 0 && parseRule(rt) ) {
		active_->clear();
	}
	return check(rt == 0, "Rule type expected!")
		&&   check(matchEol(*input(), true), "Symbol table expected!")
		&&   input()->skipWhite();
}

bool LparseParser::parseRule(int rt) {
	if (knownRuleType(rt)) {
		int  bound   = -1;
		bool weights = false;
		active_->setType(static_cast<Asp::RuleType>(rt));
		if (rt == Asp::CHOICERULE || rt == Asp::DISJUNCTIVERULE) {
			int heads = input()->parseInt(1, INT_MAX, "Rule has too few heads");
			for (int i = 0; i < heads; ++i) { active_->addHead(parseAtom()); }
		}
		else if (rt == Asp::OPTIMIZERULE) {
			weights = input()->parseInt(0, 0, "Minimize rule: 0 expected!") == 0;
		}
		else {
			active_->addHead(parseAtom());
			weights = rt == Asp::WEIGHTRULE && check(input()->parseInt(bound, 0, INT_MAX), "Weightrule: Positive weight expected!");
		}
		int lits = input()->parseInt(0, INT_MAX, "Number of body literals expected!");
		int neg  = input()->parseInt(0, lits,  "Illegal negative body size!");
		check(rt != Asp::CONSTRAINTRULE || input()->parseInt(bound, 0, INT_MAX), "Constraint rule: Positive bound expected!");
		if (bound >= 0) { active_->setBound(bound); }
		return parseBody(static_cast<uint32>(lits), static_cast<uint32>(neg), weights) && addRule(*active_);
	}
	else if (rt >= 90 && rt < 93) {
		if (rt == 90) { return input()->parseInt(0, 0, "0 expected") == 0; }
		int a = input()->parseInt(1, INT_MAX, "atom id expected");
		if (rt == 91) { builder_->freeze(a, static_cast<ValueRep>( (input()->parseInt(0, 2, "0..2 expected") ^ 3) - 1) ); }
		else          { builder_->unfreeze(a); }
		return true;
	}
	else {
		return parseRuleExtension(rt);
	}
}
bool LparseParser::parseBody(uint32 lits, uint32 neg, bool readWeights) {
	for (uint32 i = 0; i != lits; ++i) {
		active_->addToBody(parseAtom(), i >= neg, 1);
	}
	if (readWeights) {
		for (uint32 i = 0; i < lits; ++i) {
			active_->body[i].second = input()->parseInt(0, INT_MAX, "Weight Rule: bad or missing weight!");
		}
	} 
	return check(matchEol(*input(), true), "Illformed rule body!");
}
bool LparseParser::parseSymbolTable() {
	int a = -1;
	PodVector<char>::type buf;
	buf.reserve(1024);
	while (input()->skipWhite() && input()->parseInt(a) && a != 0) {
		check(a >= 1, "Symbol Table: Atom id out of bounds!");
		check(input()->skipSpace() && readLine(*input(), buf), "Symbol Table: Atom name too long or end of file!");
		builder_->setAtomName(a, &buf[0]);
	}
	check(a == 0, "Symbol Table: Atom id expected!");
	return check(matchEol(*input(), true), "Compute Statement expected!");
}

bool LparseParser::parseComputeStatement() {
	const char* B[2] = { "B+", "B-" };
	for (int i = 0; i != 2; ++i) {
		input()->skipWhite();
		check(match(*input(), B[i], false) && matchEol(*input(), true), (i == 0 ? "B+ expected!" : "B- expected!"));
		int id = -1;
		while (input()->skipWhite() && input()->parseInt(id) && id != 0) {
			check(id >= 1, "Compute Statement: Atom out of bounds");
			builder_->setCompute(static_cast<Var>(id), B[i][1] == '+');
		}
		check(id == 0, "Compute Statement: Atom id or 0 expected!");
	}
	return true;
}

bool LparseParser::parseExtStatement() {
	input()->skipWhite();
	if (match(*input(), 'E', false)) {
		for (int id; input()->skipWhite() && input()->parseInt(id) && id != 0;) {
			builder_->freeze(id, value_free);
		}
	}
	return true;
}

bool LparseParser::parseModels() {
	int m = 1;
	check(input()->skipWhite() && input()->parseInt(m, 0, INT_MAX), "Number of models expected!");
	return true;
}

bool LparseParser::endParse() { return true; } 
DefaultLparseParser::DefaultLparseParser(Asp::LogicProgram& api) : LparseParser(api) {}
bool DefaultLparseParser::parseRuleExtension(int) { input()->error("Unsupported rule type!"); return false; }
/////////////////////////////////////////////////////////////////////////////////////////
// DIMACS PARSING
/////////////////////////////////////////////////////////////////////////////////////////
DimacsParser::DimacsParser(SatBuilder& prg) : builder_(&prg) {}
void DimacsParser::setProgram(SatBuilder& prg) { builder_ = &prg; }

bool DimacsParser::doParse() {
	parseHeader();
	parseClauses();
	check(!**input(), "Unrecognized format!");
	return true;
}
// Parses the p line: p [w]cnf #vars #clauses [max clause weight]
void DimacsParser::parseHeader() {
	skipComments("c");
	check(match(*input(), "p ", false), "Missing problem line!");
	wcnf_        = input()->match('w');
	check(match(*input(), "cnf", false), "Unrecognized format!");
	numVar_      = input()->parseInt(0, (int)varMax, "#vars expected!");
	uint32 numC  = (uint32)input()->parseInt(0, INT_MAX, "#clauses expected!");
	wsum_t cw    = 0;
	if (wcnf_) { input()->parseInt64(cw); }
	builder_->prepareProblem(numVar_, cw, numC);
	input()->skipWhite();
}

void DimacsParser::parseClauses() {
	LitVec cc;
	const bool wcnf = wcnf_;
	wsum_t     cw   = 0;
	const int  numV = numVar_;
	while (input()->skipWhite() && skipComments("c") && **input()) {
		cc.clear();
		if (wcnf) { check(input()->parseInt64(cw) && cw > 0, "wcnf: clause weight expected!"); }
		for (int lit; (lit = input()->parseInt(-numV, numV, "Invalid variable in clause!")) != 0;) {
			cc.push_back( Literal(static_cast<uint32>(lit > 0 ? lit : -lit), lit < 0) );
			input()->skipWhite();
		}
		builder_->addClause(cc, cw);
	}
	input()->skipWhite();
}

/////////////////////////////////////////////////////////////////////////////////////////
// OPB PARSING
/////////////////////////////////////////////////////////////////////////////////////////
OPBParser::OPBParser(PBBuilder& prg) : builder_(&prg) {}
void OPBParser::setProgram(PBBuilder& prg) { builder_ = &prg; }
bool OPBParser::doParse() {
	parseHeader();
	skipComments("*");
	parseOptObjective();
	for (;;) {
		skipComments("*");
		if (!**input()) { return true; }
		parseConstraint();
	}
}

// * #variable= int #constraint= int [#product= int sizeproduct= int] [#soft= int mincost= int maxcost= int sumcost= int]
// where [] indicate optional parts, i.e.
//  LIN-PBO: * #variable= int #constraint= int
//  NLC-PBO: * #variable= int #constraint= int #product= int sizeproduct= int
//  LIN-WBO: * #variable= int #constraint= int #soft= int mincost= int maxcost= int sumcost= int
//  NLC-WBO: * #variable= int #constraint= int #product= int sizeproduct= int #soft= int mincost= int maxcost= int sumcost= int
void OPBParser::parseHeader() {
	check(match(*input(), "* #variable=", false), "Missing problem line '* #variable='!");
	input()->skipWhite();
	int numV = input()->parseInt(0, (int)varMax, "Number of vars expected");
	check(match(*input(), "#constraint=", true), "Bad problem line. Missing '#constraint='!");
	int numC = input()->parseInt(0, INT_MAX, "Number of constraints expected!");
	int numProd = 0, sizeProd = 0, numSoft = 0;
	minCost_    = 0, maxCost_ = 0;
	while (match(*input(), "#", true)) {
		if (match(*input(), "product=", true)) { // NLC instance
			numProd = input()->parseInt(0, INT_MAX, "Positive integer expected!");
			check(match(*input(), "sizeproduct=", true), "'sizeproduct=' expected!");
			sizeProd= input()->parseInt(0, INT_MAX, "Positive integer expected!");
			(void)sizeProd;
		}
		else if (match(*input(), "soft=", true)) { // WBO instance
			numSoft = input()->parseInt(0, INT_MAX, "Positive integer expected!");
			check(match(*input(), "mincost=", true), "'mincost=' expected!");
			minCost_= input()->parseInt(0, INT_MAX, "Positive integer expected!");
			check(match(*input(), "maxcost=", true), "'maxcost=' expected!");
			maxCost_= input()->parseInt(0, INT_MAX, "Positive integer expected!");
			check(match(*input(), "sumcost=", true), "'sumcost=' expected!");
			wsum_t sum;
			check(input()->parseInt64(sum) && sum > 0, "Positive integer expected!");
		}
		else { input()->error("Unrecognized problem line!"); }
	}
	check(matchEol(*input(), true), "Unrecognized characters in problem line!");
	builder_->prepareProblem(numV, numProd, numSoft, numC);
}

// <objective>::= "min:" <zeroOrMoreSpace> <sum>  ";"
// OR
// <softobj>  ::= "soft:" [<unsigned_integer>] ";"
void OPBParser::parseOptObjective() {
	if (match(*input(), "min:", true)) {
		input()->skipWhite();
		parseSum();
		builder_->addObjective(active_.lits); 
	}
	else if (match(*input(), "soft:", true)) {
		wsum_t softCost;
		check(input()->parseInt64(softCost) && softCost > 0, "Positive integer expected!");
		check(match(*input(), ';', true), "Semicolon missing after constraint!");
		builder_->setSoftBound(softCost);
		input()->skipWhite();
	}
}

// <constraint>::= <sum> <relational_operator> <zeroOrMoreSpace> <integer> <zeroOrMoreSpace> ";"
// OR
// <softconstr>::= "[" <zeroOrMoreSpace> <unsigned_integer> <zeroOrMoreSpace> "]" <constraint>
void OPBParser::parseConstraint() {
	weight_t cost = 0;
	if (match(*input(), '[', true)) {
		cost = input()->parseInt(minCost_, maxCost_, "Invalid soft constraint cost!");
		check(match(*input(), "]", true), "Invalid soft constraint!");
	}
	parseSum();
	active_.eq = match(*input(), "=", true);
	check(active_.eq || match(*input(), ">=", false), "Relational operator expected!");
	input()->skipWhite();
	active_.bound = input()->parseInt(INT_MIN, INT_MAX, "Invalid coefficient on rhs of constraint!");
	check(match(*input(), ';', true), "Semicolon missing after constraint!");
	builder_->addConstraint(active_.lits, active_.bound, active_.eq, cost);
	input()->skipWhite();
}

// <sum>::= <weightedterm> | <weightedterm> <sum>
// <weightedterm>::= <integer> <oneOrMoreSpace> <term> <oneOrMoreSpace>
void OPBParser::parseSum() {
	active_.lits.clear();
	while (!match(*input(), ';', true)) {
		int coeff = input()->parseInt(INT_MIN+1, INT_MAX, "Coefficient expected!");
		parseTerm();
		Literal x = term_.size() == 1 ? term_[0] : builder_->addProduct(term_);
		active_.lits.push_back(WeightLiteral(x, coeff));
		if (**input() == '>' || **input() == '=') break;
		input()->skipWhite();
	}
	input()->skipWhite();
}
// <term>::=<variablename>
// OR
// <term>::= <literal> | <literal> <space>+ <term>
void OPBParser::parseTerm() {
	term_.clear();
	char peek;
	do  {
		match(*input(), '*', true);             // optionally
		bool sign = match(*input(), '~', true); // optionally
		check(match(*input(), 'x', true), "Identifier expected!");
		int var   = input()->parseInt(1, builder_->numVars(), "Invalid identifier!");
		term_.push_back(Literal((uint32)var, sign));
		input()->skipWhite();
		peek = **input();
	} while (peek == '*' || peek == '~' || peek == 'x');
}

}
