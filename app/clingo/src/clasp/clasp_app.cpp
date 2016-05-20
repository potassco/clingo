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
#include <clasp/solver.h>
#include <clasp/dependency_graph.h>
#include <clasp/parser.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <climits>
#ifdef _WIN32
#define snprintf _snprintf
#pragma warning (disable : 4996)
#endif
#include <clasp/clause.h>

#if defined( __linux__ )
#include <fpu_control.h>
#define FPU_SWITCH_DOUBLE(oldW) _FPU_GETCW(oldW);\
	unsigned __t = ((oldW) & ~_FPU_EXTENDED & ~_FPU_SINGLE) | _FPU_DOUBLE;\
	_FPU_SETCW(__t)
#define FPU_RESTORE_DOUBLE(oldW) _FPU_SETCW(oldW)
#elif defined (_MSC_VER) && !defined(_WIN64)
#include <float.h>
#define FPU_SWITCH_DOUBLE(oldW) \
	(oldW) = _controlfp(0, 0); \
	_controlfp(_PC_53, _MCW_PC);
#define FPU_RESTORE_DOUBLE(oldW) \
	_controlfp((oldW), _MCW_PC);
#pragma fenv_access (on)
#endif

#if !defined(FPU_SWITCH_DOUBLE)
#define FPU_SWITCH_DOUBLE(x) 
#define FPU_RESTORE_DOUBLE(x)
#endif

