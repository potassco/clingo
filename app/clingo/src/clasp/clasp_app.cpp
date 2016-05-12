// 
// Copyright (c) 2006-2012, Benjamin Kaufmann
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
#include "clasp_app.h"
#include <clasp/parser.h>
#include <clasp/solver.h>
#include <clasp/dependency_graph.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <climits>
#ifdef _WIN32
#define snprintf _snprintf
#pragma warning (disable : 4996)
#endif
#include <clasp/clause.h>
namespace Clasp {
/////////////////////////////////////////////////////////////////////////////////////////
// Some helpers
/////////////////////////////////////////////////////////////////////////////////////////
inline std::ostream& operator << (std::ostream& os, Literal l) {
	if (l.sign()) os << '-';
	os << l.var();
	return os;
}
inline std::istream& operator >> (std::istream& in, Literal& l) {
	int i;
	if (in >> i) {
		l = Literal(i >= 0 ? Var(i) : Var(-i), i < 0);
	}
	return in;
}
inline bool isStdIn(const std::string& in)  { return in == "-" || in == "stdin"; }
inline bool isStdOut(const std::string& out){ return out == "-" || out == "stdout"; }
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspAppOptions
/////////////////////////////////////////////////////////////////////////////////////////
namespace Cli {
ClaspAppOptions::ClaspAppOptions() : outf(0), ifs(' '), hideAux(false), onlyPre(false), printPort(false), outLbd(Activity::MAX_LBD), inLbd(Activity::MAX_LBD) {
	quiet[0] = quiet[1] = quiet[2] = static_cast<uint8>(UCHAR_MAX);
}
void ClaspAppOptions::initOptions(ProgramOptions::OptionContext& root) {
	using namespace ProgramOptions;
	OptionGroup basic("Basic Options");
	basic.addOptions()
		("quiet,q"    , notify(this, &ClaspAppOptions::mappedOpts)->implicit("2,2,2")->arg("<levels>"), 
		 "Configure printing of models, costs, and calls\n"
		 "      %A: <mod>[,<cost>][,<call>]\n"
		 "        <mod> : print {0=all|1=last|2=no} models\n"
		 "        <cost>: print {0=all|1=last|2=no} optimize values [<m>]\n"
		 "        <call>: print {0=all|1=last|2=no} call steps      [2]")
		("pre" , flag(onlyPre), "Run preprocessing and exit")
		("print-portfolio" , flag(printPort), "Print default portfolio and exit")
		("outf,@1", storeTo(outf)->arg("<n>"), "Use {0=default|1=competition|2=JSON|3=no} output")
		("out-atomf,@1" , storeTo(outAtom), "Set atom format string (<Pre>?%%s<Post>?)")
		("out-ifs,@1"   , notify(this, &ClaspAppOptions::mappedOpts), "Set internal field separator")
		("out-hide-aux,@1", flag(hideAux), "Hide auxiliary atoms in answers")
		("lemma-out,@1" , storeTo(lemmaOut)->arg("<file>"), "Write learnt lemmas to %A on exit")
		("lemma-out-lbd,@1",notify(this, &ClaspAppOptions::mappedOpts)->arg("<n>"), "Only write lemmas with lbd <= %A")
		("lemma-in,@1"  , storeTo(lemmaIn)->arg("<file>"), "Read additional lemmas from %A")
		("lemma-in-lbd,@1", notify(this, &ClaspAppOptions::mappedOpts)->arg("<n>"), "Initialize lbd of additional lemmas to <n>")
		("hcc-out,@1", storeTo(hccOut)->arg("<file>"), "Write non-hcf programs to %A.#scc")
		("file,f,@2", storeTo(input)->composing(), "Input files")
	;
	root.add(basic);
}
bool ClaspAppOptions::mappedOpts(ClaspAppOptions* this_, const std::string& name, const std::string& value) {
	uint32 x;
	if (name == "quiet") {
		const char* err = 0;
		uint32      q[3]= {uint32(UCHAR_MAX),uint32(UCHAR_MAX),uint32(UCHAR_MAX)};
		int      parsed = bk_lib::xconvert(value.c_str(), q, &err);
		for (int i = 0; i != parsed; ++i) { this_->quiet[i] = static_cast<uint8>(q[i]); }
		return parsed && *err == 0;
	}
	else if (name == "out-ifs") {
		if (value.empty() || value.size() > 2) { return false;}
		if (value.size() == 1) { this_->ifs = value[0]; return true; }
		if (value[1] == 't')   { this_->ifs = '\t'; return true; }
		if (value[1] == 'n')   { this_->ifs = '\n'; return true; }
		if (value[1] == 'v')   { this_->ifs = '\v'; return true; }
		if (value[1] == '\\')  { this_->ifs = '\\'; return true; }
	}
	else if (name.find("-lbd") != std::string::npos && bk_lib::string_cast(value, x) && x < Activity::MAX_LBD) {
		if      (name == "lemma-out-lbd") { this_->outLbd = (uint8)x; return true; }
		else if (name == "lemma-in-lbd")  { this_->inLbd  = (uint8)x; return true; }
	}
	return false; 
}
bool ClaspAppOptions::validateOptions(const ProgramOptions::ParsedOptions&) {
	if (quiet[1] == static_cast<uint8>(UCHAR_MAX)) { quiet[1] = quiet[0]; }
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspAppBase
/////////////////////////////////////////////////////////////////////////////////////////
ClaspAppBase::ClaspAppBase() { }
ClaspAppBase::~ClaspAppBase(){ }
const int* ClaspAppBase::getSignals() const {
	static const int signals[] = { 
		SIGINT, SIGTERM
#if !defined (_WIN32)
		, SIGUSR1, SIGUSR2, SIGQUIT, SIGHUP, SIGXCPU, SIGXFSZ
#endif
		, 0};
		return signals;
}
bool ClaspAppBase::parsePositional(const std::string& t, std::string& out) {
	int num;
	if   (bk_lib::string_cast(t, num)) { out = "number"; }
	else                               { out = "file";   }
	return true;
}
void ClaspAppBase::initOptions(ProgramOptions::OptionContext& root) {
	claspConfig_.addOptions(root);
	claspAppOpts_.initOptions(root);
	root.find("verbose")->get()->value()->defaultsTo("1");
}

void ClaspAppBase::validateOptions(const ProgramOptions::OptionContext&, const ProgramOptions::ParsedOptions& parsed, const ProgramOptions::ParsedValues& values) {
	if (claspAppOpts_.printPort) {
		printTemplate();
		exit(E_UNKNOWN);
	}
	if (!claspAppOpts_.validateOptions(parsed) || !claspConfig_.finalize(parsed, getProblemType(), true)) {
		error("command-line error!");
		exit(E_NO_RUN);
	}
	ClaspAppOptions& app = claspAppOpts_;
	if (!app.lemmaOut.empty() && !isStdOut(app.lemmaOut)) {
		if (std::find(app.input.begin(), app.input.end(), app.lemmaOut) != app.input.end() || app.lemmaIn == app.lemmaOut) {
			error("'lemma-out': cowardly refusing to overwrite input file!");
			exit(E_NO_RUN);
		}
	}
	if (!app.lemmaIn.empty() && !isStdIn(app.lemmaIn) && !std::ifstream(app.lemmaIn.c_str()).is_open()) {
		error("'lemma-in': could not open file!");
		exit(E_NO_RUN);
	}
	for (std::size_t i = 1; i < app.input.size(); ++i) {
		if (!isStdIn(app.input[i]) && !std::ifstream(app.input[i].c_str()).is_open()) {
			error(clasp_format_error("'%s': could not open input file!", app.input[i].c_str()));
			exit(E_NO_RUN);
		}
	}
	storeCommandArgs(values);
}
void ClaspAppBase::setup() {
	ProblemType pt = getProblemType();
	clasp_         = new ClaspFacade();
	if (!claspAppOpts_.onlyPre) {
		out_ = createOutput(pt);
		Event::Verbosity verb	= (Event::Verbosity)std::min(verbose(), (uint32)Event::verbosity_max);
		if (out_.get() && out_->verbosity() < (uint32)verb) { verb = (Event::Verbosity)out_->verbosity(); }
		EventHandler::setVerbosity(Event::subsystem_facade , verb);
		EventHandler::setVerbosity(Event::subsystem_load   , verb);
		EventHandler::setVerbosity(Event::subsystem_prepare, verb);
		EventHandler::setVerbosity(Event::subsystem_solve  , verb);
		clasp_->ctx.setEventHandler(this);
	}
	else if (pt != Problem_t::ASP) { 
		error("Option '--pre' only supported for ASP!"); 
		exit(E_NO_RUN); 
	}
}

void ClaspAppBase::shutdown() {
	if (clasp_.get()) {
		const ClaspFacade::Summary& result = clasp_->shutdown();
		if (out_.get()) { out_->shutdown(result); }
		if (!claspAppOpts_.lemmaOut.empty()) {
			std::ofstream file;
			std::ostream* os = &std::cout;
			if (!isStdOut(claspAppOpts_.lemmaOut)) {
				file.open(claspAppOpts_.lemmaOut.c_str());
				os = &file;
			}
			WriteLemmas lemmaOut(*os);
			lemmaOut.attach(clasp_->ctx);
			Constraint_t::Set x; x.addSet(Constraint_t::learnt_conflict);
			lemmaOut.flush(x, claspAppOpts_.outLbd);
			lemmaOut.detach();
		}
		int ec = getExitCode();
		((ec |= exitCode(result)) & E_ERROR) != 0 ? exit(ec) : setExitCode(ec);
	}
	out_   = 0;
	clasp_ = 0;
}

void ClaspAppBase::run() {
	if (out_.get()) { out_->run(getName(), getVersion(), &claspAppOpts_.input[0], &claspAppOpts_.input[0] + claspAppOpts_.input.size()); }
	try        { run(*clasp_); }
	catch(...) {
		try { blockSignals(); setExitCode(E_ERROR); throw; }
		catch (const std::bad_alloc&  ) { setExitCode(E_MEMORY); error("std::bad_alloc"); }
		catch (const std::exception& e) { error(e.what()); }
		catch (...)                     { ; }
	}
}

bool ClaspAppBase::onSignal(int sig) {
	if (!clasp_.get() || !clasp_->terminate(sig)) {
		info("INTERRUPTED by signal!");
		setExitCode(E_INTERRUPT);
		shutdown();
		exit(getExitCode());
	}
	else {
		// multiple threads are active - shutdown was initiated
		info("Shutting down threads...");
	}
	return false; // ignore all future signals
}

void ClaspAppBase::onEvent(const Event& ev) {
	const LogEvent* log = event_cast<LogEvent>(ev);
	if (log && log->isWarning()) {
		warn(log->msg);
		return;
	}
	if (out_.get()) {
		blockSignals();
		out_->onEvent(ev);
		unblockSignals(true);
	}
}

bool ClaspAppBase::onModel(const Solver& s, const Model& m) {
	bool ret = true;
	if (out_.get() && !out_->quiet()) {
		blockSignals();
		ret = out_->onModel(s, m);
		unblockSignals(true);
	}
	return ret;
}

int ClaspAppBase::exitCode(const RunSummary& run) const {
	int ec = 0;
	if (run.sat())               { ec |= E_SAT;       }
	if (run.complete())          { ec |= E_EXHAUST;   }
	if (run.result.interrupted()){ ec |= E_INTERRUPT; }
	return ec;
}

void ClaspAppBase::printTemplate() const {
	printf(
		"# clasp %s configuration file\n"
		"# A configuration file contains a (possibly empty) list of configurations.\n"
		"# Each of which must have the following format:\n"
		"#   <name>: <cmd>\n"
		"# where <name> is a string that must not contain ':'\n"
		"# and   <cmd>  is a command-line string of clasp options in long-format, e.g.\n"
		"# ('--heuristic=vsids --restarts=L,100').\n"
		"#\n"
		"# SEE: clasp --help\n"
		"#\n"
		"# NOTE: The options '--configuration' and '--tester' must not occur in a\n"
		"#       configuration file. Furthermore, global options are ignored in all\n"
		"#       but the first configuration.\n"
		"#\n"
		"# NOTE: Options given on the command-line are added to all configurations in a\n"
		"#       configuration file. If an option is given both on the command-line and\n"
		"#       in a configuration file, the one from the command-line is preferred.\n"
		"#\n"
		"# NOTE: If, after adding command-line options, a configuration\n"
		"#       contains mutually exclusive options an error is raised.\n"
		"#\n", CLASP_VERSION);
	for (ConfigIter it = ClaspCliConfig::getConfig(Clasp::Cli::config_many); it.valid(); it.next()) {
		printf("%s: %s\n", it.name(), it.args());
	}
}

void ClaspAppBase::printVersion() {
	ProgramOptions::Application::printVersion();
	printLibClaspVersion();
}

void ClaspAppBase::printLibClaspVersion() const {
	if (strcmp(getName(), "clasp") != 0) {
		printf("libclasp version %s\n", CLASP_VERSION);
	}
	printf("Configuration: WITH_THREADS=%d", WITH_THREADS);
#if WITH_THREADS
	printf(" (Intel TBB version %d.%d)", TBB_VERSION_MAJOR, TBB_VERSION_MINOR);
#endif
	printf("\n%s\n", CLASP_LEGAL);
	fflush(stdout);
}

void ClaspAppBase::printHelp(const ProgramOptions::OptionContext& root) {
	ProgramOptions::Application::printHelp(root);
	if (root.getActiveDescLevel() >= ProgramOptions::desc_level_e1) {
		printf("[asp] %s\n", ClaspCliConfig::getDefaults(Problem_t::ASP));
		printf("[cnf] %s\n", ClaspCliConfig::getDefaults(Problem_t::SAT));
		printf("[opb] %s\n", ClaspCliConfig::getDefaults(Problem_t::PB));
	}
	if (root.getActiveDescLevel() >= ProgramOptions::desc_level_e2) {
		printf("\nDefault configurations:\n");
		printDefaultConfigs();
	}
	fflush(stdout);
}

void ClaspAppBase::printDefaultConfigs() const {
	uint32 minW = 2, maxW = 80;
	std::string cmd;
	for (int i = Clasp::Cli::config_default+1; i != Clasp::Cli::config_default_max_value; ++i) {
		ConfigIter it = ClaspCliConfig::getConfig(static_cast<Clasp::Cli::ConfigKey>(i));
		printf("%s:\n%*c", it.name(), minW-1, ' ');
		cmd = it.args();
		// split options into formatted lines
		std::size_t sz = cmd.size(), off = 0, n = maxW - minW;
		while (n < sz) {
			while (n != off  && cmd[n] != ' ') { --n; }
			if (n != off) { cmd[n] = 0; printf("%s\n%*c", &cmd[off], minW-1, ' '); }
			else          { break; }
			off = n+1;
			n   = (maxW - minW) + off;
		}
		printf("%s\n", cmd.c_str()+off);
	}
}

void ClaspAppBase::readLemmas(SharedContext& ctx) {
	std::ifstream fileStream;	
	std::istream& file = isStdIn(claspAppOpts_.lemmaIn) ? std::cin : (fileStream.open(claspAppOpts_.lemmaIn.c_str()), fileStream);
	Solver& s          = *ctx.master();
	bool ok            = !s.hasConflict();
	uint32 numVars;
	for (ClauseCreator clause(&s); file && ok; ) {
		while (file.peek() == 'c' || file.peek() == 'p') { 
			const char* m = file.get() == 'p' ? " cnf" : " clasp";
			while (file.get() == *m) { ++m; }
			if (!*m && (!(file >> numVars) || numVars != ctx.numVars()) ) {
				throw std::runtime_error("Wrong number of vars in file: "+claspAppOpts_.lemmaIn); 
			}
			while (file.get() != '\n' && file) {} 
		}
		Literal x; bool elim = false; 
		clause.start(Constraint_t::learnt_conflict);
		clause.setLbd(claspAppOpts_.inLbd);
		while ( (file >> x) ) {
			if (x.var() == 0)         { ok = elim || clause.end(); break; }			
			elim = elim || ctx.eliminated(x.var());
			if (!s.validVar(x.var())) { throw std::runtime_error("Bad variable in file: "+claspAppOpts_.lemmaIn); }
			if (!elim)                { clause.add(x); }
		}
		if (x.var() != 0) { throw std::runtime_error("Unrecognized format: "+claspAppOpts_.lemmaIn); }
	}
	if (ok && !file.eof()){ throw std::runtime_error("Error reading file: "+claspAppOpts_.lemmaIn); }
	s.simplify();
}

void ClaspAppBase::writeNonHcfs(const SharedDependencyGraph& graph) const {
	uint32 scc = 0;
	char   buf[10];
	std::ofstream file;
	for (SharedDependencyGraph::NonHcfIter it = graph.nonHcfBegin(), end = graph.nonHcfEnd(); it != end; ++it, ++scc) {
		snprintf(buf, 10, ".%u", scc);
		std::string n     = claspAppOpts_.hccOut + buf;
		if (!isStdOut(claspAppOpts_.hccOut) && (file.open(n.c_str()), !file.is_open())) { throw std::runtime_error("Could not open hcc file!\n"); }
		WriteCnf cnf(file.is_open() ? file : std::cout);
		const SharedContext& ctx = it->second->ctx();
		cnf.writeHeader(ctx.numVars(), ctx.numConstraints());
		cnf.write(ctx.numVars(), ctx.shortImplications());
		Solver::DBRef db = ctx.master()->constraints();
		for (uint32 i = 0; i != db.size(); ++i) {
			if (ClauseHead* x = db[i]->clause()) { cnf.write(x); }
		}
		for (uint32 i = 0; i != ctx.master()->trail().size(); ++i) {
			cnf.write(ctx.master()->trail()[i]);
		}
		cnf.close();
		file.close();
	}
}
std::istream& ClaspAppBase::getStream() {
	ProgramOptions::StringSeq& input = claspAppOpts_.input;
	if (input.empty() || isStdIn(input[0])) {
		input.resize(1, "stdin");
		return std::cin;
	}
	else {
		static std::ifstream file;
		if (file.is_open()) return file;
		file.open(input[0].c_str());
		if (!file) { throw std::runtime_error("Can not read from '"+input[0]+"'");  }
		return file;
	}
}

// Creates output object suitable for given input format
Output* ClaspAppBase::createOutput(ProblemType f) {
	SingleOwnerPtr<Output> out;
	if (claspAppOpts_.outf == ClaspAppOptions::out_none) {
		return 0;
	}
	if (claspAppOpts_.outf != ClaspAppOptions::out_json || claspAppOpts_.onlyPre) {
		TextOutput::Format outFormat = TextOutput::format_asp;
		if      (f == Problem_t::SAT){ outFormat = TextOutput::format_sat09; }
		else if (f == Problem_t::PB) { outFormat = TextOutput::format_pb09;  }
		else if (f == Problem_t::ASP && claspAppOpts_.outf == ClaspAppOptions::out_comp) {
			outFormat = TextOutput::format_aspcomp;
		}
		out.reset(new TextOutput(verbose(), outFormat, claspAppOpts_.outAtom.c_str(), claspAppOpts_.ifs));
		if (claspConfig_.solve.maxSat && f == Problem_t::SAT) {
			static_cast<TextOutput*>(out.get())->result[TextOutput::res_sat] = "UNKNOWN";
		}
	}
	else {
		out.reset(new JsonOutput(verbose()));
	}
	if (claspAppOpts_.quiet[0] != static_cast<uint8>(UCHAR_MAX)) {
		out->setModelQuiet((Output::PrintLevel)std::min(uint8(Output::print_no), claspAppOpts_.quiet[0]));
	}
	if (claspAppOpts_.quiet[1] != static_cast<uint8>(UCHAR_MAX)) {
		out->setOptQuiet((Output::PrintLevel)std::min(uint8(Output::print_no), claspAppOpts_.quiet[1]));
	}
	if (claspAppOpts_.quiet[2] != static_cast<uint8>(UCHAR_MAX)) {
		out->setCallQuiet((Output::PrintLevel)std::min(uint8(Output::print_no), claspAppOpts_.quiet[2]));
	}
	if (claspAppOpts_.hideAux) {
		out->setHide('_');
	}
	return out.release();
}
void ClaspAppBase::storeCommandArgs(const ProgramOptions::ParsedValues&) { 
	/* We don't need the values */
}

bool ClaspAppBase::handlePostGroundOptions(ProgramBuilder& prg) {
	if (!claspAppOpts_.onlyPre || prg.type() != Problem_t::ASP) { return true; }
	if (prg.endProgram()) { static_cast<Asp::LogicProgram&>(prg).write(std::cout); }
	else                  { std::cout << "0\n0\nB+\n1\n0\nB-\n1\n0\n0\n"; }
	exit(E_UNKNOWN);
	return false;
}
bool ClaspAppBase::handlePreSolveOptions(ClaspFacade& clasp) {
	if (!claspAppOpts_.lemmaIn.empty() && clasp.step() == 0)       { readLemmas(clasp.ctx); }
	if (!claspAppOpts_.hccOut.empty()  && clasp.ctx.sccGraph.get()){ writeNonHcfs(*clasp.ctx.sccGraph); }
	return true;
}
void ClaspAppBase::run(ClaspFacade& clasp) {
	ProblemType     pt    = getProblemType();
	StreamSource    input(getStream());
	bool            inc   = pt == Problem_t::ASP && *input == '9';
	ProgramBuilder& prg   = clasp.start(claspConfig_, pt);
	if (inc) { inc = clasp.enableProgramUpdates(); }
	else     { claspConfig_.releaseOptions(); }
	while (prg.parseProgram(input) && handlePostGroundOptions(prg)) {
		if (clasp.prepare() && handlePreSolveOptions(clasp)) {
			clasp.solve();
		}
		if (!inc || clasp.result().interrupted() || !input.skipWhite() || *input != '9' || !clasp.update().ok()) { break; }
		prg.disposeMinimizeConstraint();
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspApp
/////////////////////////////////////////////////////////////////////////////////////////
ClaspApp::ClaspApp() {}

ProblemType ClaspApp::getProblemType() {
	InputFormat input = Input_t::detectFormat(getStream());
	return Problem_t::format2Type(input);
}

void ClaspApp::run(ClaspFacade& clasp) {
	ClaspAppBase::run(clasp);
}

void ClaspApp::printHelp(const ProgramOptions::OptionContext& root) {
	ClaspAppBase::printHelp(root);
	printf("\nclasp is part of Potassco: %s\n", "http://potassco.sourceforge.net/#clasp");
	printf("Get help/report bugs via : http://sourceforge.net/projects/potassco/support\n");
	fflush(stdout);
}
/////////////////////////////////////////////////////////////////////////////////////////
// WriteLemmas
/////////////////////////////////////////////////////////////////////////////////////////
WriteLemmas::WriteLemmas(std::ostream& os) : ctx_(0), os_(os) {}
WriteLemmas::~WriteLemmas(){ detach(); }
void WriteLemmas::detach() { if (ctx_) { ctx_ = 0; } }
void WriteLemmas::attach(SharedContext& ctx) {
	detach();
	ctx_ = &ctx;
}
bool WriteLemmas::unary(Literal p, Literal x) const {
	if (!isSentinel(x) && x.asUint() > p.asUint() && (p.watched() + x.watched()) != 0) {
		os_ << ~p << " " << x << " 0\n"; 
		++outShort_;
	}
	return true;
}
bool WriteLemmas::binary(Literal p, Literal x, Literal y) const {
	if (x.asUint() > p.asUint() && y.asUint() > p.asUint() && (p.watched() + x.watched() + y.watched()) != 0) {
		os_ << ~p << " " << x << " " << y << " 0\n"; 
		++outShort_;
	}
	return true;
}
// NOTE: ON WINDOWS this function is unsafe if called from time-out handler because
// it has potential races with the main thread
void WriteLemmas::flush(Constraint_t::Set x, uint32 maxLbd) {
	if (!ctx_ || !os_) { return; }
	// write problem description
	os_ << "c clasp " << ctx_->numVars() << "\n";
	// write learnt units
	Solver& s         = *ctx_->master();
	const LitVec& t   = s.trail();
	Antecedent trueAnte(posLit(0));
	for (uint32 i = ctx_->numUnary(), end = s.decisionLevel() ? s.levelStart(1) : t.size(); i != end; ++i) {
		const Antecedent& a = s.reason(t[i]);
		if (a.isNull() || a.asUint() == trueAnte.asUint()) {
			os_ << t[i] << " 0\n";
		}
	}
	// write implicit learnt constraints
	uint32 numLearnts = ctx_->shortImplications().numLearnt();
	outShort_         = 0;
	for (Var v = 1; v <= ctx_->numVars() && outShort_ < numLearnts; ++v) {
		ctx_->shortImplications().forEach(posLit(v), *this);
		ctx_->shortImplications().forEach(negLit(v), *this);
	}
	// write explicit learnt conflict constraints matching the current filter
	LitVec lits; ClauseHead* c;
	for (LitVec::size_type i = 0; i != s.numLearntConstraints() && os_; ++i) {
		if ((c = s.getLearnt(i).clause()) != 0 && c->lbd() <= maxLbd && x.inSet(c->ClauseHead::type())) {
			lits.clear();
			c->toLits(lits);
			std::copy(lits.begin(), lits.end(), std::ostream_iterator<Literal>(os_, " "));
			os_ << "0\n";
		}
	}
	os_.flush();
}
/////////////////////////////////////////////////////////////////////////////////////////
// WriteCnf
/////////////////////////////////////////////////////////////////////////////////////////
void WriteCnf::writeHeader(uint32 numVars, uint32 numCons) {
	os_ << "p cnf " << numVars << " " << numCons << "\n";
}
void WriteCnf::write(ClauseHead* h) {
	lits_.clear();
	h->toLits(lits_);
	std::copy(lits_.begin(), lits_.end(), std::ostream_iterator<Literal>(os_, " "));
	os_ << "0\n";
}
void WriteCnf::write(Var maxVar, const ShortImplicationsGraph& g) {
	for (Var v = 1; v <= maxVar; ++v) {
		g.forEach(posLit(v), *this);
		g.forEach(negLit(v), *this);
	}	
}
void WriteCnf::write(Literal u) {
	os_ << u << " 0\n";
}
bool WriteCnf::unary(Literal p, Literal x) const {
	if (p.asUint() < x.asUint()) {
		os_ << ~p << " " << x << " 0\n";
	}
	return true;
}
bool WriteCnf::binary(Literal p, Literal x, Literal y) const {
	if (p.asUint() < x.asUint() && p.asUint() < y.asUint()) {
		os_ << ~p << " " << x << " " << y << " 0\n"; 
	}
	return true;
}
void WriteCnf::close() {
	os_ << std::flush;
}

}} // end of namespace Clasp::Cli

