// 
// Copyright (c) 2009-2013, Benjamin Kaufmann
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
#ifndef CLASP_CLI_OUTPUT_H_INCLUDED
#define CLASP_CLI_OUTPUT_H_INCLUDED
#include <clasp/clasp_facade.h>
#include <clasp/dependency_graph.h>
#include <clasp/solver_types.h>
#include <string>
namespace Clasp { namespace Cli {
void format(const Clasp::BasicSolveEvent& ev, char* out, uint32 outSize);
void format(const Clasp::SolveTestEvent&  ev, char* out, uint32 outSize);
#if WITH_THREADS
void format(const Clasp::mt::MessageEvent& ev, char* out, uint32 outSize);
#endif

/*!
 * \addtogroup cli
 */
//@{

/*!
 *
 * Interface for printing status and input format dependent information,
 * like models, optimization values, and summaries.
 */
class Output : public EventHandler {
public:
	//! Supported levels for printing models, optimize values, and individual calls.
	enum PrintLevel { 
		print_all  = 0, /**< Print all models, optimize values, or calls.       */
		print_best = 1, /**< Only print last model, optimize value, or call.    */
		print_no   = 2, /**< Do not print any models, optimize values, or calls.*/
	};
	explicit Output(uint32 verb = 1);
	virtual ~Output();
	//! Active verbosity level.
	uint32 verbosity()               const { return verbose_; }
	//! Do not output any models?
	bool   quiet()                   const { return modelQ() == 2 && optQ() == 2; }
	//! Print level for models.
	int    modelQ()                  const { return static_cast<int>(quiet_[0]); }
	//! Print level for optimization values.
	int    optQ()                    const { return static_cast<int>(quiet_[1]); }
	//! Print level for individual (solve) calls.
	int    callQ()                   const { return static_cast<int>(quiet_[2]); }
	
	using  EventHandler::setVerbosity;
	void   setVerbosity(uint32 verb);
	void   setModelQuiet(PrintLevel model);
	void   setOptQuiet(PrintLevel opt);
	void   setCallQuiet(PrintLevel call);
	void   setHide(char c);

	//! Shall be called once on startup.
	virtual void run(const char* solver, const char* version, const std::string* begInput, const std::string* endInput) = 0;
	//! Shall be called once on shutdown.
	virtual void shutdown(const ClaspFacade::Summary& summary);
	virtual void shutdown() = 0;
	//! Handles ClaspFacade events by forwarding calls to startStep() and stopStep().
	virtual void onEvent(const Event& ev);
	//! Checks quiet-levels and forwards to printModel() if appropriate.
	virtual bool onModel(const Solver& s, const Model& m);	
	//! Shall print the given model.
	virtual void printModel(const SymbolTable& sym, const Model& m, PrintLevel x) = 0;
	//! A solving step has started.
	virtual void startStep(const ClaspFacade&);
	//! A solving step has stopped.
	virtual void stopStep(const ClaspFacade::Summary& summary);
	//! Shall print the given summary.
	virtual void printSummary(const ClaspFacade::Summary& summary, bool final)    = 0;
	//! Shall print the given statistics.
	virtual void printStatistics(const ClaspFacade::Summary& summary, bool final) = 0;
protected:
	const Model* getModel() const { return saved_.values ? &saved_ : 0; }
	void         saveModel(const Model& m);
	void         clearModel() { saved_.values = 0; }
	bool         doPrint(const SymbolTable::symbol_type& sym) const {
		return !sym.name.empty() && *sym.name.c_str() != hidePref_;
	}
private:
	Output(const Output&);
	Output& operator=(const Output&);
	typedef const ClaspFacade::Summary* SumPtr ;
	SumPtr    summary_ ; // summary of last step
	ValueVec  vals_    ; // saved values from most recent model
	SumVec    costs_   ; // saved costs from most recent model
	Model     saved_   ; // most recent model
	uint32    verbose_ ; // verbosity level
	uint8     quiet_[3]; // quiet levels for models, optimize, calls
	char      hidePref_; // hide printing of symbols starting with this char
};

//! Interface for printing statistics.
class StatsVisitor {
public:
	virtual ~StatsVisitor();
	virtual void visitStats(const SharedContext& ctx, const Asp::LpStats* lp, bool accu);
	// compound
	virtual void visitSolverStats(const SolverStats& stats, bool accu);
	virtual void visitProblemStats(const ProblemStats& stats, const Asp::LpStats* lp);
	virtual void visitThreads(const SharedContext& ctx);
	virtual void visitHccs(const SharedContext& ctx);
	virtual void visitThread(uint32, const SolverStats& stats) { visitSolverStats(stats, false); }
	virtual void visitHcc(uint32, const SharedContext& stats);
	// leafs
	virtual void visitLogicProgramStats(const Asp::LpStats& stats) = 0;
	virtual void visitProblemStats(const ProblemStats& stats) = 0;
	virtual void visitCoreSolverStats(double cpuTime, uint64 models, const SolverStats& stats, bool accu) = 0;
	virtual void visitExtSolverStats(const ExtendedStats& stats, bool accu) = 0;
	virtual void visitJumpStats(const JumpStats& stats, bool accu) = 0;
	virtual void accuStats(const SharedContext& ctx, SolverStats& out) const;