namespace Clasp {
/////////////////////////////////////////////////////////////////////////////////////////
// Some helpers
/////////////////////////////////////////////////////////////////////////////////////////
unsigned doubleMode_g = ((unsigned)(sizeof(void*)*CHAR_BIT)) < 64;
double shutdownTime_g;
inline bool isStdIn(const std::string& in)  { return in == "-" || in == "stdin"; }
inline bool isStdOut(const std::string& out){ return out == "-" || out == "stdout"; }
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspAppOptions
/////////////////////////////////////////////////////////////////////////////////////////
namespace Cli {
ClaspAppOptions::ClaspAppOptions() : outf(0), compute(0), ifs(' '), hideAux(false), onlyPre(false), printPort(false), lemmaLbd(Clasp::LBD_MAX) {
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
		("parse-ext", notify(this, &ClaspAppOptions::mappedOpts)->flag(), "Enable extensions in smodels and dimacs input")
		("outf,@1", storeTo(outf)->arg("<n>"), "Use {0=default|1=competition|2=JSON|3=no} output")
		("out-atomf,@1" , storeTo(outAtom), "Set atom format string (<Pre>?%%s<Post>?)")
		("out-ifs,@1"   , notify(this, &ClaspAppOptions::mappedOpts), "Set internal field separator")
		("out-hide-aux,@1", flag(hideAux), "Hide auxiliary atoms in answers")
		("lemma-out,@1" , storeTo(lemmaLog)->arg("<file>"), "Write learnt lemmas to %A on exit")
		("lemma-out-lbd,@1",notify(this, &ClaspAppOptions::mappedOpts)->arg("<n>"), "Only write lemmas with lbd <= %A")
		("hcc-out,@1", storeTo(hccOut)->arg("<file>"), "Write non-hcf programs to %A.#scc")
		("file,f,@2" , storeTo(input)->composing(), "Input files")
		("compute,@2", storeTo(compute)->arg("<lit>"), "Force given literal to true")
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
	else if (name == "lemma-out-lbd") {
		return  bk_lib::string_cast(value, x) && x < Clasp::LBD_MAX && (this_->lemmaLbd = (uint8)x, true);
	}
	return name == "parse-ext";
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
	setExitCode(E_NO_RUN);
	using ProgramOptions::Error;
	ProblemType pt = getProblemType();
	if (!claspAppOpts_.validateOptions(parsed) || !claspConfig_.finalize(parsed, pt, true)) {
		throw Error("command-line error!");
	}
	ClaspAppOptions& app = claspAppOpts_;
	if (!app.lemmaLog.empty() && !isStdOut(app.lemmaLog)) {
		if (std::find(app.input.begin(), app.input.end(), app.lemmaLog) != app.input.end()) {
			throw Error("'lemma-out': cowardly refusing to overwrite input file!");
		}
	}
	for (std::size_t i = 1; i < app.input.size(); ++i) {
		if (!isStdIn(app.input[i]) && !std::ifstream(app.input[i].c_str()).is_open()) {
			throw Error(clasp_format_error("'%s': could not open input file!", app.input[i].c_str()));
		}
	}
	if (app.onlyPre && pt != Problem_t::Asp) {
		throw Error("Option '--pre' only supported for ASP!");
	}
	if (parsed.count("parse-ext") != 0) {
		claspConfig_.parse.enableMinimize();
		claspConfig_.parse.enableAcycEdges();
		claspConfig_.parse.enableProject();
	}
	setExitCode(0);
	storeCommandArgs(values);
}
void ClaspAppBase::setup() {
	ProblemType pt = getProblemType();
	clasp_         = new ClaspFacade();
	if (!claspAppOpts_.onlyPre) {
		if (doubleMode_g) { FPU_SWITCH_DOUBLE(doubleMode_g); }
		out_ = createOutput(pt);
		Event::Verbosity verb	= (Event::Verbosity)std::min(verbose(), (uint32)Event::verbosity_max);
		if (out_.get() && out_->verbosity() < (uint32)verb) { verb = (Event::Verbosity)out_->verbosity(); }
		if (!claspAppOpts_.lemmaLog.empty()) {
			logger_ = new LemmaLogger(claspAppOpts_.lemmaLog.c_str(), claspAppOpts_.lemmaLbd);
		}
		EventHandler::setVerbosity(Event::subsystem_facade , verb);
		EventHandler::setVerbosity(Event::subsystem_load   , verb);
		EventHandler::setVerbosity(Event::subsystem_prepare, verb);
		EventHandler::setVerbosity(Event::subsystem_solve  , verb);
		clasp_->ctx.setEventHandler(this, logger_.get() == 0 ? SharedContext::report_default : SharedContext::report_conflict);
	}
}

void ClaspAppBase::shutdown() {
	if (!clasp_.get()) { return; }
	if (logger_.get()) { logger_->close(); }
	const ClaspFacade::Summary& result = clasp_->shutdown();
	if (shutdownTime_g) {
		shutdownTime_g += RealTime::getTime();
		char msg[80];
		info(clasp_format(msg, sizeof(msg), "Shutdown completed in %.3f seconds", shutdownTime_g));
	}
	if (out_.get()) { out_->shutdown(result); }
	setExitCode(getExitCode() | exitCode(result));
	if (doubleMode_g) { FPU_RESTORE_DOUBLE(doubleMode_g); doubleMode_g = 1; }
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
	if (!clasp_.get() || !clasp_->interrupt(sig)) {
		info("INTERRUPTED by signal!");
		setExitCode(E_INTERRUPT);
		shutdown();
		exit(getExitCode());
	}
	else {
		// multiple threads are active - shutdown was initiated
		shutdownTime_g = -RealTime::getTime();
		info("Sending shutdown signal...");
	}
	return false; // ignore all future signals
}

void ClaspAppBase::onEvent(const Event& ev) {
	const LogEvent* log = event_cast<LogEvent>(ev);
	if (log && log->isWarning()) {
		warn(log->msg);
		return;
	}
	else if (const NewConflictEvent* cfl = event_cast<NewConflictEvent>(ev)) {
		if (logger_.get()) { logger_->add(*cfl->solver, *cfl->learnt, cfl->info); }
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
		"#   <name>[(<config>)]: <cmd>\n"
		"# where <name> is a string that must not contain ':',\n"
		"# <config> is one of clasp's default configs (and optional)\n"
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
#if defined(WITH_THREADS) && defined(TBB_VERSION_MAJOR) && WITH_THREADS
	printf(" (Intel TBB version %d.%d)", TBB_VERSION_MAJOR, TBB_VERSION_MINOR);
#endif
	printf("\n%s\n", CLASP_LEGAL);
	fflush(stdout);
}

void ClaspAppBase::printHelp(const ProgramOptions::OptionContext& root) {
	ProgramOptions::Application::printHelp(root);
	if (root.getActiveDescLevel() >= ProgramOptions::desc_level_e1) {
		printf("[asp] %s\n", ClaspCliConfig::getDefaults(Problem_t::Asp));
		printf("[cnf] %s\n", ClaspCliConfig::getDefaults(Problem_t::Sat));
		printf("[opb] %s\n", ClaspCliConfig::getDefaults(Problem_t::Pb));
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
void ClaspAppBase::writeNonHcfs(const PrgDepGraph& graph) const {
	uint32 scc = 0;
	char   buf[10];
	for (PrgDepGraph::NonHcfIter it = graph.nonHcfBegin(), end = graph.nonHcfEnd(); it != end; ++it, ++scc) {
		snprintf(buf, 10, ".%u", scc);
		WriteCnf cnf(claspAppOpts_.hccOut + buf);
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
		if      (f == Problem_t::Sat){ outFormat = TextOutput::format_sat09; }
		else if (f == Problem_t::Pb) { outFormat = TextOutput::format_pb09;  }
		else if (f == Problem_t::Asp && claspAppOpts_.outf == ClaspAppOptions::out_comp) {
			outFormat = TextOutput::format_aspcomp;
		}
		out.reset(new TextOutput(verbose(), outFormat, claspAppOpts_.outAtom.c_str(), claspAppOpts_.ifs));
		if (claspConfig_.solve.maxSat && f == Problem_t::Sat) {
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
	if (claspAppOpts_.hideAux && clasp_.get()) {
		clasp_->ctx.output.setFilter('_');
	}
	return out.release();
}
void ClaspAppBase::storeCommandArgs(const ProgramOptions::ParsedValues&) { 
	/* We don't need the values */
}

bool ClaspAppBase::handlePostGroundOptions(ProgramBuilder& prg) {
	if (!claspAppOpts_.onlyPre) { 
		if (logger_.get()) { logger_->start(prg); }
		return true; 
	}
	prg.endProgram();
	if (prg.type() == Problem_t::Asp) {
		AspParser::write(static_cast<Asp::LogicProgram&>(prg), std::cout);
	}
	else {
		error("Option '--pre': unsupported input format!");
		setExitCode(E_ERROR);
	}
	return false;
}
bool ClaspAppBase::handlePreSolveOptions(ClaspFacade& clasp) {
	if (!claspAppOpts_.hccOut.empty() && clasp.ctx.sccGraph.get()){ writeNonHcfs(*clasp.ctx.sccGraph); }
	return true;
}
void ClaspAppBase::run(ClaspFacade& clasp) {
	clasp.start(claspConfig_, getStream());
	if (!clasp.incremental()) {
		claspConfig_.releaseOptions();
	}
	if (claspAppOpts_.compute && clasp.program()->type() == Problem_t::Asp) {
		bool val = claspAppOpts_.compute < 0;
		Var  var = static_cast<Var>(!val ? claspAppOpts_.compute : -claspAppOpts_.compute);
		static_cast<Asp::LogicProgram*>(clasp.program())->startRule().addToBody(var, val).endRule();
	}
	while (clasp.read()) {
		if (handlePostGroundOptions(*clasp.program())) {
			clasp.prepare();
			if (handlePreSolveOptions(clasp)) { clasp.solve(); }
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspApp
/////////////////////////////////////////////////////////////////////////////////////////
ClaspApp::ClaspApp() {}

ProblemType ClaspApp::getProblemType() {
	return ClaspFacade::detectProblemType(getStream());
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
// LemmaLogger
/////////////////////////////////////////////////////////////////////////////////////////
LemmaLogger::LemmaLogger(const std::string& to, uint32 maxLbd)
	: str_(isStdOut(to) ? stdout : fopen(to.c_str(), "w"))
	, lbd_(maxLbd)
	, fmt_(Problem_t::Asp) {
	CLASP_FAIL_IF(!str_, "Could not open lemma log file '%s'!", to.c_str());
}
LemmaLogger::~LemmaLogger() { close(); }
void LemmaLogger::start(ProgramBuilder& prg) {
	if ( (fmt_ = static_cast<Problem_t::Type>(prg.type())) == Problem_t::Asp && prg.endProgram() ) {
		// create solver variable to potassco literal mapping
		Asp::LogicProgram& asp = static_cast<Asp::LogicProgram&>(prg);
		for (Asp::Atom_t a = asp.startAtom(); a != asp.inputEnd(); ++a) {
			Literal sLit = asp.getLiteral(a);
			if (sLit.var() >= solver2asp_.size()) {
				solver2asp_.resize(sLit.var() + 1, 0);
			}
			Potassco::Lit_t& p = solver2asp_[sLit.var()];
			if (!p || (!sLit.sign() && p < 0)) {
				p = !sLit.sign() ? Potassco::lit(a) : Potassco::neg(a);
			}
		}
	}
}
void LemmaLogger::add(const Solver& s, const LitVec& cc, const ConstraintInfo& info) {
	LitVec temp;
	const LitVec* out = &cc;
	uint32 lbd = info.lbd();
	if (lbd > lbd_) { return; }
	if (info.aux() || std::find_if(cc.begin(), cc.end(), std::not1(std::bind1st(std::mem_fun(&Solver::inputVar), &s))) != cc.end()) {
		if (!s.resolveToInput(cc, temp, lbd) || lbd > lbd_) { return; }
		out = &temp;
	}
	PodVector<char>::type buf;
	buf.reserve(cc.size() * 2);
	switch (fmt_) {
		case Problem_t::Asp: formatAspif(*out , lbd, buf); break;
		case Problem_t::Sat: formatDimacs(*out, lbd, buf); break;
		case Problem_t::Pb:  formatOpb(*out, lbd, buf);    break;
		default: break;
	}
	if (!buf.empty()) { fwrite(&buf[0], sizeof(char), buf.size(), str_); }
}
void LemmaLogger::append(BufT& out, const char* fmt, int data) const {
	char temp[1024];
	int w = snprintf(temp, sizeof(temp), fmt, data);
	CLASP_FAIL_IF(w < 0 || w >= (int)sizeof(temp), "Invalid format!");
	out.insert(out.end(), temp, temp + w);
}
void LemmaLogger::formatDimacs(const LitVec& cc, uint32, BufT& buf) const {
	for (LitVec::const_iterator it = cc.begin(), end = cc.end(); it != end; ++it) {
		append(buf, "%d ", toInt(*it));
	}
	append(buf, "%d\n", 0);
}
void LemmaLogger::formatOpb(const LitVec& cc, uint32, BufT& buf) const {
	for (LitVec::const_iterator it = cc.begin(), end = cc.end(); it != end; ++it) {
		append(buf, "%+d ", it->sign() ? -1 : 1);
		append(buf, "x%d ", Potassco::atom(toInt(*it)));
	}
	append(buf, ">= %d;\n", 1);
}
void LemmaLogger::formatAspif(const LitVec& cc, uint32, BufT& out) const {
	append(out, "1 0 0 0 %d", (int)cc.size());
	for (LitVec::const_iterator it = cc.begin(), end = cc.end(); it != end; ++it) {
		Literal sLit = ~*it; // clause -> constraint
		Potassco::Lit_t a = sLit.var() < solver2asp_.size() ? solver2asp_[sLit.var()] : 0;
		if (!a) { out.clear(); return; }
		if (sLit.sign() != (a < 0)) { a = -a; }
		append(out, " %d", a);
	}
	out.push_back('\n');
}
void LemmaLogger::close() {
	if (!str_) { return; }
	solver2asp_.clear();
	fflush(str_);
	if (str_ != stdout) { fclose(str_); }
	str_ = 0;
}
/////////////////////////////////////////////////////////////////////////////////////////
// WriteCnf
/////////////////////////////////////////////////////////////////////////////////////////
WriteCnf::WriteCnf(const std::string& outFile) : str_(fopen(outFile.c_str(), "w")) {
	CLASP_FAIL_IF(!str_, "Could not open cnf file '%s'!", outFile.c_str());
}
WriteCnf::~WriteCnf() { close(); }
void WriteCnf::writeHeader(uint32 numVars, uint32 numCons) {
	fprintf(str_, "p cnf %u %u\n", numVars, numCons);
}
void WriteCnf::write(ClauseHead* h) {
	lits_.clear();
	h->toLits(lits_);
	for (LitVec::const_iterator it = lits_.begin(), end = lits_.end(); it != end; ++it) {
		fprintf(str_, "%d ", toInt(*it));
	}
	fprintf(str_, "%d\n", 0);
}
void WriteCnf::write(Var maxVar, const ShortImplicationsGraph& g) {
	for (Var v = 1; v <= maxVar; ++v) {
		g.forEach(posLit(v), *this);
		g.forEach(negLit(v), *this);
	}	
}
void WriteCnf::write(Literal u) {
	fprintf(str_, "%d 0\n", toInt(u));
}
bool WriteCnf::unary(Literal p, Literal x) const {
	return p.rep() >= x.rep() || fprintf(str_, "%d %d 0\n", toInt(~p), toInt(x)) > 0;
}
bool WriteCnf::binary(Literal p, Literal x, Literal y) const {
	return p.rep() >= x.rep() || p.rep() >= y.rep() || fprintf(str_, "%d %d %d 0\n", toInt(~p), toInt(x), toInt(y)) > 0;
}
void WriteCnf::close() {
	if (str_) {
		fflush(str_);
		fclose(str_);
		str_ = 0;
	}
}

}} // end of namespace Clasp::Cli

