// 
// Copyright (c) 2009-2016, Benjamin Kaufmann
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
#include <clasp/cli/clasp_output.h>
#include <clasp/solver.h>
#include <clasp/satelite.h>
#include <clasp/minimize_constraint.h>
#include <clasp/util/timer.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <climits>
#include <string>
#include <cstdlib>
#if !defined(_WIN32)
#include <signal.h>
#elif !defined(SIGALRM)
#define SIGALRM 14
#endif
#ifdef _MSC_VER
#include <float.h>
#define CLASP_ISNAN(x) (_isnan(x) != 0)
#pragma warning (disable : 4996)
#elif defined(__cplusplus) && __cplusplus >= 201103L
#include <cmath>
#define CLASP_ISNAN(x) std::isnan(x)
#else
#include <math.h>
#define CLASP_ISNAN(x) isnan(x)
#endif
inline bool isNan(double d) { return CLASP_ISNAN(d); }
#undef CLASP_ISNAN
namespace Clasp { namespace Cli {
/////////////////////////////////////////////////////////////////////////////////////////
// Event formatting
/////////////////////////////////////////////////////////////////////////////////////////
static void null_term_copy(const char* in, int inSize, char* buf, uint32 bufSize) {
	if (!in || !buf || !bufSize) return;
	uint32 n = inSize < 0 ? static_cast<uint32>(0) : std::min(bufSize-1, static_cast<uint32>(inSize));
	std::memcpy(buf, in, n);
	buf[n] = 0;
}
void format(const Clasp::BasicSolveEvent& ev, char* out, uint32 outSize) {
	char buf[1024]; int n;
	const Solver& s = *ev.solver;
	n = sprintf(buf, "%2u:%c|%7u/%-7u|%8u/%-8u|%10" PRIu64"/%-6.3f|%8" PRId64"/%-10" PRId64"|"
		, s.id()
		, static_cast<char>(ev.op)
		, s.numFreeVars()
		, (s.decisionLevel() > 0 ? s.levelStart(1) : s.numAssignedVars())
		, s.numConstraints()
		, s.numLearntConstraints()
		, s.stats.conflicts
		, s.stats.conflicts/std::max(1.0,double(s.stats.choices))
		, ev.cLimit <= (UINT32_MAX) ? (int64)ev.cLimit:-1
		, ev.lLimit != (UINT32_MAX) ? (int64)ev.lLimit:-1
	);
	null_term_copy(buf, n, out, outSize);
}
void format(const Clasp::SolveTestEvent&  ev, char* out, uint32 outSize) {
	char buf[1024]; int n;
	char ct = ev.partial ? 'P' : 'F';
	if (ev.result == -1) { n = sprintf(buf, "%2u:%c| HC: %-5u %-60s|", ev.solver->id(), ct, ev.scc, "..."); }
	else                 {
		n = sprintf(buf, "%2u:%c| HC: %-5u %-4s|%8u/%-8u|%10" PRIu64"/%-6.3f| T: %-15.3f|", ev.solver->id(), ct, ev.scc, (ev.result == 1 ? "OK" : "FAIL")
		  , ev.solver->numConstraints()
		  , ev.solver->numLearntConstraints()
		  , ev.conflicts()
		  , ev.conflicts()/std::max(1.0,double(ev.choices()))
		  , ev.time
		);
	}
	null_term_copy(buf, n, out, outSize);
}
#if WITH_THREADS
void format(const Clasp::mt::MessageEvent& ev, char* out, uint32 outSize) {
	typedef Clasp::mt::MessageEvent EV;
	char buf[1024]; int n;
	if (ev.op != EV::completed) { n = sprintf(buf, "%2u:X| %-15s %-53s |", ev.solver->id(), ev.msg, ev.op == EV::sent ? "sent" : "received"); }
	else                        { n = sprintf(buf, "%2u:X| %-15s %-35s in %13.3fs |", ev.solver->id(), ev.msg, "completed", ev.time); }
	null_term_copy(buf, n, out, outSize);
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////
// Output
/////////////////////////////////////////////////////////////////////////////////////////
Output::Output(uint32 verb) : summary_(0), verbose_(0) {
	std::memset(quiet_, 0, sizeof(quiet_));
	setCallQuiet(print_no);
	setVerbosity(verb);
}
Output::~Output() {}
void Output::setVerbosity(uint32 verb) {
	Event::Verbosity x = (Event::Verbosity)std::min(verbose_ = verb, (uint32)Event::verbosity_max);
	EventHandler::setVerbosity(Event::subsystem_facade , x);
	EventHandler::setVerbosity(Event::subsystem_load   , x);
	EventHandler::setVerbosity(Event::subsystem_prepare, x);
	EventHandler::setVerbosity(Event::subsystem_solve  , x);
}
void Output::setModelQuiet(PrintLevel model){ quiet_[0] = static_cast<uint8>(model); }
void Output::setOptQuiet(PrintLevel opt)    { quiet_[1] = static_cast<uint8>(opt);   }
void Output::setCallQuiet(PrintLevel call)  { quiet_[2] = static_cast<uint8>(call);  }

void Output::saveModel(const Model& m) {
	saved_ = m;
	const SumVec*  costs = m.costs ? &(costs_ = *m.costs) : 0;
	const ValueVec* vals = &(vals_ = *m.values);
	saved_.values = vals;
	saved_.costs  = costs;
}

void Output::onEvent(const Event& ev) {
	typedef ClaspFacade::StepStart StepStart;
	typedef ClaspFacade::StepReady StepReady;
	if (const StepStart* start = event_cast<StepStart>(ev)) {
		startStep(*start->facade);
	}
	else if (const StepReady* ready = event_cast<StepReady>(ev)) {
		stopStep(*ready->summary);
	}
}
bool Output::onModel(const Solver& s, const Model& m) {
	if (modelQ() == print_all || optQ() == print_all) {
		printModel(s.outputTable(), m, print_all);
	}
	if (modelQ() == print_best || optQ() == print_best) {
		if (m.opt == 1 && !m.consequences()) { printModel(s.outputTable(), m, print_best); clearModel(); }
		else                                 { saveModel(m); }
	}
	return true;
}
void Output::startStep(const ClaspFacade&) { clearModel(); summary_ = 0; }
void Output::stopStep(const ClaspFacade::Summary& s){
	if (getModel()) {
		if (s.model()){
			saved_.opt = s.model()->opt;
			saved_.def = s.model()->def;
		}
		printModel(s.ctx().output, *getModel(), print_best);
		clearModel();
	}
	if (callQ() == print_all) { 
		printSummary(s, false);
		if (s.stats()) { printStatistics(s, false); }
	}
	else if (callQ() == print_best) {
		summary_ = &s;
	}
}
void Output::shutdown(const ClaspFacade::Summary& summary) {
	if (summary_) {
		printSummary(*summary_, false);
		if (summary_->stats()) { printStatistics(*summary_, false); }
	}
	printSummary(summary, true);
	if (summary.stats()) { printStatistics(summary, true); }
	shutdown();
}
// Returns the number of consequences and estimates in m.
// For a model m with m.consequences and a return value ret:
//   - ret.first  is the number of definite consequences
//   - ret.second is the number of remaining possible consequences
Output::UPair Output::numCons(const OutputTable& out, const Model& m) const {
	uint32 low = 0, up = 0;
	if (out.projectMode() == OutputTable::project_output) {
		low += out.numFacts();
		for (OutputTable::pred_iterator it = out.pred_begin(); it != out.pred_end(); ++it) {
			low += m.isDef(it->cond);
			up  += m.isEst(it->cond);
		}
		for (OutputTable::range_iterator it = out.vars_begin(), end = out.vars_end(); it != end; ++it) {
			Literal p = posLit(*it);
			low += m.isDef(p);
			up  += m.isEst(p);
		}
	}
	else {
		for (OutputTable::lit_iterator it = out.proj_begin(), end = out.proj_end(); it != end; ++it) {
			low += m.isDef(*it);
			up  += m.isEst(*it);
		}
	}
	return UPair(low, m.def ? 0 : up);
}

// Prints shown symbols in model.
// The functions prints
// - true literals in definite answer, followed by 
// - true literals in current estimate if m.consequences()
void Output::printWitness(const OutputTable& out, const Model& m, uintp data) {
	for (OutputTable::fact_iterator it = out.fact_begin(); it != out.fact_end(); ++it) {
		data = doPrint(OutPair(*it, lit_true()), data);
	}
	for (const char* x = out.theory ? out.theory->first(m) : 0; x; x = out.theory->next()) {
		data = doPrint(OutPair(x, lit_true()), data);
	}
	const bool onlyD = m.type != Model::Cautious || m.def;
	for (bool D = true;; D = !D) {
		for (OutputTable::pred_iterator it = out.pred_begin(); it != out.pred_end(); ++it) {
			if (m.isTrue(it->cond) && (onlyD || m.isDef(it->cond) == D)) {
				data = doPrint(OutPair(it->name, lit_true()), data);
			}
		}
		if (out.vars_begin() != out.vars_end()) {
			const bool showNeg = !m.consequences();
			if (out.projectMode() == OutputTable::project_output || !out.filter("_")) {
				for (OutputTable::range_iterator it = out.vars_begin(), end = out.vars_end(); it != end; ++it) {
					Literal p = posLit(*it);
					if ((showNeg || m.isTrue(p)) && (onlyD || m.isDef(p) == D)) {
						data = doPrint(OutPair(static_cast<const char*>(0), m.isTrue(p) ? p : ~p), data);
					}
				}
			}
			else {
				for (OutputTable::lit_iterator it = out.proj_begin(), end = out.proj_end(); it != end; ++it) {
					if ((showNeg || m.isTrue(*it)) && (onlyD || m.isDef(*it) == D)) {
						data = doPrint(OutPair(static_cast<const char*>(0), m.isTrue(*it) ? *it : ~*it), data);
					}
				}
			}
		}
		if (D == onlyD) { return; }
	}
}
uintp Output::doPrint(const OutPair&, UPtr x) { return x; }
/////////////////////////////////////////////////////////////////////////////////////////
// StatsVisitor
/////////////////////////////////////////////////////////////////////////////////////////
StatsVisitor::~StatsVisitor() {}
void StatsVisitor::visitStats(const ClaspFacade::Summary& summary) {
	const SharedContext& ctx = summary.ctx();
	visitSolverStats(summary.solvers(), true);
	visitProblemStats(ctx.stats(), summary.lpStats());
	if (summary.stats() > 1 && summary.numSolver() > 1) {
		visitThreads(summary);
	}
	if (summary.numHcc() != 0) { visitHccs(summary); }
}
void StatsVisitor::visitProblemStats(const Clasp::ProblemStats& stats, const Clasp::Asp::LpStats* lp) {
	if (lp) { visitLogicProgramStats(*lp); }
	visitProblemStats(stats);
}

void StatsVisitor::visitSolverStats(const Clasp::SolverStats& stats, bool accu) {
	ExtendedStats e;
	const ExtendedStats* ext = stats.extra ? stats.extra : &e;
	visitCoreSolverStats(ext->cpuTime, ext->models, stats, accu);
	if (stats.extra) { visitExtSolverStats(*stats.extra, accu); }
	if (stats.jumps) { visitJumpStats(*stats.jumps, accu); }
}

void StatsVisitor::visitThreads(const ClaspFacade::Summary& summary) {
	for (uint32 i = 0, end = summary.numSolver(); i != end; ++i) {
		visitThread(i, summary.solver(i));
	}
}
void StatsVisitor::visitHccs(const ClaspFacade::Summary& summary) {
	visitProblemStats(*summary.hccs().first, 0);
	visitSolverStats(*summary.hccs().second, false);
	if (summary.stats() > 1) {
		for (uint32 i = 0, end = summary.numHcc(); i != end; ++i) {
			const ClaspFacade::Summary::HccPair& p = summary.hcc(i);
			visitHcc(i, *p.first, *p.second);
		}
	}
}
void StatsVisitor::visitHcc(uint32, const ProblemStats& p, const SolverStats& s) {
	visitProblemStats(p, 0);
	visitSolverStats(s, false);
}
/////////////////////////////////////////////////////////////////////////////////////////
// JsonOutput
/////////////////////////////////////////////////////////////////////////////////////////
JsonOutput::JsonOutput(uint32 v) : Output(std::min(v, uint32(1))), open_("") {
	objStack_.reserve(10);
}
JsonOutput::~JsonOutput() { JsonOutput::shutdown(); }
void JsonOutput::run(const char* solver, const char* version, const std::string* iBeg, const std::string* iEnd) {
	if (indent() == 0) {
		open_ = "";
		JsonOutput::pushObject();
	}
	printKeyValue("Solver", std::string(solver).append(" version ").append(version).c_str());
	pushObject("Input", type_array);
	printf("%-*s", indent(), " ");
	for (const char* sep = ""; iBeg != iEnd; ++iBeg, sep = ",") {
		printString(iBeg->c_str(), sep);
	}
	popObject();
	pushObject("Call", type_array);
}
void JsonOutput::shutdown(const ClaspFacade::Summary& summary) {
	while (!objStack_.empty() && *objStack_.rbegin() == '[') {
		popObject();
	}
	Output::shutdown(summary);
}

void JsonOutput::shutdown() {
	if (!objStack_.empty()) {
		do { popObject(); } while (!objStack_.empty());
		printf("\n");
	}
	fflush(stdout);
}
void JsonOutput::startStep(const ClaspFacade& f) {
	Output::startStep(f);
	pushObject(0, type_object);
}
void JsonOutput::stopStep(const ClaspFacade::Summary& s) {
	assert(!objStack_.empty());
	Output::stopStep(s);
	while (popObject() != '{') { ; }
}

void JsonOutput::visitCoreSolverStats(double cpuTime, uint64 models, const SolverStats& st, bool accu) {
	pushObject("Core");
	if (!accu) { printKeyValue("CPU", cpuTime); }
	printKeyValue("Models"     , models);
	printKeyValue("Choices"    , st.choices);
	printKeyValue("Conflicts"  , st.conflicts);
	printKeyValue("Backtracks" , st.backtracks());
	printKeyValue("Backjumps"  , st.backjumps());
	printKeyValue("Restarts"   , st.restarts);
	printKeyValue("RestartAvg" , st.avgRestart());
	printKeyValue("RestartLast", st.lastRestart);
	popObject(); // Core
}

void JsonOutput::visitExtSolverStats(const ExtendedStats& stx, bool accu) {
	pushObject("More");
	if (stx.domChoices) {
		printKeyValue("DomChoices", stx.domChoices);
	}
	if (stx.hccTests) {
		pushObject("StabTests");
		printKeyValue("Sum", stx.hccTests);
		printKeyValue("Full", stx.hccTests - stx.hccPartial);
		printKeyValue("Partial", stx.hccPartial);
		popObject();
	}
	if (stx.models) {
		printKeyValue("AvgModel", stx.avgModel());
	}
	printKeyValue("Splits", stx.splits);
	printKeyValue("Problems", stx.gps);
	printKeyValue("AvgGPLength", stx.avgGp());
	pushObject("Lemma");
	printKeyValue("Sum", stx.lemmas());
	printKeyValue("Deleted" , stx.deleted);
	pushObject("Type", type_array);
	const char* names[] = {"Short", "Conflict", "Loop", "Other"};
	for (int i = Constraint_t::Static; i <= Constraint_t::Other; ++i) {
		pushObject();
		printKeyValue("Type", names[i]);
		if (i == 0) {
			printKeyValue("Sum", stx.binary+stx.ternary);
			printKeyValue("Ratio", percent(stx.binary+stx.ternary, stx.lemmas()));
			printKeyValue("Binary", stx.binary);
			printKeyValue("Ternary", stx.ternary);
		}
		else {
			printKeyValue("Sum", stx.lemmas(ConstraintType(i)));
			printKeyValue("AvgLen", stx.avgLen(ConstraintType(i)));
		}
		popObject();
	}
	popObject();
	popObject(); // Lemma
	if (stx.distributed || stx.integrated) {
		pushObject("Distribution");
		printKeyValue("Distributed", stx.distributed);
		printKeyValue("Ratio", stx.distRatio());
		printKeyValue("AvgLbd", stx.avgDistLbd());
		popObject();
		pushObject("Integration");
		printKeyValue("Integrated", stx.integrated);
		printKeyValue("Units", stx.intImps);
		printKeyValue("AvgJump", stx.avgIntJump());
		if (accu) { printKeyValue("Ratio", stx.intRatio()); }
		popObject();
	}
	popObject(); // More
}
void JsonOutput::visitJumpStats(const JumpStats& st, bool) {
	pushObject("Jumps");
	printKeyValue("Sum", st.jumps);
	printKeyValue("Max", st.maxJump);
	printKeyValue("MaxExec", st.maxJumpEx);
	printKeyValue("Avg", st.avgJump());
	printKeyValue("AvgExec", st.avgJumpEx());
	printKeyValue("Levels", st.jumpSum);
	printKeyValue("LevelsExec", st.jumped());
	pushObject("Bounded");
	printKeyValue("Sum", st.bounded);
	printKeyValue("Max", st.maxBound);
	printKeyValue("Avg", st.avgBound());
	printKeyValue("Levels", st.boundSum);
	popObject();
	popObject();
}
void JsonOutput::visitLogicProgramStats(const Asp::LpStats& lp) {
	using namespace Asp;
	pushObject("LP");
	pushObject("Rules");
	printKeyValue("Original", lp.rules[0].sum());
	printKeyValue("Final",    lp.rules[1].sum());
	for (uint32 i = 0; i != RuleStats::numKeys(); ++i) {
		if (i != RuleStats::Normal && lp.rules[0][i]) {
			pushObject(RuleStats::toStr(i));
			printKeyValue("Original", lp.rules[0][i]);
			printKeyValue("Final",    lp.rules[1][i]);
			popObject();
		}
	}
	popObject(); // Rules
	printKeyValue("Atoms", lp.atoms);
	if (lp.auxAtoms) { printKeyValue("AuxAtoms", lp.auxAtoms); }
	if (lp.disjunctions[0]) {
		pushObject("Disjunctions");
		printKeyValue("Original", lp.disjunctions[0]);
		printKeyValue("Final", lp.disjunctions[1]);
		popObject();
	}
	pushObject("Bodies");
	printKeyValue("Original", lp.bodies[0].sum());
	printKeyValue("Final"   , lp.bodies[1].sum());
	for (uint32 i = 1; i != BodyStats::numKeys(); ++i) {
		if (lp.bodies[0][i]) {
			pushObject(BodyStats::toStr(i));
			printKeyValue("Original", lp.bodies[0][i]);
			printKeyValue("Final",    lp.bodies[1][i]);
			popObject();
		}
	}
	popObject();
	if      (lp.sccs == 0)              { printKeyValue("Tight", "yes"); }
	else if (lp.sccs == PrgNode::noScc) { printKeyValue("Tight", "N/A"); }
	else                                {
		printKeyValue("Tight", "no");
		printKeyValue("SCCs", lp.sccs);
		printKeyValue("NonHcfs", lp.nonHcfs);
		printKeyValue("UfsNodes", lp.ufsNodes);
		printKeyValue("NonHcfGammas", lp.gammas);
	}
	pushObject("Equivalences");
	printKeyValue("Sum", lp.eqs());
	printKeyValue("Atom", lp.eqs(Var_t::Atom));
	printKeyValue("Body", lp.eqs(Var_t::Body));
	printKeyValue("Other", lp.eqs(Var_t::Hybrid));
	popObject();
	popObject(); // LP
}
void JsonOutput::visitProblemStats(const ProblemStats& p) {
	pushObject("Problem");
	printKeyValue("Variables", p.vars.num);
	printKeyValue("Eliminated", p.vars.eliminated);
	printKeyValue("Frozen", p.vars.frozen);
	pushObject("Constraints");
	uint32 sum = p.numConstraints();
	printKeyValue("Sum", sum);
	printKeyValue("Binary", p.constraints.binary);
	printKeyValue("Ternary", p.constraints.ternary);
	popObject(); // Constraints
	printKeyValue("AcycEdges", p.acycEdges);
	popObject(); // PS
}

void JsonOutput::printKey(const char* k) {
	printf("%s%-*s\"%s\": ", open_, indent(), " ", k);
	open_ = ",\n";
}

void JsonOutput::printString(const char* v, const char* sep) {
	assert(v);
	const uint32 BUF_SIZE = 1024;
	char buf[BUF_SIZE];
	uint32 n = 0;
	buf[n++] = '"';
	while (*v) {
		if      (*v != '\\' && *v != '"')                       { buf[n++] = *v++; }
		else if (*v == '"' || !strchr("\"\\/\b\f\n\r\t", v[1])) { buf[n++] = '\\'; buf[n++] = *v++; }
		else                                                    { buf[n++] = v[0]; buf[n++] = v[1]; v += 2; }
		if (n > BUF_SIZE - 2) { buf[n] = 0; printf("%s%s", sep, buf); n = 0; sep = ""; }
	}
	buf[n] = 0;
	printf("%s%s\"", sep, buf);
}

void JsonOutput::printKeyValue(const char* k, const char* v) {
	printf("%s%-*s\"%s\": ", open_, indent(), " ", k);
	printString(v,"");
	open_ = ",\n";
}
void JsonOutput::printKeyValue(const char* k, uint64 v) {
	printf("%s%-*s\"%s\": %" PRIu64, open_, indent(), " ", k, v);
	open_ = ",\n";
}
void JsonOutput::printKeyValue(const char* k, uint32 v) { return printKeyValue(k, uint64(v)); }
void JsonOutput::printKeyValue(const char* k, double v) {
	if (!isNan(v)) { printf("%s%-*s\"%s\": %.3f", open_, indent(), " ", k, v); } 
	else           { printf("%s%-*s\"%s\": %s", open_, indent(), " ", k, "null"); }
	open_ = ",\n";
}

void JsonOutput::pushObject(const char* k, ObjType t) {
	if (k) {
		printKey(k);
	}
	else {
		printf("%s%-*.*s", open_, indent(), indent(), " ");
	}
	char o = t == type_object ? '{' : '[';
	objStack_ += o;
	printf("%c\n", o);
	open_ = "";
}
char JsonOutput::popObject() {
	assert(!objStack_.empty());
	char o = *objStack_.rbegin();
	objStack_.erase(objStack_.size()-1);
	printf("\n%-*.*s%c", indent(), indent(), " ", o == '{' ? '}' : ']');
	open_ = ",\n";
	return o;
}
void JsonOutput::startModel() {
	if (!hasWitness()) {
		pushObject("Witnesses", type_array);
	}
	pushObject();
}
uintp JsonOutput::doPrint(const OutPair& out, uintp data) {
	const char* sep = reinterpret_cast<const char*>(data);
	if (out.first){ printString(out.first, sep); }
	else          { printf("%s%d", sep, out.second.sign() ? -static_cast<int>(out.second.var()) : static_cast<int>(out.second.var())); }
	return reinterpret_cast<UPtr>(", ");
}
void JsonOutput::printModel(const OutputTable& out, const Model& m, PrintLevel x) {
	uint32 hasModel = (x == modelQ());
	if (hasModel) {
		startModel();
		pushObject("Value", type_array);
		printf("%-*s", indent(), " ");
		printWitness(out, m, reinterpret_cast<UPtr>(""));
		popObject();
		if (m.consequences() && x == optQ()) {
			printCons(numCons(out, m));
		}
	}
	if (x == optQ()) {
		if (m.consequences() && !hasModel++) {
			startModel();
			printCons(numCons(out, m));
		}
		if (m.costs) {
			if (!hasModel++) { startModel(); }
			printCosts(*m.costs);
		}
	}
	if (hasModel) { popObject(); }
}
void JsonOutput::printCosts(const SumVec& opt, const char* name) { 
	pushObject(name, type_array);
	printf("%-*s", indent(), " ");
	const char* sep = "";
	for (SumVec::const_iterator it = opt.begin(), end = opt.end(); it != end; ++it) {
		printf("%s%" PRId64, sep, *it);
		sep = ", ";
	}
	popObject();
}
void JsonOutput::printCons(const UPair& cons) {
	pushObject("Consequences");
	printKeyValue("True", cons.first);
	printKeyValue("Open", cons.second);
	popObject();
}

void JsonOutput::printSummary(const ClaspFacade::Summary& run, bool final) {
	if (hasWitness()) { popObject(); }
	const char* res = "UNKNOWN";
	if      (run.unsat()) { res = "UNSATISFIABLE"; }
	else if (run.sat())   { res = !run.optimum() ? "SATISFIABLE" : "OPTIMUM FOUND"; }
	printKeyValue("Result", res);
	if (verbosity()) {
		if (run.result.interrupted()){ printKeyValue(run.result.signal != SIGALRM ? "INTERRUPTED" : "TIME LIMIT", uint32(1));  }
		pushObject("Models");
		printKeyValue("Number", run.enumerated());
		printKeyValue("More"  , run.complete() ? "no" : "yes");
		if (run.sat()) {
			if (run.consequences()){ 
				printKeyValue(run.consequences(), run.complete() ? "yes":"unknown");
				printCons(numCons(run.ctx().output, *run.model()));
			}
			if (run.optimize())    { 
				printKeyValue("Optimum", run.optimum()?"yes":"unknown"); 
				printKeyValue("Optimal", run.optimal());
				printCosts(*run.costs());
			}
		}
		popObject();
		if (run.hasLower() && !run.optimum()) {
			pushObject("Bounds");
			printCosts(run.lower(), "Lower");
			printCosts(run.costs() ? *run.costs() : SumVec(), "Upper");
			popObject();
		}
		if (final) { printKeyValue("Calls", run.step + 1); }
		pushObject("Time");
		printKeyValue("Total", run.totalTime);
		printKeyValue("Solve", run.solveTime);
		printKeyValue("Model", run.satTime);
		printKeyValue("Unsat", run.unsatTime);
		printKeyValue("CPU"  , run.cpuTime);
		popObject(); // Time
		if (run.ctx().concurrency() > 1) {
			printKeyValue("Threads", run.ctx().concurrency());
			printKeyValue("Winner",  run.ctx().winner());
		}
	}
}
void JsonOutput::printStatistics(const ClaspFacade::Summary& summary, bool) {
	if (hasWitness()) { popObject(); }
	pushObject("Stats", type_object); 
	StatsVisitor::visitStats(summary);
	popObject();
}
/////////////////////////////////////////////////////////////////////////////////////////
// TextOutput
/////////////////////////////////////////////////////////////////////////////////////////
#define printKeyValue(k, fmt, value) printf("%s%-*s: " fmt, format[cat_comment], width_, (k), (value))
#define printLN(cat, fmt, ...)       printf("%s" fmt "\n", format[cat], __VA_ARGS__)
#define printBR(cat)                 printf("%s\n", format[cat])
#define printKey(k)                  printf("%s%-*s: ", format[cat_comment], width_, (k))
const char* const rowSep   = "----------------------------------------------------------------------------|";
const char* const finalSep = "=============================== Accumulation ===============================|";

static inline std::string prettify(const std::string& str) {
	if (str.size() < 40) return str;
	std::string t("...");
	t.append(str.end()-38, str.end());
	return t;
}
TextOutput::TextOutput(uint32 verbosity, Format f, const char* catAtom, char ifs) : Output(verbosity), stTime_(0.0), state_(0) {
	result[res_unknonw]    = "UNKNOWN";
	result[res_sat]        = "SATISFIABLE";
	result[res_unsat]      = "UNSATISFIABLE";
	result[res_opt]        = "OPTIMUM FOUND";
	format[cat_comment]    = "";
	format[cat_value]      = "";
	format[cat_objective]  = "Optimization: ";
	format[cat_result]     = "";
	format[cat_value_term] = "";
	format[cat_atom]       = "%s";
	if (f == format_aspcomp) {
		format[cat_comment]   = "% ";
		format[cat_value]     = "ANSWER\n";
		format[cat_objective] = "COST ";
		format[cat_atom]      = "%s.";
		result[res_sat]       = "";
		result[res_unsat]     = "INCONSISTENT";
		result[res_opt]       = "OPTIMUM";
		setModelQuiet(print_best);
		setOptQuiet(print_best);
	}
	else if (f == format_sat09 || f == format_pb09) {
		format[cat_comment]   = "c ";
		format[cat_value]     = "v ";
		format[cat_objective] = "o ";
		format[cat_result]    = "s ";
		format[cat_value_term]= "0";
		format[cat_atom]      = "-%d";
		if (f == format_pb09) {
			format[cat_value_term]= "";
			format[cat_atom]      = "-x%d";
			setModelQuiet(print_best);
		}
	}
	if (catAtom && *catAtom) {
		format[cat_atom] = catAtom;
		char ca = f == format_sat09 || f == format_pb09 ? 'd' : 's';
		while (*catAtom) {
			if      (*catAtom == '\n'){ break; }
			else if (*catAtom == '%') {
				if      (!*++catAtom)    { ca = '%'; break; }
				else if (*catAtom == ca) { ca = 0; }
				else if (*catAtom != '%'){ break; }
			}
			++catAtom;
		}
		CLASP_FAIL_IF(ca != 0         , "cat_atom: Invalid format string - format '%%%c' expected!", ca);
		CLASP_FAIL_IF(*catAtom == '\n', "cat_atom: Invalid format string - new line not allowed!");
		CLASP_FAIL_IF(*catAtom != 0   , "cat_atom: Invalid format string - '%%%c' too many arguments!", *catAtom);
	}
	CLASP_FAIL_IF((f == format_sat09 || f == format_pb09) && *format[cat_atom] != '-' , "cat_atom: Invalid format string - must start with '-'!");
	ifs_[0] = ifs;
	ifs_[1] = 0;
	width_  = 13+(int)strlen(format[cat_comment]);
	ev_     = -1;
}
TextOutput::~TextOutput() {}

void TextOutput::comment(uint32 v, const char* fmt, ...) const {
	if (verbosity() >= v) {
		printf("%s", format[cat_comment]);
		va_list args;
		va_start(args, fmt);
		vfprintf(stdout, fmt, args);
		va_end(args);
		fflush(stdout);
	}
}	

void TextOutput::run(const char* solver, const char* version, const std::string* begInput, const std::string* endInput) {
	if (!version) version = "";
	if (solver) comment(1, "%s version %s\n", solver, version);
	if (begInput != endInput) {
		comment(1, "Reading from %s%s\n", prettify(*begInput).c_str(), (endInput - begInput) > 1 ? " ..." : "");
	}
}
void TextOutput::shutdown() {}
void TextOutput::printSummary(const ClaspFacade::Summary& run, bool final) {
	if (final && callQ() != print_no){ 
		comment(1, "%s\n", finalSep);
	}
	const char* res = result[res_unknonw];
	if      (run.unsat()) { res = result[res_unsat]; }
	else if (run.sat())   { res = !run.optimum() ? result[res_sat] : result[res_opt]; }
	if (std::strlen(res)) { printLN(cat_result, "%s", res); }
	if (verbosity() || run.stats()) {
		printBR(cat_comment);
		if (run.result.interrupted()){ printKeyValue((run.result.signal != SIGALRM ? "INTERRUPTED" : "TIME LIMIT"), "%u\n", uint32(1));  }
		const char* const moreStr = run.complete() ? "" :  "+";
		printKey("Models");
		printf("%" PRIu64 "%s\n", run.enumerated(), moreStr);
		if (run.sat()) {
			if (run.consequences()) { printLN(cat_comment, "  %-*s: %s", width_-2, run.consequences(), (run.complete()?"yes":"unknown")); }
			if (run.costs())        { printKeyValue("  Optimum", "%s\n", run.optimum()?"yes":"unknown"); }
			if (run.optimize())     {
				if (run.optimal() > 1){ printKeyValue("  Optimal", "%" PRIu64"\n", run.optimal()); }
				printKey("Optimization");
				printCosts(*run.costs());
				printf("\n");
			}
			if (run.consequences()) {
				printKey("Consequences");
				printf("%u%s\n", numCons(run.ctx().output, *run.model()).first, moreStr);
			}
		}
		if (run.hasLower() && !run.optimum()) {
			printKey("Bounds");
			printBounds(run.lower(), run.costs() ? *run.costs() : SumVec());
			printf("\n");
		}
		if (final) { printKeyValue("Calls", "%u\n", run.step + 1); }
		printKey("Time");
		printf("%.3fs (Solving: %.2fs 1st Model: %.2fs Unsat: %.2fs)\n"
			, run.totalTime
			, run.solveTime
			, run.satTime
			, run.unsatTime);
		printKeyValue("CPU Time", "%.3fs\n", run.cpuTime);
		if (run.ctx().concurrency() > 1) {
			printKeyValue("Threads", "%-8u", run.ctx().concurrency());
			printf(" (Winner: %u)\n", run.ctx().winner());
		}
	}
}
void TextOutput::printStatistics(const ClaspFacade::Summary& run, bool) {
	printBR(cat_comment);
	StatsVisitor::visitStats(run);
}
void TextOutput::startStep(const ClaspFacade& f) {
	Output::startStep(f);
	if (callQ() != print_no) { comment(1, "%s\n", rowSep); comment(2, "%-13s: %d\n", "Call", f.step()+1); }
}
void TextOutput::onEvent(const Event& ev) {
	typedef SatElite::SatElite::Progress SatPre;
	if (ev.verb <= verbosity()) {
		if      (ev.system == 0)      { setState(0,0,0); }
		else if (ev.system == state_) {
			if      (ev.system == Event::subsystem_solve)       { printSolveProgress(ev); }
			else if (const SatPre* sat = event_cast<SatPre>(ev)){
				if      (sat->op != SatElite::SatElite::Progress::event_algorithm) { comment(2, "Sat-Prepro   : %c: %8u/%-8u\r", (char)sat->op, sat->cur, sat->max); }
				else if (sat->cur!= sat->max)                                      {
					setState(0,0,0); comment(2, "Sat-Prepro   :\r");
					state_ = Event::subsystem_prepare;
				}
				else {
					SatPreprocessor* p = sat->self;
					double tEnd = RealTime::getTime();
					comment(2, "Sat-Prepro   : %.3f (ClRemoved: %u ClAdded: %u LitsStr: %u)\n", tEnd - stTime_, p->stats.clRemoved, p->stats.clAdded, p->stats.litsRemoved);
					state_ = 0;
				}
			}
		}
		else if (const LogEvent* log = event_cast<LogEvent>(ev)) { 
			setState(ev.system, ev.verb, log->msg);
		}
	}
	Output::onEvent(ev);
}

void TextOutput::setState(uint32 state, uint32 verb, const char* m) {
	if (state != state_ && verb <= verbosity()) {
		double tEnd = RealTime::getTime();
		if      (state_ == Event::subsystem_solve) { comment(2, "%s\n", rowSep); line_ = 20; }
		else if (state_ != Event::subsystem_facade){ printf("%.3f\n", tEnd - stTime_); }
		stTime_ = tEnd;
		state_  = state;
		ev_     = -1;
		if      (state_ == Event::subsystem_load)   { comment(2, "%-13s: ", m ? m : "Reading"); }
		else if (state_ == Event::subsystem_prepare){ comment(2, "%-13s: ", m ? m : "Preprocessing"); }
		else if (state_ == Event::subsystem_solve)  { comment(1, "Solving...\n"); line_ = 0; }
	}
}

void TextOutput::printSolveProgress(const Event& ev) {
	if (ev.id == SolveTestEvent::id_s  && (verbosity() & 4) == 0) { return; }
	if (ev.id == BasicSolveEvent::id_s && (verbosity() & 1) == 0) { return; }
	char lEnd = '\n';
	char line[128];
	if      (const BasicSolveEvent* be = event_cast<BasicSolveEvent>(ev)) { Clasp::Cli::format(*be, line, 128); }
	else if (const SolveTestEvent*  te = event_cast<SolveTestEvent>(ev) ) { Clasp::Cli::format(*te, line, 128); lEnd= te->result == -1 ? '\r' : '\n'; }
#if WITH_THREADS
	else if (const mt::MessageEvent*me = event_cast<mt::MessageEvent>(ev)){ Clasp::Cli::format(*me, line, 128); }
#endif
	else if (const LogEvent* log = event_cast<LogEvent>(ev))              { 
		clasp_format(line, sizeof(line), "[Solving+%.3fs]", RealTime::getTime() - stTime_);
		printLN(cat_comment, "%2u:L| %-30s %-38s |", log->solver->id(), line, log->msg);
		return;
	}
	else if (const OptBound* bnd = event_cast<OptBound>(ev))              {
		if (bnd->upper != INT64_MAX) {
			clasp_format(line, sizeof(line), "%2u:B| @Level %-6u [%17" PRId64 ";%-17" PRId64 "]%20c", bnd->solver->id(), bnd->at, bnd->lower, bnd->upper, '|');
		}
		else {
			clasp_format(line, sizeof(line), "%2u:B| @Level %-6u [%17" PRId64 ";%-17c]%20c", bnd->solver->id(), bnd->at, bnd->lower, '*', '|');
		}
	}
	else                                                                  { return; }
	bool newEvent = ((uint32)ev_.fetch_and_store(static_cast<int>(ev.id))) != ev.id;
	if ((lEnd == '\n' && --line_ == 0) || newEvent) {
		if (line_ <= 0) {
			line_ = 20;
			if ((verbosity() & 1) != 0) {
				printf("%s%s\n"
					"%sID:T       Vars           Constraints         State            Limits       |\n"
					"%s       #free/#fixed   #problem/#learnt  #conflicts/ratio #conflict/#learnt  |\n"
					"%s%s\n", format[cat_comment], rowSep, format[cat_comment], format[cat_comment], format[cat_comment], rowSep);
			}
			else {
				printf("%s%s\n"
					"%sID:T       Info                     Info                      Info          |\n"
					"%s%s\n", format[cat_comment], rowSep, format[cat_comment], format[cat_comment], rowSep);
			}
		}
		else { printLN(cat_comment, "%s", rowSep); }
	}
	printf("%s%s%c", format[cat_comment], line, lEnd);
	fflush(stdout);
}

const char* TextOutput::fieldSeparator() const { return ifs_; }
int TextOutput::printSep(CategoryKey k) const {
	return printf("%s%s", fieldSeparator(), *fieldSeparator() != '\n' ? "" : format[k]);
}
uintp TextOutput::doPrint(const OutPair& s, UPtr data) {
	uint32& accu    = reinterpret_cast<UPair*>(data)->first;
	uint32& maxLine = reinterpret_cast<UPair*>(data)->second;
	if      (accu < maxLine) { accu += printSep(cat_value); }
	else if (!maxLine)       { maxLine = s.first || *fieldSeparator() != ' ' ? UINT32_MAX : 70; }
	else                     { printf("\n%s", format[cat_value]); accu = 0; }
	if (s.first){ accu += printf(format[cat_atom], s.first); }
	else        { accu += printf(format[cat_atom] + !s.second.sign(), static_cast<int>(s.second.var())); }
	return data;
}
void TextOutput::printModel(const OutputTable& out, const Model& m, PrintLevel x) {
	if (x == modelQ()) {
		comment(1, "Answer: %" PRIu64"\n", m.num);
		printf("%s", format[cat_value]);
		UPair data;
		printWitness(out, m, reinterpret_cast<UPtr>(&data));
		if (*format[cat_value_term]) {
			printSep(cat_value);
			printf("%s", format[cat_value_term]); 
		}
		printf("\n");
	}
	if (x == optQ()) {
		if (m.consequences()) {
			UPair cons = numCons(out, m);
			printLN(cat_comment, "Consequences: [%u;%u]", cons.first, cons.first + cons.second);
		}
		if (m.costs) {
			printf("%s", format[cat_objective]);
			printCosts(*m.costs);
			printf("\n");
		}
	}
	fflush(stdout);
}

void TextOutput::printBounds(const SumVec& lower, const SumVec& upper) const {
	for (uint32 i = 0, uMax = upper.size(), lMax = lower.size(), end = std::max(uMax, lMax), seps = end; i != end; ++i) {
		if (i >= uMax) {
			printf("[%" PRId64";*]", lower[i]); 
		}
		else if (i >= lMax || lower[i] == upper[i]) {
			printf("%" PRId64, upper[i]);
		}
		else {
			printf("[%" PRId64";%" PRId64"]", lower[i], upper[i]);
		}
		if (--seps) { printSep(cat_objective); }
	}
}

void TextOutput::printCosts(const SumVec& costs) const {
	if (!costs.empty()) {
		printf("%" PRId64, costs[0]);
		for (uint32 i = 1, end = (uint32)costs.size(); i != end; ++i) {
			printSep(cat_objective);
			printf("%" PRId64, costs[i]);
		}
	}
}
void TextOutput::startSection(const char* n) const {
	printLN(cat_comment, "============ %s Stats ============", n);
	printBR(cat_comment);
}
void TextOutput::startObject(const char* n, uint32 i) const {
	printLN(cat_comment, "[%s %u]", n, i);
	printBR(cat_comment);
}
void TextOutput::visitProblemStats(const ProblemStats& stats, const Asp::LpStats* lp) {
	StatsVisitor::visitProblemStats(stats, lp);
	printBR(cat_comment);
}
void TextOutput::visitSolverStats(const Clasp::SolverStats& s, bool accu) {
	StatsVisitor::visitSolverStats(s, accu);
	printBR(cat_comment);
}
	
void TextOutput::visitLogicProgramStats(const Asp::LpStats& lp) {
	using namespace Asp;
	uint32 rFinal = lp.rules[1].sum(), rOriginal = lp.rules[0].sum();
	printKeyValue("Rules", "%-8u", rFinal);
	if (rFinal != rOriginal) {
		printf(" (Original: %u)", rOriginal);
	}
	printf("\n");
	char buf[80];
	for (uint32 i = 0; i != RuleStats::numKeys(); ++i) {
		if (i == RuleStats::Normal) { continue; }
		if (uint32 r = lp.rules[0][i]) {
			printKeyValue(clasp_format(buf, 80, "  %s", RuleStats::toStr(i)), "%-8u", lp.rules[1][i]);
			if (r != lp.rules[1][i]) { printf(" (Original: %u)", r); }
			printf("\n");
		}
	}
	printKeyValue("Atoms", "%-8u", lp.atoms);
	if (lp.auxAtoms) {
		printf(" (Original: %u Auxiliary: %u)", lp.atoms-lp.auxAtoms, lp.auxAtoms);
	}
	printf("\n");
	if (lp.disjunctions[0]) {
		printKeyValue("Disjunctions", "%-8u", lp.disjunctions[1]);
		printf(" (Original: %u)\n", lp.disjunctions[0]);
	}
	uint32 bFinal = lp.bodies[1].sum(), bOriginal = lp.bodies[0].sum();
	printKeyValue("Bodies", "%-8u", bFinal);
	if (bFinal != bOriginal) {
		printf(" (Original: %u)", bOriginal);
	}
	printf("\n");
	for (uint32 i = 1; i != BodyStats::numKeys(); ++i) {
		if (uint32 b = lp.bodies[0][i]) {
			printKeyValue(clasp_format(buf, 80, "  %s", BodyStats::toStr(i)), "%-8u", lp.bodies[1][i]);
			if (b != lp.bodies[1][i]) { printf(" (Original: %u)", b); }
			printf("\n");
		}
	}
	if (lp.eqs() > 0) {
		printKeyValue("Equivalences", "%-8u", lp.eqs());
		printf(" (Atom=Atom: %u Body=Body: %u Other: %u)\n" 
			, lp.eqs(Var_t::Atom)
			, lp.eqs(Var_t::Body)
			, lp.eqs(Var_t::Hybrid));
	}
	printKey("Tight");
	if      (lp.sccs == 0)              { printf("Yes"); }
	else if (lp.sccs != PrgNode::noScc) { printf("%-8s (SCCs: %u Non-Hcfs: %u Nodes: %u Gammas: %u)", "No", lp.sccs, lp.nonHcfs, lp.ufsNodes, lp.gammas); }
	else                                 { printf("N/A"); }
	printf("\n");
}
void TextOutput::visitProblemStats(const ProblemStats& ps) {
	uint32 sum = ps.numConstraints();
	printKeyValue("Variables", "%-8u", ps.vars.num);
	printf(" (Eliminated: %4u Frozen: %4u)\n", ps.vars.eliminated, ps.vars.frozen);
	printKeyValue("Constraints", "%-8u", sum);
	printf(" (Binary: %5.1f%% Ternary: %5.1f%% Other: %5.1f%%)\n"
		, percent(ps.constraints.binary, sum)
		, percent(ps.constraints.ternary, sum)
		, percent(ps.constraints.other, sum));
	if (ps.acycEdges) {
		printKeyValue("Acyc-Edges", "%-8u\n", ps.acycEdges);
	}
}
void TextOutput::visitCoreSolverStats(double cpuTime, uint64 models, const SolverStats& st, bool accu) {
	if (!accu) {
		printKeyValue("CPU Time",  "%.3fs\n", cpuTime);
		printKeyValue("Models", "%" PRIu64"\n", models);
	}
	printKeyValue("Choices", "%-8" PRIu64, st.choices);
	if (st.extra && st.extra->domChoices) { printf(" (Domain: %" PRIu64")", st.extra->domChoices); }
	printf("\n");
	printKeyValue("Conflicts", "%-8" PRIu64"", st.conflicts);
	printf(" (Analyzed: %" PRIu64")\n", st.backjumps());
	printKeyValue("Restarts", "%-8" PRIu64"", st.restarts);
	if (st.restarts) {
		printf(" (Average: %.2f Last: %" PRIu64")", st.avgRestart(), st.lastRestart);
	}
	printf("\n");
}
void TextOutput::visitExtSolverStats(const ExtendedStats& stx, bool accu) {
	if (stx.hccTests) {
		printKeyValue("Stab. Tests", "%-8" PRIu64, stx.hccTests);
		printf(" (Full: %" PRIu64" Partial: %" PRIu64")\n", stx.hccTests - stx.hccPartial, stx.hccPartial);
	}
	if (stx.models) {
		printKeyValue("Model-Level", "%-8.1f\n", stx.avgModel());
	}
	printKeyValue("Problems", "%-8" PRIu64,  (uint64)stx.gps);
	printf(" (Average Length: %.2f Splits: %" PRIu64")\n", stx.avgGp(), (uint64)stx.splits);
	uint64 sum = stx.lemmas();
	printKeyValue("Lemmas", "%-8" PRIu64, sum);
	printf(" (Deleted: %" PRIu64")\n",  stx.deleted);
	printKeyValue("  Binary", "%-8" PRIu64, uint64(stx.binary));
	printf(" (Ratio: %6.2f%%)\n", percent(stx.binary, sum));
	printKeyValue("  Ternary", "%-8" PRIu64, uint64(stx.ternary));
	printf(" (Ratio: %6.2f%%)\n", percent(stx.ternary, sum));
	const char* names[] = {"  Conflict", "  Loop", "  Other"};
	for (int i = 0; i != sizeof(names)/sizeof(names[0]); ++i) {
		ConstraintType type = static_cast<ConstraintType>(i+1);
		printKeyValue(names[i], "%-8" PRIu64, stx.lemmas(type));
		printf(" (Average Length: %6.1f Ratio: %6.2f%%) \n", stx.avgLen(type), percent(stx.lemmas(type), sum));
	}
	if (stx.distributed || stx.integrated) {
		printKeyValue("  Distributed", "%-8" PRIu64, stx.distributed);
		printf(" (Ratio: %6.2f%% Average LBD: %.2f) \n", stx.distRatio()*100.0, stx.avgDistLbd());
		printKeyValue("  Integrated", "%-8" PRIu64, stx.integrated);
		if (accu){ printf(" (Ratio: %6.2f%% ", stx.intRatio()*100.0); }
		else     { printf(" ("); }
		printf("Unit: %" PRIu64" Average Jumps: %.2f)\n", stx.intImps, stx.avgIntJump());
	}	
}
void TextOutput::visitJumpStats(const JumpStats& st, bool) {
	printKeyValue("Backjumps", "%-8" PRIu64, st.jumps);
	printf(" (Average: %5.2f Max: %3u Sum: %6" PRIu64")\n", st.avgJump(), st.maxJump, st.jumpSum);
	printKeyValue("  Executed", "%-8" PRIu64, st.jumps-st.bounded);
	printf(" (Average: %5.2f Max: %3u Sum: %6" PRIu64" Ratio: %6.2f%%)\n", st.avgJumpEx(), st.maxJumpEx, st.jumped(), st.jumpedRatio()*100.0);
	printKeyValue("  Bounded", "%-8" PRIu64, st.bounded);
	printf(" (Average: %5.2f Max: %3u Sum: %6" PRIu64" Ratio: %6.2f%%)\n", st.avgBound(), st.maxBound, st.boundSum, 100.0 - (st.jumpedRatio()*100.0));
}

#undef printKeyValue
#undef printKey
#undef printLN
#undef printBR
}}
