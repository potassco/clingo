// 
// Copyright (c) 2009, Benjamin Kaufmann
// 
// This file is part of gringo. See http://www.cs.uni-potsdam.de/gringo/ 
// 
// gringo is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with gringo; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#include "gringo_app.h"
#include <gringo/grounder.h>
#include <gringo/gringoexception.h>
#include <gringo/smodelsoutput.h>
#include <gringo/lparseoutput.h>
#include <gringo/pilsoutput.h>
#include <gringo/lparseconverter.h>
#include <gringo/lparseparser.h>
#include <gringo/domain.h>
#if defined(WITH_CLASP)
#include <clasp/reader.h>
#include <clasp/smodels_constraints.h>
#include <gringo/claspoutput.h>
#include <iomanip>
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <signal.h>
#include <unistd.h>
using namespace std;
namespace gringo {

/////////////////////////////////////////////////////////////////////////////////////////
// Application - common stuff
/////////////////////////////////////////////////////////////////////////////////////////
Application::Application() {}
Application& Application::instance() {
#if !defined(WITH_CLASP)
	static GringoApp app;
#else
	static ClingoApp app;
#endif
	return app;
}

void Application::printWarnings() {
	while (!messages.warning.empty()) {
		cerr << "Warning: " << messages.warning.back() << endl;
		messages.warning.pop_back();
	}
}

int Application::run(int argc, char** argv) {
	try {
		if (!parse(argc, argv, getPositionalParser())) {
			// command-line error
			throw std::runtime_error( messages.error.c_str() );
		}
		if ((generic.help&&(printHelp(),1)) || (generic.version&&(printVersion(),1))) {
			return EXIT_SUCCESS;
		}
		printWarnings();
		installSigHandlers();
		return doRun();
	}
	catch (const std::bad_alloc& e) {
		cerr << "\nERROR: " << e.what() << endl;
		return S_MEMORY;
	}
#if defined(WITH_CLASP)
	catch (const Clasp::ClaspError& e) {
		cerr << "\nclasp " << e.what() << endl;
		return S_ERROR;
	}
#endif
	catch (const gringo::GrinGoException& e) {
		cerr << "\nGringo ERROR: " << e.what() << endl;
		return S_ERROR;
	}
	catch (const std::exception& e) {
		cerr << "\nERROR: " << e.what() << endl;
		return S_ERROR;
	}
}

void Application::sigHandler(int sig) {
	// ignore further signals
	signal(SIGINT, SIG_IGN);  // Ctrl + C
	signal(SIGTERM, SIG_IGN); // kill(but not kill -9)
	Application::instance().handleSignal(sig);
}

void Application::installSigHandlers() {
	if (signal(SIGINT, &Application::sigHandler) == SIG_IGN) {
		signal(SIGINT, SIG_IGN);
	}
	if (signal(SIGTERM, &Application::sigHandler) == SIG_IGN) {
		signal(SIGTERM, SIG_IGN);
	}
}

void Application::printHelp() const {
	cout << EXECUTABLE << " version " << getVersion()
		   << "\n\n"
			 << "Usage: " << EXECUTABLE << " " << getUsage()
			 << "\n"
			 << getHelp()
			 << "\n"
			 << "Usage: " << EXECUTABLE << " " << getUsage()
			 << "\n\n"
			 << "Default commandline: \n"
			 << "  " << EXECUTABLE << " " << getDefaults() << endl;
}

Streams::~Streams() {
	for(StreamVec::iterator i = streams.begin(); i != streams.end(); i++) {
		delete *i;
	}
}
void Streams::open(const std::vector<std::string>& fileList) {
	if (fileList.empty() || fileList[0] == "stdin") {
		streams.push_back(new std::istream(std::cin.rdbuf()));
	}
	else {
		for (std::vector<std::string>::const_iterator i = fileList.begin(); i != fileList.end(); ++i) {
			streams.push_back(new std::ifstream(i->c_str()));
			if (!streams.back()->good()) {
				throw GrinGoException(std::string("Error: could not open file: ") + *i);
			}
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// GringoApp
/////////////////////////////////////////////////////////////////////////////////////////
std::string GringoApp::getVersion() const { return GRINGO_VERSION; }
std::string GringoApp::getUsage()   const { return "[options] [files]"; }
ProgramOptions::PosOption GringoApp::getPositionalParser() const { return &gringo::parsePositional; }
void GringoApp::handleSignal(int) {
	printf("\n*** INTERRUPTED! ***\n");
	_exit(S_UNKNOWN);
}
void GringoApp::printSyntax() const {
	string indent(strlen(EXECUTABLE) + 5, ' ');
	cout << EXECUTABLE << " version " << GRINGO_VERSION << "\n\n";
	cout << "The  input  language   supports  standard  logic  programming" << std::endl
	     << "syntax, including  function  symbols (' f(X,g(a,X)) '),  classical negation  ('-a')," << std::endl
	     << "disjunction ('a | b :- c.'), and  various types of aggregates e.g." << std::endl
	     << std::endl
	     << indent << "'count'" << std::endl
	     << indent << indent << "0 count {a, b, not c} 2." << std::endl
	     << indent << indent << "0       {a, b, not c} 2." << std::endl
	     << indent << "'sum'" << std::endl
	     << indent << indent << "1 sum   [a=1, b=3, not c=-2] 2." << std::endl
	     << indent << indent << "1       [a=1, b=3, not c=-2] 2." << std::endl
	     << indent << "'min'" << std::endl
	     << indent << indent << "0 max   [a=1, b=2, not c=3] 5." << std::endl
	     << indent << "'max'" << std::endl
	     << indent << indent << "1 min   [a=2, b=4, not c=1] 3." << std::endl
	     << "Further  details and  notes on compatibility  to lparse" << std::endl
	     << "can be found at <http://potassco.sourceforge.net/>." << std::endl;
}

void GringoApp::printVersion() const {
	cout << EXECUTABLE << " " << getVersion()
		   << "\n\n";
	cout << "Copyright (C) Roland Kaminski" << "\n";
	cout << "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n";
	cout << "Gringo is free software: you are free to change and redistribute it.\n";
	cout << "There is NO WARRANTY, to the extent permitted by law." << endl;
}

int GringoApp::doRun() {
	if (opts.syntax) { printSyntax(); return EXIT_SUCCESS; }
	std::auto_ptr<NS_OUTPUT::Output> out;
	if (opts.smodelsOut) {
		out.reset(new NS_OUTPUT::SmodelsOutput(&std::cout, opts.shift));
	}
	else if (opts.textOut) {
		out.reset(new NS_OUTPUT::LparseOutput(&std::cout));
	}
	else if (opts.aspilsOut > 1) {
		out.reset(new NS_OUTPUT::PilsOutput(&std::cout, opts.aspilsOut));
	}
	if (out.get()) {
		ground(*out.get());
		if (opts.stats) {
			printGrounderStats(*out.get());
		}
	}
	return EXIT_SUCCESS;
}

void GringoApp::ground(NS_OUTPUT::Output& output) const {
	Streams s;
	addConstStream(s);
	s.open(generic.input);
	if(opts.convert) {
		LparseConverter parser(s.streams);
		if(!parser.parse(&output))
			throw gringo::GrinGoException("Error: Parsing failed.");
		// just an approximation
		if (opts.textOut) {
			output.stats_.atoms = 0;
			const DomainVector* d = parser.getDomains();
			for (DomainVector::const_iterator i = d->begin(); i != d->end(); ++i) {
				output.stats_.atoms += (*i)->getDomain().size();
			}
		}
	}
	else {
		Grounder grounder(opts.grounderOptions);
		LparseParser parser(&grounder, s.streams);
		if(!parser.parse(&output))
			throw gringo::GrinGoException("Error: Parsing failed.");
		grounder.prepare(false);
		if(generic.verbose > 1)
			cerr << "Grounding..." << endl;
		grounder.ground();
		// just an approximation
		if (opts.textOut) {
			output.stats_.atoms = 0;
			//output.stats_.atoms = grounder.getDomains()->size();
			const DomainVector* d = grounder.getDomains();
			for (DomainVector::const_iterator i = d->begin(); i != d->end(); ++i) {
				output.stats_.atoms += (*i)->getDomain().size();
			}
		}
	}
}
void GringoApp::printGrounderStats(NS_OUTPUT::Output& output) const {
	using NS_OUTPUT::Output;
	cerr << "=== Grounder Statistics ===" << std::endl;
	cerr << "#rules      : " << output.stats_.rules << std::endl;
	if (output.stats_.language == Output::Stats::TEXT) {
		cerr << "#headatoms  : " << output.stats_.atoms << std::endl;
	}
	else {
		cerr << "#atoms      : " << output.stats_.atoms << std::endl;
		if (output.stats_.language == Output::Stats::SMODELS)
			cerr << "#aux. atoms : " << output.stats_.auxAtoms << std::endl;
	}
	cerr << "#count      : " << output.stats_.count << std::endl;
	cerr << "#sum        : " << output.stats_.sum << std::endl;
	cerr << "#min        : " << output.stats_.min << std::endl;
	cerr << "#max        : " << output.stats_.max << std::endl;
	cerr << "#compute    : " << output.stats_.compute << std::endl;
	cerr << "#optimize   : " << output.stats_.optimize << std::endl;
}
void GringoApp::addConstStream(Streams& s) const {
	std::stringstream* constants = new std::stringstream;
	s.streams.push_back(constants);
	for(vector<std::string>::const_iterator i = opts.consts.begin(); i != opts.consts.end(); ++i) {
		*constants << "#const " << *i << ".\n";
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// ClingoApp
/////////////////////////////////////////////////////////////////////////////////////////
#if defined(WITH_CLASP)
#define STATUS(v1,x) if (generic.verbose<v1);else (x)
namespace {
// class for using gringos output as clasps input
struct FromGringo : public Clasp::Input {
	typedef std::auto_ptr<gringo::Grounder> GrounderPtr;
	typedef std::auto_ptr<gringo::LparseParser> ParserPtr;
	typedef std::auto_ptr<NS_OUTPUT::Output> OutputPtr;
	typedef Clasp::MinimizeConstraint* MinConPtr;
	FromGringo(const GringoOptions& opts, Streams& str, bool clingoMode) : clingo(clingoMode) {
		grounder.reset(new gringo::Grounder(opts.grounderOptions));
		parser.reset(new gringo::LparseParser(grounder.get(), str.streams));
		if (clingo) {
			out.reset(new NS_OUTPUT::ClaspOutput(0, opts.shift));
		}
#if defined(WITH_ICLASP)
		else {
			out.reset(new NS_OUTPUT::IClaspOutput(0, opts.shift));
		}
#endif
	}
	Format    format()      const { return Clasp::Input::SMODELS; }
	MinConPtr getMinimize(Clasp::Solver& s, Clasp::ProgramBuilder* api, bool heu) { 
		return api ? api->createMinimize(s, heu) : 0;
	}
	void      getAssumptions(Clasp::LitVec& a) {
		(void)a;
#if defined(WITH_ICLASP)
		if (!clingo) {
			const Clasp::AtomIndex& i = *solver->strategies().symTab.get();
			a.push_back(i.find(static_cast<NS_OUTPUT::IClaspOutput*>(out.get())->getIncUid())->lit);
		}
#endif
	}
	bool      read(Clasp::Solver& s, Clasp::ProgramBuilder* api, int) {
		static_cast<NS_OUTPUT::ClaspOutput*>(out.get())->setProgramBuilder(api);
		solver = &s;
		if (parser.get()) {
			if (!parser->parse(out.get())) throw gringo::GrinGoException("Error: Parsing failed.");
			grounder->prepare(!clingo);
			parser.reset(0);
		}
		grounder->ground();
		release();
		return true;
	}
	void release() {
		if (clingo) {
			grounder.reset(0);
			out.reset(0);
		}
	}
	GrounderPtr            grounder;
	ParserPtr              parser;
	OutputPtr              out;
	Clasp::Solver*         solver;
	bool                   clingo;
};
}
void ClingoApp::printVersion() const {
	GringoApp::printVersion();
	cout << endl;
	cout << "clasp " << CLASP_VERSION << "\n";
	cout << "Copyright (C) Benjamin Kaufmann" << "\n";
	cout << "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n";
	cout << "clasp is free software: you are free to change and redistribute it.\n";
	cout << "There is NO WARRANTY, to the extent permitted by law." << endl;
}
std::string ClingoApp::getVersion() const { 
	std::string r(GRINGO_VERSION);
	r += " (clasp ";
	r += CLASP_VERSION;
	r += ")";
	return r;
}
std::string ClingoApp::getUsage()   const { return "[number] [options] [files]"; }
ProgramOptions::PosOption ClingoApp::getPositionalParser() const { return &Clasp::parsePositional; }

void ClingoApp::handleSignal(int) {
	for (int i = 0; i != sizeof(timer_)/sizeof(Timer); ++i) { 
		timer_[i].stop(); 
	}
	fprintf(stderr, "\n*** INTERRUPTED! ***\n");
	if (facade_ && facade_->state() != Clasp::ClaspFacade::state_not_started) {
		printResult(ClingoApp::reason_interrupt);
	}
	_exit(solver_.stats.solve.models != 0 ?  S_SATISFIABLE : S_UNKNOWN);
}

void ClingoApp::configureInOut(Streams& s) {
	using namespace Clasp;
	in_.reset(0);
	facade_ = 0;
	if (clingo_.claspMode) {
		s.open(generic.input);
		if (generic.input.size() > 1) { messages.warning.push_back("Only first file will be used"); }
		in_.reset(new StreamInput(*s.streams[0], detectFormat(*s.streams[0])));
	}
	else {
		GringoApp::addConstStream(s);
		s.open(generic.input);
		in_.reset(new FromGringo(opts, s, clingo_.clingoMode));
	}
	if (config_.onlyPre) { 
		if (clingo_.claspMode || clingo_.clingoMode) {
			generic.verbose = 0; 
		}
		else { warning("Option '--pre' is ignored in incremental setting!"); config_.onlyPre = false; }
	}
	if (in_->format() == Input::SMODELS) {
		out_.reset(new AspOutput(cmdOpts_.basic.asp09));
		if (cmdOpts_.basic.asp09){ generic.verbose = 0; }
	}
	else if (in_->format() == Input::DIMACS) { out_.reset(new SatOutput()); }
	else if (in_->format() == Input::OPB)    { out_.reset(new PbOutput(generic.verbose > 1));  }
}

int ClingoApp::doRun() {
	using namespace Clasp;
	if (opts.onlyGround) {
		opts.stats = cmdOpts_.basic.stats != 0;
		return GringoApp::doRun();
	}
	if (cmdOpts_.basic.stats > 1) {
		solver_.stats.solve.enableJumpStats();
	}
	Streams s;
	configureInOut(s);
	ClaspFacade clasp; facade_ = &clasp;
	timer_[0].start();
	if (clingo_.claspMode || clingo_.clingoMode) {
		clingo_.iStats = false;
		clasp.solve(*in_, config_, this);
	}
	else {
		clasp.solveIncremental(*in_, config_, clingo_.inc, this);
	}
	timer_[0].stop();
	printResult(reason_end);
	if      (clasp.result() == ClaspFacade::result_unsat) return S_UNSATISFIABLE;
	else if (clasp.result() == ClaspFacade::result_sat)   return S_SATISFIABLE;
	else                                                  return S_UNKNOWN;
}

void ClingoApp::state(Clasp::ClaspFacade::Event e, Clasp::ClaspFacade& f) {
	using namespace Clasp;
	if (e == ClaspFacade::event_state_enter) {
		Application::printWarnings();
		if (f.state() == ClaspFacade::state_read) {
			if (f.step() == 0) {
				STATUS(2, cout << EXECUTABLE << " version " << getVersion() << "\n");
			}
			if (clingo_.iStats) {
				cout << "=============== step " << f.step()+1 << " ===============" << endl;
			}
			STATUS(2, cout << "Reading      : ");
		}	
		else if (f.state() == ClaspFacade::state_preprocess) {
			STATUS(2, cout << "Preprocessing: ");
		}
		else if (f.state() == ClaspFacade::state_solve) {
			STATUS(2, cout << "Solving...\n");
		}
		cout << flush;
		timer_[f.state()].start();
	}
	else if (e == ClaspFacade::event_state_exit) {
		timer_[f.state()].stop();
		if (generic.verbose > 1 && (f.state() == ClaspFacade::state_read || f.state() == ClaspFacade::state_preprocess)) {
			cout << fixed << setprecision(3) << timer_[f.state()].elapsed() << endl;
		}
		if (f.state() == ClaspFacade::state_solve) {
			stats_.accu(solver_.stats.solve);
			if (clingo_.iStats) {
				timer_[0].stop();
				cout << "\nModels   : " << solver_.stats.solve.models << "\n"
						 << "Time     : " << fixed << setprecision(3) << timer_[0].current() << " (g: " << timer_[ClaspFacade::state_read].current()
						 << ", p: " << timer_[ClaspFacade::state_preprocess].current() << ", s: " << timer_[ClaspFacade::state_solve].current() << ")\n"
						 << "Rules    : " << f.api()->stats.rules[0] << "\n" 
						 << "Choices  : " << solver_.stats.solve.choices   << "\n"
						 << "Conflicts: " << solver_.stats.solve.conflicts << "\n";
				timer_[0].start();
			}
			solver_.stats.solve.reset();
		}
	}
}

void ClingoApp::event(Clasp::ClaspFacade::Event e, Clasp::ClaspFacade& f) {
	using namespace Clasp;
	if (e == ClaspFacade::event_model) {
		if (!cmdOpts_.basic.quiet) { 
			if ( !(config_.enumerate.consequences()) ) {
				STATUS(1, cout << "Answer: " << solver_.stats.solve.models << endl);
				out_->printModel(solver_, *config_.solve.enumerator());
			}
			else {
				STATUS(1, cout << config_.enumerate.cbType() << " consequences:" << endl); 
				out_->printConsequences(solver_, *config_.solve.enumerator());
			}
			if (config_.solve.enumerator()->minimize()) {
				out_->printOptimize(*config_.solve.enumerator()->minimize());
			}
		}
	}
	else if (e == ClaspFacade::event_p_prepared) {
		if (config_.onlyPre) {
			if (f.api()) f.releaseApi(); // keep api so that we can later print the program
			else         STATUS(0, cout << "Vars: " << solver_.numVars() << " Constraints: " <<  solver_.numConstraints()<<endl);
			AtomIndex* x = solver_.strategies().symTab.release();
			solver_.reset(); // release constraints and strategies - no longer needed
			solver_.strategies().symTab.reset(x);
		}
		else { out_->initSolve(solver_, f.api(), f.config()->solve.enumerator()); }
	}
}

void ClingoApp::printResult(ReasonEnd end) {
	using namespace Clasp;
	if (config_.onlyPre) {
		if (end != reason_end) return;
		if (facade_->api()) { // asp-mode
			facade_->result() == ClaspFacade::result_unsat
				? (void)(std::cout << "0\n0\nB+\n1\n0\nB-\n1\n0\n0\n")
				: facade_->api()->writeProgram(std::cout);
			delete facade_->releaseApi();
		}
		else {
			if (facade_->result() != ClaspFacade::result_unsat) {
				STATUS(0, cout << "Search not started because of option '--pre'!" << endl);
			}
			cout << "S UNKNWON" << endl;
		}
		return;
	}
	bool complete        = end == reason_end && !facade_->more();	
	Solver& s            = solver_;
	s.stats.solve.accu(stats_);
	const Enumerator& en = *config_.solve.enumerator();
	if (clingo_.iStats) {
		cout << "=============== Summary ===============" << endl;
	}
	out_->printSolution(s, en, complete);
	if (cmdOpts_.basic.quiet && config_.enumerate.consequences() && s.stats.solve.models != 0) {
		STATUS(1, cout << config_.enumerate.cbType() << " consequences:\n");
		out_->printConsequences(s, en);
	}
	if (generic.verbose > 0) {
		const char* c= out_->format[OutputFormat::comment];
		const int   w= 12-(int)strlen(c);
		if      (end == reason_timeout)  { cout << "\n" << c << "TIME LIMIT  : 1\n"; }
		else if (end == reason_interrupt){ cout << "\n" << c << "INTERRUPTED : 1\n"; }
		uint64 enumerated = s.stats.solve.models;
		uint64 models     = enumerated;
		if      (config_.enumerate.consequences() && enumerated > 1) { models = 1; }
		else if (en.minimize())                                      { models = en.minimize()->models(); }
		cout << "\n" << c << left << setw(w) << "Models" << ": ";
		if (!complete) {
			char buf[64];
			int wr    = sprintf(buf, "%"PRIu64, models);
			buf[wr]   = '+';
			buf[wr+1] = 0;
			cout << setw(6) << buf << "\n";
		}
		else {
			cout << setw(6) << models << "\n";
		}
		if (enumerated) {
			if (enumerated != models) { 
				cout << c << setw(w) << "  Enumerated" << ": " << enumerated << "\n";
			}
			if (config_.enumerate.consequences()) {
				cout << c <<"  " <<  setw(w-2) << config_.enumerate.cbType() << ": " << (complete?"yes":"unknown") << "\n";
			}
			if (en.minimize()) {
				cout << c << setw(w) << "  Optimum" << ": " << (complete?"yes":"unknown") << "\n";
				cout << c << setw(w) << "Optimization" << ": ";
				out_->printOptimizeValues(*en.minimize());
				cout << "\n";
			}
		}
		if (facade_->step() > 0) {
			cout << c << setw(w) << "Total Steps" <<": " << facade_->step()+1 << endl;
		}
		cout << c << setw(w) << "Time" << ": " << fixed << setprecision(3) << timer_[0].elapsed() << endl;
		cout << c << setw(w) << "  Prepare" << ": " << fixed << setprecision(3) << timer_[ClaspFacade::state_read].elapsed() << endl;
		cout << c << setw(w) << "  Prepro." << ": " << fixed << setprecision(3) << timer_[ClaspFacade::state_preprocess].elapsed() << endl;
		cout << c << setw(w) << "  Solving" << ": " << fixed << setprecision(3) << timer_[ClaspFacade::state_solve].elapsed() << endl;
	}
	if (cmdOpts_.basic.stats) { 
		out_->printStats(s.stats, en); 
	}	
}
#undef STATUS
#endif
}
