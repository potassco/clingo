// 
// Copyright (c) 2006-2007, Benjamin Kaufmann
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
#include <clasp/reader.h>
#include <clasp/program_builder.h>
#include <clasp/clause.h>
#include <clasp/solver.h>
#include <clasp/smodels_constraints.h>
#include <limits.h>
#include <cassert>
#include <stdio.h>
#ifdef _MSC_VER
#define snprintf _snprintf
#pragma warning (disable : 4996)
#endif
namespace Clasp {
ReadError::ReadError(unsigned line, const char* msg) : ClaspError(format(line, msg)), line_(line) {}
std::string ReadError::format(unsigned line, const char* msg) {
	char buffer[1024];
	snprintf(buffer, 1023, "Read Error: Line %u, %s", line, msg);
	buffer[1023] = 0;
	return buffer;
}
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

char StreamSource::operator*() {
	if (buffer_[pos_] == 0) {
		underflow();
	}
	return buffer_[pos_];
}

StreamSource& StreamSource::operator++() {
	if (buffer_[pos_++] == '\n') {++line_;}
	**this;
	return *this;
}
	
bool StreamSource::parseInt( int& val) {
	val = 0;
	bool  pos = true;
	skipWhite();
	if (**this == '-') {
		pos = false;
		++*this;
	}
	if (**this == '+') {
		++*this;
	}
	bool ok = **this >= '0' && **this <= '9';
	int d;
	while (**this >= '0' && **this <= '9') {
		d    = **this - '0';
		assert((val < (INT_MAX/10)
			|| ((val==(INT_MAX/10)) && ((INT_MAX-(val*10))>=d)))
			&& "Integer overflow while parsing");
		val *= 10;
		val += d;
		++*this;
	}
	val = pos ? val : -val;
	return ok;
}

void StreamSource::skipWhite() {
	while ( **this == ' ' || **this == '\t' ) {
		++*this;
	}
}

bool StreamSource::readLine( char* buf, unsigned maxSize ) {
	if (maxSize == 0) return false;
	for (;;) {
		char ch = **this;
		++*this;
		if ( ch == '\0' || ch == '\n' || --maxSize == 0) break;
		*buf++ = ch;
	}
	*buf = 0;
	return maxSize != 0;
}

bool readLine(StreamSource& in, PodVector<char>::type& buf ) {
	char   buffer[1024];
	uint32 i;
	buf.clear();
	for (i = 0;;) {
		buffer[i] = *in;
		++in;
		if (buffer[i] == '\n' || buffer[i] == '\0') {
			buf.insert(buf.end(), buffer, buffer+i+1);
			buf.back() = '\0';
			break;
		}
		if (++i == 1024) {
			buf.insert(buf.end(), buffer, buffer+i);
			i = 0;
		}
	}
	return buffer[i] == '\n';
}
	
namespace {
/////////////////////////////////////////////////////////////////////////////////////////
// LPARSE PARSING
/////////////////////////////////////////////////////////////////////////////////////////
class LparseReader {
public:
	LparseReader();
	~LparseReader();
	bool parse(std::istream& prg, ProgramBuilder& api);
	void  clear();
	Var   parseAtom();
	bool  readRules();
	bool  readSymbolTable();
	bool  readComputeStatement();
	bool  readModels();
	bool  endParse();
	bool  readRule(int);
	bool  readBody(uint32 lits, uint32 neg, bool weights);
	PrgRule         rule_;
	StreamSource*   source_;
	ProgramBuilder* api_;
};

LparseReader::LparseReader()
	: source_(0)
	, api_(0) {
}

LparseReader::~LparseReader() {
	clear();
}

void LparseReader::clear() {
	rule_.clear();
	api_  = 0;
}

bool LparseReader::parse(std::istream& prg, ProgramBuilder& api) {
	clear();
	api_ = &api;
	if (!prg) {
		throw ReadError(0, "Could not read from stream!");
	}
	StreamSource source(prg);
	source_ = &source;
	return readRules()
		&& readSymbolTable()
		&& readComputeStatement()
		&& readModels()
		&& endParse();
}

Var LparseReader::parseAtom() {
	int r = -1;
	if (!source_->parseInt(r) || r < 1 || r > (int)varMax) {
		throw ReadError(source_->line(), (r == -1 ? "Atom id expected!" : "Atom out of bounds"));
	}
	return static_cast<Var>(r);
}

bool LparseReader::readRules() {
	int rt = -1;
	while ( skipAllWhite(*source_) && source_->parseInt(rt) && rt != 0 && readRule(rt) ) ;
	if (rt != 0) {
		throw ReadError(source_->line(), "Rule type expected!");
	}
	if (!match(*source_, '\n', true)) {
		throw ReadError(source_->line(), "Symbol table expected!");
	}
	return skipAllWhite(*source_);
}

bool LparseReader::readRule(int rt) {
	int bound = -1;
	if (rt <= 0 || rt > 6 || rt == 4) {
		throw ReadError(source_->line(), "Unsupported rule type!");
	}
	RuleType type(static_cast<RuleType>(rt));
	rule_.setType(type);
	if ( type == BASICRULE || rt == CONSTRAINTRULE || rt == WEIGHTRULE) {
		rule_.addHead(parseAtom());
		if (rt == WEIGHTRULE && (!source_->parseInt(bound) || bound < 0)) {
			throw ReadError(source_->line(), "Weightrule: Positive weight expected!");
		}
	}
	else if (rt == CHOICERULE) {
		int heads;
		if (!source_->parseInt(heads) || heads < 1) {
			throw ReadError(source_->line(), "Choicerule: To few heads");
		}
		for (int i = 0; i < heads; ++i) {
			rule_.addHead(parseAtom());
		}
	}
	else {
		assert(rt == 6);
		int x;
		if (!source_->parseInt(x) || x != 0) {
			throw ReadError(source_->line(), "Minimize rule: 0 expected!");
		}
	}
	int lits, neg;
	if (!source_->parseInt(lits) || lits < 0) {
		throw ReadError(source_->line(), "Number of body literals expected!");
	}
	if (!source_->parseInt(neg) || neg < 0 || neg > lits) {
		throw ReadError(source_->line(), "Illegal negative body size!");
	}
	if (rt == CONSTRAINTRULE && (!source_->parseInt(bound) || bound < 0)) {
		throw ReadError(source_->line(), "Constraint rule: Positive bound expected!");
	}
	if (bound >= 0) {
		rule_.setBound(static_cast<uint32>(bound));
	}
	return readBody(static_cast<uint32>(lits), static_cast<uint32>(neg), rt >= 5);  
}

bool LparseReader::readBody(uint32 lits, uint32 neg, bool readWeights) {
	for (uint32 i = 0; i != lits; ++i) {
		rule_.addToBody(parseAtom(), i >= neg, 1);
	}
	if (readWeights) {
		for (uint32 i = 0; i < lits; ++i) {
			int w;
			if (!source_->parseInt(w) || w < 0) {
				throw ReadError(source_->line(), "Weight Rule: bad or missing weight!");
			}
			rule_.body[i].second = w;
		}
	} 
	api_->addRule(rule_);
	rule_.clear();
	return match(*source_, '\n', true) ? true : throw ReadError(source_->line(), "Illformed rule body!");
}

bool LparseReader::readSymbolTable() {
	int a = -1;
	PodVector<char>::type buf;
	buf.reserve(1024);
	while (source_->parseInt(a) && a != 0) {
		if (a < 1) {
			throw ReadError(source_->line(), "Symbol Table: Atom id out of bounds!");
		}
		source_->skipWhite();
		if (!readLine(*source_, buf)) {
			throw ReadError(source_->line(), "Symbol Table: Atom name too long or end of file!");
		}
		api_->setAtomName(a, &buf[0]);
		skipAllWhite(*source_);
	}
	if (a != 0) {
		throw ReadError(source_->line(), "Symbol Table: Atom id expected!");
	}
	if (!match(*source_, '\n', true)) {
		throw ReadError(source_->line(), "Compute Statement expected!");
	}
	return skipAllWhite(*source_);
}

bool LparseReader::readComputeStatement() {
	char pos[2] = { '+', '-' };
	for (int i = 0; i != 2; ++i) {
		char sec[3];
		skipAllWhite(*source_);
		if (!source_->readLine(sec, 3) || sec[0] != 'B' || sec[1] != pos[i]) {
			throw ReadError(source_->line(), (i == 0 ? "B+ expected!" : "B- expected!"));
		}
		skipAllWhite(*source_);
		int id = -1;
		while (source_->parseInt(id) && id != 0) {
			if (id < 1) throw ReadError(source_->line(), "Compute Statement: Atom out of bounds");  
			api_->setCompute(static_cast<Var>(id), pos[i] == '+');
			if (!match(*source_, '\n', true)) {
				throw ReadError(source_->line(), "Newline expected!");
			}
			skipAllWhite(*source_);
		}
		if (id != 0) {
			throw ReadError(source_->line(), "Compute Statement: Atom id or 0 expected!");
		}
		if (!match(*source_, '\n', true)) {
			throw ReadError(source_->line(), (i == 0 ? "B- expected!" : "Number of models expected!"));
		}
	}
	return skipAllWhite(*source_);
}

bool LparseReader::readModels() {
	int m;
	if (!source_->parseInt(m) || m < 0) {
		throw ReadError(source_->line(), "Number of models expected!");
	}
	return skipAllWhite(*source_);
}

bool LparseReader::endParse() {
	return true;
} 

/////////////////////////////////////////////////////////////////////////////////////////
// DIMACS PARSING
/////////////////////////////////////////////////////////////////////////////////////////
bool parseDimacsImpl(std::istream& prg, Solver& s, bool assertPure) {
	LitVec currentClause;
	ClauseCreator nc(&s);
	SatPreprocessor* p = 0;
	int numVars = -1, numClauses = 0;
	bool ret = true;
	StreamSource in(prg);
	
	// For each var v: 0000p1p2c1c2
	// p1: set if v occurs negatively in any clause
	// p2: set if v occurs positively in any clause
	// c1: set if v occurs negatively in the current clause
	// c2: set if v occurs positively in the current clause
	PodVector<uint8>::type flags;
	for (;ret;) {
		skipAllWhite(in);
		if (*in == 0 && numVars != -1) {
			break;
		}
		else if (*in == 'p') {
			if (match(in, "p cnf", false)) {
				if (!in.parseInt(numVars) || !in.parseInt(numClauses) ) {
					throw ReadError(in.line(), "Bad parameters in the problem line!");
				}
				s.reserveVars(numVars+1);
				flags.resize(numVars+1);
				for (int v = 1; v <= numVars; ++v) {
					s.addVar(Var_t::atom_var);
				}
				// prepare solver/preprocessor for adding constraints
				s.startAddConstraints();
				// HACK: Don't waste time preprocessing a gigantic problem
				if (numClauses > 4000000) { p = s.strategies().satPrePro.release(); }
			}
			else {
				throw ReadError(in.line(), "'cnf'-format expected!");
			}
		} 
		else if (*in == 'c' || *in == 'p') {
			skipLine(in);
		}
		else if (numVars != -1) { // read clause
			int lit;
			Literal rLit;
			bool sat = false;
			nc.start();
			currentClause.clear();
			for (;;){
				if (!in.parseInt(lit)) {
					throw ReadError(in.line(), "Bad parameter in clause!");
				}
				else if (abs(lit) > numVars) {
					throw ReadError(in.line(), "Unrecognized format - variables must be numbered from 1 up to $VARS!");
				}
				skipAllWhite(in);
				rLit = lit >= 0 ? posLit(lit) : negLit(-lit);
				if (lit == 0) {
					for (LitVec::iterator it = currentClause.begin(); it != currentClause.end(); ++it) {
						flags[it->var()] &= ~3u; // clear "in clause"-flags
						if (!sat) { 
							// update "in problem"-flags
							flags[it->var()] |= ((1 + it->sign()) << 2);
						}
					}
					ret = sat || nc.end();
					break;
				}
				else if ( (flags[rLit.var()] & (1+rLit.sign())) == 0 ) {
					flags[rLit.var()] |= 1+rLit.sign();
					nc.add(rLit);
					currentClause.push_back(rLit);
					if ((flags[rLit.var()] & 3u) == 3u) sat = true;
				}
			}
		}
		else {
			throw ReadError(in.line(), "Unrecognized format - missing problem line!");
		}
	}
	if (p) s.strategies().satPrePro.reset(p);
	if (assertPure) {
		for (Var i = 1; ret && i <= s.numVars(); ++i) {
			uint8 d = (flags[i]>>2);
			if      (d == 0)  { ret = s.force(negLit(i), 0); }
			else if (d != 3)  { ret = s.force(Literal(i, d != 1), 0); }
		}
	}
	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OPB PARSING
/////////////////////////////////////////////////////////////////////////////////////////
#define ERROR(x) throw ReadError(in.line(), (x));
class OPBParser {
public:
	OPBParser(std::istream& in, Solver& s, ObjectiveFunction& obj) 
		: source_(in)
		, solver_(s)
		, obj_(obj) {
		obj_.lits.clear();
	}
	bool parse();
private:
	bool good() const   { return !solver_.hasConflict(); }
	void skipComments() { while (*source_ == '*') skipLine(source_); }
	void parseHeader();
	void parseOptObjective();
	void parseConstraint();
	void parseSum();
	void addConstraint(const char* relOp, int rhs);
	StreamSource       source_;
	WeightLitVec       lhs_;
	Solver&            solver_;
	ObjectiveFunction& obj_;
	int                numVars_;
	int                adjust_;
};

bool OPBParser::parse() {
	parseHeader();
	skipComments();
	parseOptObjective();
	skipComments();
	while (*source_ && good()) {
		parseConstraint();
		skipComments();
	}
	return good();
}

// * #variable= integer #constraint= integer
void OPBParser::parseHeader() {
	StreamSource& in = source_;
	if (!match(in, "* #variable=", false)){ ERROR("Missing problem line \"* #variable=\""); }
	in.skipWhite();
	if (!in.parseInt(numVars_))           { ERROR("Number of vars expected\n"); }
	if (!match(in, "#constraint=", true)) { ERROR("Bad problem line. Missing \"#constraint=\""); }
	int numCons;
	if (!in.parseInt(numCons))            { ERROR("Number of constraints expected\n"); }
	if (match(in, "#product=", true))     { ERROR("Bad problem line. Non-linear constraints are not supported"); }
	if (!match(in, '\n', true))           { ERROR("Unrecognized characters in problem line\n"); }
	
	// Init solver
	solver_.reserveVars(numVars_+1);
	for (int v = 1; v <= numVars_; ++v) {
		solver_.addVar(Var_t::atom_var);
	}
	solver_.startAddConstraints();
}

// <objective>::= "min:" <zeroOrMoreSpace> <sum>  ";"
void OPBParser::parseOptObjective() {
	if (match(source_, "min:", true)) {
		source_.skipWhite();
		parseSum();
		addConstraint(0, adjust_);
	}
}

// <constraint>::= <sum> <relational_operator> <zeroOrMoreSpace> <integer> <zeroOrMoreSpace> ";"
void OPBParser::parseConstraint() {
	parseSum();
	const char* relOp;
	StreamSource& in = source_;
	if (!match(in, (relOp="="), true) && !match(in, (relOp=">="), false)) {
		ERROR("Relational operator expected\n");
	}
	in.skipWhite();
	int coeff;
	if (!in.parseInt(coeff))   { ERROR("Missing coefficient on rhs of constraint\n"); }
	if (!match(in, ';', true)) { ERROR("Semicolon missing after constraint\n"); }
	skipAllWhite(in);
	addConstraint(relOp, coeff+adjust_);
}

// <sum>::= <weightedterm> | <weightedterm> <sum>
void OPBParser::parseSum() {
	StreamSource& in = source_;
	int  coeff, var;
	bool sign;
	adjust_ = 0;
	lhs_.clear();
	while (!match(in, ';', true)) {
		if (!in.parseInt(coeff)) { ERROR("Coefficient expected\n"); }
		match(in, '*', true);         // optionally
		sign  = match(in, '~', true); // optionally
		if (coeff < 0) {
			coeff   = -coeff;
			adjust_+= coeff;
			sign    = !sign;
		}
		if (!match(in, 'x', true) || !in.parseInt(var)) { ERROR("Identifier expected\n"); }
		if (var <= 0)       { ERROR("Identifier must be strictly positive\n"); }
		if (var > numVars_) { ERROR("Identifier out of bounds\n"); }
		lhs_.push_back(WeightLiteral(Literal(var, sign), coeff));
		in.skipWhite();
		if (*in == '>' || *in == '=') break;
	}
	skipAllWhite(in);
}
#undef ERROR

void OPBParser::addConstraint(const char* relOp, int rhs) {
	if (good()) {
		if (relOp != 0) {
			if (relOp[0] == '>') {
				assert(relOp[1] == '=');
				// TRUE == [lhs >= rhs]
				WeightConstraint::newWeightConstraint(solver_, posLit(0), lhs_, rhs);	
			}
			else {
				assert(relOp[0] == '=');
				// replace TRUE == [lhs = rhs] with
				// TRUE  == [lhs >= rhs]   and
				// FALSE == [lhs >= rhs+1]
				WeightLitVec temp(lhs_); // copy because newWeightConstraint() removes assigned literals 
				WeightConstraint::newWeightConstraint(solver_, posLit(0), lhs_, rhs) &&
				WeightConstraint::newWeightConstraint(solver_, negLit(0), temp, rhs+1);
			}
		}
		else {
			obj_.lits   = lhs_;	
			obj_.adjust = rhs;
		}
	}	
}
} // end of anonymous namespace


/////////////////////////////////////////////////////////////////////////////////////////
// interface functions
/////////////////////////////////////////////////////////////////////////////////////////
bool parseLparse(std::istream& prg, ProgramBuilder& api) {
	LparseReader reader;
	return reader.parse(prg, api);
}

bool parseDimacs(std::istream& prg, Solver& s, bool assertPure) {
	return parseDimacsImpl(prg, s, assertPure);
}

bool parseOPB(std::istream& prg, Solver& s, ObjectiveFunction& objective) {
	OPBParser parser(prg, s, objective);
	return parser.parse();
}

Input::Format detectFormat(std::istream& in) {
	std::istream::int_type x = in.peek();
	if (x != std::char_traits<char>::eof()) {
		unsigned char c = static_cast<unsigned char>(x);
		if (c >= '0' && c <= '9') return Input::SMODELS;
		if (c == 'c' || c == 'p') return Input::DIMACS;
		if (c == '*')             return Input::OPB;
	}
	throw ReadError(1, "Unrecognized input format!\n");
}

StreamInput::StreamInput(std::istream& in, Format f)
	: prg_(in), format_(f) 
{ }

bool StreamInput::read(Solver& s, ProgramBuilder* api, int numModels) {
	if (format_ == Input::SMODELS) {
		assert(api);
		return parseLparse(prg_, *api);
	}
	else if (format_ == Input::DIMACS) {
		return parseDimacs(prg_, s, numModels == 1);
	}
	else {
		return parseOPB(prg_, s, func_);
	}
}

MinimizeConstraint* StreamInput::getMinimize(Solver& s, ProgramBuilder* api, bool heu) {
	if (format_ == Input::SMODELS) {
		assert(api);
		return api->createMinimize(s, heu);
	}
	else if (!func_.lits.empty()) {
		MinimizeConstraint* min = new MinimizeConstraint();
		min->minimize(s, func_.lits, heu);
		assert(func_.adjust >= 0);
		if (func_.adjust > 0) {
			min->adjustSum(0, -func_.adjust);
		}
		return min;
	}
	return 0;
}
}