	bool accu;
};

//! Prints models and solving statistics in Json-format to stdout.
class JsonOutput : public Output, private StatsVisitor {
public:
	explicit JsonOutput(uint32 verb);
	~JsonOutput();
	virtual void run(const char* solver, const char* version, const std::string* begInput, const std::string* endInput);
	virtual void shutdown(const ClaspFacade::Summary& summary);
	virtual void shutdown();
	virtual void printSummary(const ClaspFacade::Summary&    summary, bool final);
	virtual void printStatistics(const ClaspFacade::Summary& summary, bool final);
private:
	virtual void startStep(const ClaspFacade&);
	virtual void stopStep(const ClaspFacade::Summary& summary);
	virtual void visitThreads(const SharedContext& ctx)         { pushObject("Thread", type_array); StatsVisitor::visitThreads(ctx);    popObject(); }
	virtual void visitHccs(const SharedContext& ctx)            { pushObject("HCC", type_array);    StatsVisitor::visitHccs(ctx);       popObject(); }
	virtual void visitThread(uint32 i, const SolverStats& stats){ pushObject(0, type_object);       StatsVisitor::visitThread(i, stats);popObject(); }
	virtual void visitHcc(uint32 i, const SharedContext& ctx)   { pushObject(0, type_object);       StatsVisitor::visitHcc(i, ctx);     popObject(); }
	virtual void visitLogicProgramStats(const Asp::LpStats& stats);
	virtual void visitProblemStats(const ProblemStats& stats);
	virtual void visitCoreSolverStats(double cpuTime, uint64 models, const SolverStats& stats, bool accu);
	virtual void visitExtSolverStats(const ExtendedStats& stats, bool accu);
	virtual void visitJumpStats(const JumpStats& stats, bool accu);
	enum ObjType { type_object, type_array };
	void pushObject(const char* k = 0, ObjType t = type_object);
	char popObject();
	void printKeyValue(const char* k, const char* v) ;
	void printKeyValue(const char* k, uint64 v);
	void printKeyValue(const char* k, uint32 v);
	void printKeyValue(const char* k, double d);
	void printString(const char* s, const char* sep);
	void printKey(const char* k);
	void printModel(const SymbolTable& sym, const Model& m, PrintLevel x);
	void printCosts(const SumVec& costs);
	void startModel();
	bool hasWitness() const { return !objStack_.empty() && *objStack_.rbegin() == '['; }
	uint32 indent()   const { return static_cast<uint32>(objStack_.size() * 2); }
	const char* open_;
	std::string objStack_;
};

//! Default clasp format printer.
/*!
 * Prints all output to stdout in given format:
 * - format_asp prints in clasp's default asp format
 * - format_aspcomp prints in in ASP competition format
 * - format_sat09 prints in SAT-competition format
 * - format_pb09 in PB-competition format
 * .
 * \see https://www.mat.unical.it/aspcomp2013/
 * \see http://www.satcompetition.org/2009/format-solvers2009.html
 * \see http://www.cril.univ-artois.fr/PB09/solver_req.html
 *
 */
class TextOutput : public Output, private StatsVisitor {
public:
	enum Format      { format_asp, format_aspcomp, format_sat09, format_pb09 };
	enum ResultStr   { res_unknonw = 0, res_sat = 1, res_unsat = 2, res_opt = 3, num_str };
	enum CategoryKey { cat_comment, cat_value, cat_objective, cat_result, cat_value_term, cat_atom, num_cat };
	
