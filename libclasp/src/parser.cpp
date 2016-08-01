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
#include <clasp/dependency_graph.h>
#include <clasp/minimize_constraint.h>
#include <clasp/shared_context.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <potassco/theory_data.h>
#include <potassco/aspif.h>
#include <potassco/smodels.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <string>
#ifdef _MSC_VER
#pragma warning (disable : 4996)
#endif
const char* clasp_format(char* buf, unsigned size, const char* fmt, ...) {
	if (!size) return buf;
	*buf = 0;
	va_list args;
	va_start(args, fmt);
	int r = vsnprintf(buf, size, fmt, args);
	if (r < 0 || unsigned(r) >= size) { buf[size-1] = 0; }
	va_end(args);
	return buf;
}
ClaspErrorString::ClaspErrorString(const char* fmt, ...) {
	*str = 0;
	va_list args;
	va_start(args, fmt);
	int res = vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);
	if (res < 0 || std::size_t(res) >= sizeof(str)) { str[sizeof(str)-1] = 0; }
}
namespace Clasp {
using Potassco::ParseError;
ProblemType detectProblemType(std::istream& in) {
	std::istream::int_type x = std::char_traits<char>::eof();
	while (in && (x = in.peek()) != std::char_traits<char>::eof() ) {
		char c = static_cast<char>(x);
		if (c == ' ' || c == '\t')  { in.get(); continue; }
		if (AspParser::accept(c))   { return Problem_t::Asp; }
		if (DimacsReader::accept(c)){ return Problem_t::Sat; }
		if (OpbReader::accept(c))   { return Problem_t::Pb;  }
		break;
	}
	char msg[] = "'c': unrecognized input format";
	msg[1]     = (char)(unsigned char)x;
	in && x != std::char_traits<char>::eof() 
		? throw ParseError(1, msg)
		: throw ParseError(0, "bad input stream");
}
/////////////////////////////////////////////////////////////////////////////////////////
// ProgramParser
/////////////////////////////////////////////////////////////////////////////////////////
ProgramParser::ProgramParser() : strat_(0) {}
ProgramParser::~ProgramParser() {}
bool ProgramParser::accept(std::istream& str, const ParserOptions& o) {
	if ((strat_ = doAccept(str, o)) != 0) {
		strat_->setMaxVar(VAR_MAX);
		return true;
	}
	return false;
}
bool ProgramParser::isOpen() const {
	return strat_ != 0;
}
bool ProgramParser::incremental() const {
	return strat_ && strat_->incremental();
}
bool ProgramParser::parse() {
	return strat_ && strat_->parse();
}
bool ProgramParser::more() {
	return strat_ && strat_->more();
}
void ProgramParser::reset() {
	if (strat_) { strat_->reset(); }
	strat_ = 0;
}
/////////////////////////////////////////////////////////////////////////////////////////
// AspParser::SmAdapter
// 
// Callback interface for smodels parser
/////////////////////////////////////////////////////////////////////////////////////////
struct AspParser::SmAdapter : public Asp::LogicProgramAdapter, public Potassco::AtomTable {
	typedef Clasp::HashMap_t<ConstString, Var, StrHash, StrEq>::map_type StrMap;
	typedef SingleOwnerPtr<StrMap> StrMapPtr;
	SmAdapter(Asp::LogicProgram& prg) : Asp::LogicProgramAdapter(prg) {}
	void endStep() {
		Asp::LogicProgramAdapter::endStep();
		if (inc_ && lp_->ctx()->hasMinimize()) {
			lp_->ctx()->removeMinimize();
		}
		if (!inc_) { atoms_ = 0; }
	}
	void add(Potassco::Atom_t id, const Potassco::StringSpan& name, bool output) {
		ConstString n(name);
		if (atoms_.get()) { atoms_->insert(StrMap::value_type(n, id)); }
		if (output) { lp_->addOutput(n, id); }
	}
	Potassco::Atom_t find(const Potassco::StringSpan& name) {
		if (!atoms_.get()) { return 0; }
		ConstString n(name);
		StrMap::iterator it = atoms_->find(n);
		return it != atoms_->end() ? it->second : 0;
	}
	StrMapPtr atoms_;
};
/////////////////////////////////////////////////////////////////////////////////////////
// AspParser 
/////////////////////////////////////////////////////////////////////////////////////////
AspParser::AspParser(Asp::LogicProgram& prg)
	: lp_(&prg)
	, in_(0)
	, out_(0) {}
AspParser::~AspParser() {
	delete in_;
	delete out_;
}
bool AspParser::accept(char c) { return Potassco::BufferedStream::isDigit(c) || c == 'a'; }

AspParser::StrategyType* AspParser::doAccept(std::istream& str, const ParserOptions& o) {
	delete in_;
	delete out_;
	if (Potassco::BufferedStream::isDigit((char)str.peek())) {
		out_ = new SmAdapter(*lp_);
		Potassco::SmodelsInput::Options so;
		so.enableClaspExt();
		if (o.isEnabled(ParserOptions::parse_heuristic)) {
			so.convertHeuristic();
			static_cast<SmAdapter*>(out_)->atoms_ = new AspParser::SmAdapter::StrMap();
		}
		if (o.isEnabled(ParserOptions::parse_acyc_edge)) {
			so.convertEdges();
		}
		in_ = new Potassco::SmodelsInput(*out_, so, static_cast<SmAdapter*>(out_));
	}
	else {
		out_ = new Asp::LogicProgramAdapter(*lp_);
		in_ = new Potassco::AspifInput(*out_);
	}
	return in_->accept(str) ? in_ : 0;
}

void AspParser::write(Asp::LogicProgram& prg, std::ostream& os) {
	write(prg, os, prg.supportsSmodels() ? format_smodels : format_aspif);
}
void AspParser::write(Asp::LogicProgram& prg, std::ostream& os, Format f) {
	using namespace Potassco;
	SingleOwnerPtr<AbstractProgram> out;
	if (f == format_aspif) {
		out.reset(new Potassco::AspifOutput(os));
	}
	else {
		out.reset(new Potassco::SmodelsOutput(os, true, prg.falseAtom()));
	}
	if (prg.startAtom() == 1) { out->initProgram(prg.isIncremental()); }
	out->beginStep();
	prg.accept(*out);
	out->endStep();
}
/////////////////////////////////////////////////////////////////////////////////////////
// Common sat parsing
/////////////////////////////////////////////////////////////////////////////////////////
SatReader::SatReader() {}
bool SatReader::skipLines(char c) {
	while (peek(true) == c) { skipLine(); }
	return true;
}
void SatReader::parseGraph(const char* pre, ExtDepGraph& graph) {
	int maxNode = matchPos("graph: positive number of nodes expected");
	while (match(pre)) {
		if      (match("node ")) { skipLine(); }
		else if (match("arc "))  {
			int neg = match("-");
			match("x"); /* ignore lit prefix */
			Var lit = matchAtom("graph: invalid edge variable");
			Var beg = matchPos(maxNode, "graph: invalid start node");
			Var end = matchPos(maxNode, "graph: invalid end node");
			graph.addEdge(Literal(lit, neg != 0), beg, end);
		}
		else if (match("endgraph")) { return; }
		else { break; }
	}
	require(false, "graph: endgraph expected");
}

SatParser::SatParser(SatBuilder& prg) : reader_(new DimacsReader(prg)) {}
SatParser::SatParser(PBBuilder& prg)  : reader_(new OpbReader(prg)) {}
SatParser::~SatParser() { delete reader_; }
ProgramParser::StrategyType* SatParser::doAccept(std::istream& str, const ParserOptions& o) {
	reader_->options = o;
	return reader_->accept(str) ? reader_ : 0;
}
/////////////////////////////////////////////////////////////////////////////////////////
// DimacsReader
/////////////////////////////////////////////////////////////////////////////////////////
DimacsReader::DimacsReader(SatBuilder& prg) : program_(&prg) {}

// Parses the p line: p [w]cnf #vars #clauses [max clause weight]
bool DimacsReader::doAttach(bool& inc) {
	inc = false;
	if (!accept(peek(false))) { return false; }
	skipLines('c');
	require(match("p "), "missing problem line");
	wcnf_        = match("w");
	require(match("cnf ", false), "unrecognized format, [w]cnf expected");
	numVar_      = matchPos(ProgramParser::VAR_MAX, "#vars expected");
	uint32 numC  = matchPos("#clauses expected");
	wsum_t cw    = 0;
	while (stream()->peek() == ' ')  { stream()->get(); };
	if (wcnf_ && peek(false) != '\n'){ stream()->match(cw); }
	while (stream()->peek() == ' ')  { stream()->get(); };
	require(stream()->get() == '\n', "invalid extra characters in problem line");
	program_->prepareProblem(numVar_, cw, numC);
	if (options.isEnabled(ParserOptions::parse_acyc_edge | ParserOptions::parse_minimize)) {
		const bool acyc = options.isEnabled(ParserOptions::parse_acyc_edge);
		const bool minw = options.isEnabled(ParserOptions::parse_minimize);
		const bool proj = options.isEnabled(ParserOptions::parse_project);
		for (ExtDepGraph* g = 0; match("c "); ) {
			if (acyc && match("graph ")) {
				require(g == 0, "graph: only one graph supported");
				g = program_->ctx()->extGraph.get();
				if (!g) { program_->ctx()->extGraph = (g = new ExtDepGraph()); }
				else    { g->update(); }
				parseGraph("c ", *g);
				g->finalize(*program_->ctx());
			}
			else if (minw && match("minweight")) {
				WeightLitVec min;
				for (int lit, weight, max = static_cast<int>(numVar_); (lit = matchInt(-max, max)) != 0; ) {
					weight = matchInt(CLASP_WEIGHT_T_MIN, CLASP_WEIGHT_T_MAX, "minweight: weight expected");
					min.push_back(WeightLiteral(toLit(lit), weight));
				}
				program_->addObjective(min);
			}
			else if (proj && match("project ")) {
				for (Var v; (v = matchPos("project: variable or 0 expected")) != 0;) {
					program_->addProject(v);
				}
			}
			else { skipLine(); }
		}
	}
	return true;
}
bool DimacsReader::doParse() {
	LitVec cc;
	const bool wcnf = wcnf_;
	wsum_t     cw   = 0;
	const int  maxV = static_cast<int>(numVar_ + 1);
	while (skipLines('c') && peek(true)) {
		cc.clear();
		if (wcnf) { require(stream()->match(cw) && cw > 0, "wcnf: positive clause weight expected"); }
		for (int lit; (lit = matchInt(-maxV, maxV, "invalid variable in clause")) != 0;) {
			cc.push_back(toLit(lit));
		}
		program_->addClause(cc, cw);
	}
	require(!more(), "unrecognized format");
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
// OpbReader
/////////////////////////////////////////////////////////////////////////////////////////
OpbReader::OpbReader(PBBuilder& prg) : program_(&prg) {}

// * #variable= int #constraint= int [#product= int sizeproduct= int] [#soft= int mincost= int maxcost= int sumcost= int]
// where [] indicate optional parts, i.e.
//  LIN-PBO: * #variable= int #constraint= int
//  NLC-PBO: * #variable= int #constraint= int #product= int sizeproduct= int
//  LIN-WBO: * #variable= int #constraint= int #soft= int mincost= int maxcost= int sumcost= int
//  NLC-WBO: * #variable= int #constraint= int #product= int sizeproduct= int #soft= int mincost= int maxcost= int sumcost= int
bool OpbReader::doAttach(bool& inc) {
	inc = false;
	if (!accept(peek(false))) { return false; }
	require(match("* #variable="), "missing problem line '* #variable='");
	unsigned numV = matchPos(ProgramParser::VAR_MAX, "number of vars expected");
	require(match("#constraint="), "bad problem line: missing '#constraint='");
	unsigned numC = matchPos("number of constraints expected");
	unsigned numProd = 0, sizeProd = 0, numSoft = 0;
	minCost_ = 0, maxCost_ = 0;
	if (match("#product=")) { // NLC instance
		numProd = matchPos();
		require(match("sizeproduct="), "'sizeproduct=' expected");
		sizeProd= matchPos();
		(void)sizeProd;
	}
	if (match("#soft=")) { // WBO instance
		numSoft = matchPos();
		require(match("mincost="), "'mincost=' expected");
		minCost_= (weight_t)matchPos(CLASP_WEIGHT_T_MAX, "invalid min costs");
		require(match("maxcost="), "'maxcost=' expected");
		maxCost_= (weight_t)matchPos(CLASP_WEIGHT_T_MAX, "invalid max costs");
		require(match("sumcost="), "'sumcost=' expected");
		wsum_t sum;
		require(stream()->match(sum) && sum > 0, "positive integer expected");
	}
	program_->prepareProblem(numV, numProd, numSoft, numC);
	return true;
}
bool OpbReader::doParse() {
	if (options.isEnabled(ParserOptions::parse_acyc_edge | ParserOptions::parse_project)) {
		const bool graph = options.isEnabled(ParserOptions::parse_acyc_edge);
		const bool proj  = options.isEnabled(ParserOptions::parse_project);
		for (ExtDepGraph* g = 0; match("*"); ) {
			if (graph && match("graph ")) {
				require(g == 0, "graph: only one graph supported");
				g = program_->ctx()->extGraph.get();
				if (!g) { program_->ctx()->extGraph = (g = new ExtDepGraph()); }
				else    { g->update(); }
				parseGraph("* ", *g);
				g->finalize(*program_->ctx());
			}
			else if (proj && match("project ")) {
				while (match("x")) { // <atom>*
					Var var = matchAtom();
					require(var <= program_->numVars() + 1, "identifier out of range");
					program_->addProject(var);
				}
			}
			else { skipLine(); }
		}
	}
	skipLines('*');
	parseOptObjective();
	for (;;) {
		skipLines('*');
		if (!more()) { return true; }
		parseConstraint();
	}
}
// <objective>::= "min:" <zeroOrMoreSpace> <sum>  ";"
// OR
// <softobj>  ::= "soft:" [<unsigned_integer>] ";"
void OpbReader::parseOptObjective() {
	if (match("min:")) {
		parseSum();
		program_->addObjective(active_.lits); 
	}
	else if (match("soft:")) {
		wsum_t softCost;
		require(stream()->match(softCost) && softCost > 0, "positive integer expected");
		require(match(";"), "semicolon missing after constraint");
		program_->setSoftBound(softCost);
	}
}

// <constraint>::= <sum> <relational_operator> <zeroOrMoreSpace> <integer> <zeroOrMoreSpace> ";"
// OR
// <softconstr>::= "[" <zeroOrMoreSpace> <unsigned_integer> <zeroOrMoreSpace> "]" <constraint>
void OpbReader::parseConstraint() {
	weight_t cost = 0;
	if (match("[")) {
		cost = matchInt(minCost_, maxCost_, "invalid soft constraint cost");
		require(match("]"), "invalid soft constraint");
	}
	parseSum();
	active_.eq = match("=");
	require(active_.eq || match(">=", false), "relational operator expected");
	active_.bound = matchInt(CLASP_WEIGHT_T_MIN, CLASP_WEIGHT_T_MAX, "invalid coefficient on rhs of constraint");
	require(match(";"), "semicolon missing after constraint");
	program_->addConstraint(active_.lits, active_.bound, active_.eq, cost);
}

// <sum>::= <weightedterm> | <weightedterm> <sum>
// <weightedterm>::= <integer> <oneOrMoreSpace> <term> <oneOrMoreSpace>
void OpbReader::parseSum() {
	active_.lits.clear();
	while (!match(";")) {
		int coeff = matchInt(INT_MIN+1, INT_MAX, "coefficient expected");
		parseTerm();
		Literal x = active_.term.size() == 1 ? active_.term[0] : program_->addProduct(active_.term);
		active_.lits.push_back(WeightLiteral(x, coeff));
		char p = peek(true);
		if (p == '>' || p == '=') break;
	}
}
// <term>::=<variablename>
// OR
// <term>::= <literal> | <literal> <space>+ <term>
void OpbReader::parseTerm() {
	active_.term.clear();
	char peek;
	do  {
		match("*");             // optionally
		bool sign = match("~"); // optionally
		require(match("x"), "identifier expected");
		Var var   = matchAtom();
		require(var <= program_->numVars() + 1, "identifier out of range");
		active_.term.push_back(Literal(var, sign));
		peek = this->peek(true);
	} while (peek == '*' || peek == '~' || peek == 'x');
}

}