	const char* result[num_str]; /**< Default result strings. */
	const char* format[num_cat]; /**< Format strings.         */

	TextOutput(uint32 verbosity, Format f, const char* catAtom = 0, char ifs = ' ');
	~TextOutput();

	//! Prints a (comment) message containing the given solver and input.
	virtual void run(const char* solver, const char* version, const std::string* begInput, const std::string* endInput);
	virtual void shutdown();
	//! Prints the given model.
	/*!
	 * Prints format[cat_value] followed by the elements of the model. Individual 
	 * elements e are printed as format[cat_atom] and separated by the internal field separator.
	 */
	virtual void printModel(const SymbolTable& sym, const Model& m, PrintLevel x);
	//! Called once a solving step has completed.
	/*!
	 * Always prints "format[cat_result] result[s.result()]".
	 * Furthermore, if verbosity() > 0, prints a summary consisting of
	 *   - the number of computed models m and whether the search space was exhausted
	 *   - the number of enumerated models e if e != m
	 *   - the state of any optimization and whether the last model was optimal
	 *   - the state of consequence computation and whether the last model corresponded to the consequences
	 *   - timing information
	 *   .
	 */
	virtual void printSummary(const ClaspFacade::Summary& s   , bool final);
	virtual void printStatistics(const ClaspFacade::Summary& s, bool final);
	//! Prints progress events (preprocessing/solving) if verbosity() > 1.
	virtual void onEvent(const Event& ev);
	//! A solving step has started.
	virtual void startStep(const ClaspFacade&);
	//! Prints a comment message.
	void comment(uint32 v, const char* fmt, ...) const;
protected:
	virtual void printNames(const SymbolTable& sym, const Model& m);
	virtual void visitProblemStats(const ProblemStats& stats, const Asp::LpStats* lp);
	virtual void visitSolverStats(const Clasp::SolverStats& s, bool accu);
	virtual void visitLogicProgramStats(const Asp::LpStats& stats);
	virtual void visitProblemStats(const ProblemStats& stats);
	virtual void visitCoreSolverStats(double cpuTime, uint64 models, const SolverStats& stats, bool accu);
	virtual void visitExtSolverStats(const ExtendedStats& stats, bool accu);
	virtual void visitJumpStats(const JumpStats& stats, bool accu);
	virtual void visitThreads(const SharedContext& ctx)      { startSection("Thread");   StatsVisitor::visitThreads(ctx); }
	virtual void visitThread(uint32 i, const SolverStats& s)  { startObject("Thread", i); StatsVisitor::visitThread(i, s); }
	virtual void visitHccs(const SharedContext& ctx)         { startSection("Tester");   StatsVisitor::visitHccs(ctx);    }
	virtual void visitHcc(uint32 i, const SharedContext& ctx){ startObject("HCC", i);    StatsVisitor::visitHcc(i, ctx);  }
	
	const char* fieldSeparator() const;
	int         printSep(CategoryKey c) const;
	void        printCosts(const SumVec&) const;
	void        startSection(const char* n)     const;
	void        startObject(const char* n, uint32 i) const;
	void        setState(uint32 state, uint32 verb, const char* st);
	void        printSolveProgress(const Event& ev);
private:
	double             stTime_;// time on state enter
	Clasp::atomic<int> ev_;    // last event type
	int                width_; // output width
	int                line_;  // lines to print until next separator
	uint32             state_; // active state
	char               ifs_[2];// field separator
};
//@}

}}
#endif
