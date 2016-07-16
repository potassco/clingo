//
// Copyright (c) 2006-2016, Benjamin Kaufmann
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
#ifndef CLASP_CLASP_FACADE_H_INCLUDED
#define CLASP_CLASP_FACADE_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

#if !defined(CLASP_VERSION)
#define CLASP_VERSION "3.2.0-RC-R52432"
#endif
#if !defined(CLASP_LEGAL)
#define CLASP_LEGAL \
"Copyright (C) Benjamin Kaufmann\n"\
"License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n"\
"clasp is free software: you are free to change and redistribute it.\n"\
"There is NO WARRANTY, to the extent permitted by law."
#endif

#include <clasp/program_builder.h>
#include <clasp/parser.h>
#include <clasp/logic_program.h>
#include <clasp/enumerator.h>
#include <clasp/solver_types.h>

#if WITH_THREADS
#include <clasp/parallel_solve.h>
namespace Clasp {
	struct SolveOptions : Clasp::mt::ParallelSolveOptions, EnumOptions {};
}
#else
#include <clasp/shared_context.h>
#include <clasp/solve_algorithms.h>
namespace Clasp {
	struct SolveOptions : Clasp::BasicSolveOptions, EnumOptions {};
}
#endif

/*!
 * \file
 * This file provides a facade around the clasp library.
 * I.e. a simplified interface for (multishot) solving a problem using
 * some configuration (set of parameters).
 */
namespace Clasp {
/////////////////////////////////////////////////////////////////////////////////////////
// Configuration
/////////////////////////////////////////////////////////////////////////////////////////	
//! Configuration object for configuring solving via the ClaspFacade.
class ClaspConfig : public BasicSatConfig {
public:
	class Configurator {
	public:
		virtual ~Configurator();
		virtual void prepare(SharedContext&);
		virtual bool addPost(Solver& s) = 0;
	};
	typedef BasicSatConfig UserConfig;
	typedef Solver**       SolverIt;
	typedef Asp::LogicProgram::AspOptions AspOptions;	
	ClaspConfig();
	~ClaspConfig();
	// Base interface
	void           prepare(SharedContext&);
	void           reset();
	Configuration* config(const char*);
	//! Adds an unfounded set checker to the given solver if necessary.
	/*!
	 * If asp.suppMod is false and the problem in s is a non-tight asp-problem,
	 * the function adds an unfounded set checker to s.
	 */
	bool           addPost(Solver& s) const;
	// own interface
	UserConfig*    testerConfig() const { return tester_; }
	UserConfig*    addTesterConfig();
	//! Registers c as additional callback for when addPost() is called.
	/*! 
	 * \param ownership if Ownership_t::Acquire, ownership of c is transferred to the configuration object.
	 * \param once      whether c should be called once in the first step or also in each subsequent step.
	 */ 
	void           addConfigurator(Configurator* c, Ownership_t::Type ownership = Ownership_t::Retain, bool once = true);
	SolveOptions   solve; /*!< Options for solve algorithm and enumerator. */
	AspOptions     asp;   /*!< Options for asp preprocessing.      */
	ParserOptions  parse; /*!< Options for input parser. */
private:
	struct Impl;
	ClaspConfig(const ClaspConfig&);
	ClaspConfig& operator=(const ClaspConfig&);
	UserConfig* tester_;
	Impl*       impl_;
};
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspFacade
/////////////////////////////////////////////////////////////////////////////////////////
//! Result of a solving step.
struct SolveResult {
	//! Possible solving results.
	enum Base {
		UNKNOWN  = 0, /**< Satisfiability unknown - a given solve limit was hit.  */
		SAT      = 1, /**< Problem is satisfiable (a model was found).            */
		UNSAT    = 2, /**< Problem is unsatisfiable.                              */
	};
	enum Ext {
		EXT_EXHAUST  = 4, /**< Search space is exhausted.            */
		EXT_INTERRUPT= 8, /**< The run was interrupted from outside. */
	};
	bool sat()        const { return *this == SAT; }
	bool unsat()      const { return *this == UNSAT; }
	bool unknown()    const { return *this == UNKNOWN; }
	bool exhausted()  const { return (flags & EXT_EXHAUST)   != 0; }
	bool interrupted()const { return (flags & EXT_INTERRUPT) != 0; }
	operator Base()   const { return static_cast<Base>(flags & 3u);}
	operator double() const { return (double(signal)*256.0) + flags; }
	uint8 flags;  // result flags
	uint8 signal; // term signal or 0
};

//! Provides a simplified interface to the services of the clasp library.
class ClaspFacade : public ModelHandler {
	struct SolveData;
	struct SolveStrategy;
public:
	typedef SolveResult Result;
	typedef Potassco::AbstractStatistics AbstractStatistics;
	//! Type summarizing one or more solving steps.
	struct Summary {
		typedef const ClaspFacade* FacadePtr;
		void init(ClaspFacade& f);
		// Problem (stats) - not accumulated.
		const Asp::LpStats*  lpStep()       const;
		const Asp::LpStats*  lpStats()      const;
		const SharedContext& ctx()          const { return facade->ctx; }
		// Solve result - not accumulated.
		bool                 sat()          const { return result.sat(); }
		bool                 unsat()        const { return result.unsat(); }
		bool                 complete()     const { return result.exhausted(); }
		bool                 optimum()      const { return costs() && (complete() || model()->opt); }
		// Model enumeration - not accumulated.
		const Model*         model()        const;
		const char*          consequences() const; /**< Cautious/brave reasoning active? */
		bool                 optimize()     const; /**< Optimization active? */
		const SumVec*        costs()        const; /**< Models have associated costs? */
		uint64               optimal()      const; /**< Number of optimal models found. */
		bool                 hasLower()     const;
		SumVec               lower()        const;
		// Statistics - possibly accumulated.
		void accept(StatsVisitor& out) const;
		FacadePtr facade;    /**< Facade object of this run.          */
		double    totalTime; /**< Total wall clock time.              */
		double    cpuTime;   /**< Total cpu time.                     */
		double    solveTime; /**< Wall clock time for solving.        */
		double    unsatTime; /**< Wall clock time to prove unsat.     */
		double    satTime;   /**< Wall clock time to first model.     */
		uint64    numEnum;   /**< Total models enumerated.            */
		uint64    numOptimal;/**< Optimal models enumerated.          */
		uint32    step;      /**< Step number (multishot solving).    */
		Result    result;    /**< Result of step.                     */
	};
	ClaspFacade();
	~ClaspFacade();
	
	//! Tries to detect the problem type from the given input stream.
	static ProblemType detectProblemType(std::istream& str);

	/*!
	 * \name Query functions
	 * Functions for checking the state of this object.
	 */
	//@{
	//! Returns whether the problem is still valid.
	bool               ok()                  const { return program() ? program()->ok() : ctx.ok(); }
	//! Returns whether the active step is ready for solving.
	bool               prepared()            const;
	//! Returns whether the active step is currently being solved.
	bool               solving()             const;
	//! Returns whether the active step has been solved, i.e., has a result.
	bool               solved()              const;
	//! Returns whether solving of the active step was interrupted.
	bool               interrupted()         const;
	//! Returns the summary of the active step.
	const Summary&     summary()             const { return step_; }
	//! Returns the summary of the active (accu = false) or all steps.
	const Summary&     summary(bool accu)    const;
	//! Returns solving statistics or throws std::logic_error if step is not yet solved.
	AbstractStatistics*getStats()            const;
	//! Returns the active configuration.
	const ClaspConfig* config()              const { return config_;}
	//! Returns the current solving step (starts at 0).
	int                step()                const { return (int)step_.step;}
	//! Returns the result of the active step (unknown if run is not yet completed).
	Result             result()              const { return step_.result; }
	//! Returns the active program or 0 if it was already released.
	ProgramBuilder*    program()             const { return builder_.get(); }
	//! Returns whether program updates are enabled.
	bool               incremental()         const;
	//! Returns the active enumerator or 0 if there is none.
	Enumerator*        enumerator()          const;
	//@}
	
	//! Event type used to signal that a new step has started.
	struct StepStart : Event_t<StepStart> {
		explicit StepStart(const ClaspFacade& f) : Event_t<StepStart>(subsystem_facade, verbosity_quiet), facade(&f) {}
		const ClaspFacade* facade;
	};
	//! Event type used to signal that a solve step has terminated.
	struct StepReady : Event_t<StepReady> {
		explicit StepReady(const Summary& x) : Event_t<StepReady>(subsystem_facade, verbosity_quiet), summary(&x) {}
		const Summary* summary;
	};

	/*!
	 * \name Start functions
	 * Functions for defining a problem.
	 * Calling one of the start functions discards any previous problem
	 * and emits a StepStart event.
	 */
	//@{
	SharedContext ctx; /*!< Context-object used to store problem. */

	//! Starts definition of an ASP-problem.
	Asp::LogicProgram& startAsp(ClaspConfig& config, bool enableProgramUpdates = false);
	//! Starts definition of a SAT-problem.
	SatBuilder&        startSat(ClaspConfig& config);
	//! Starts definition of a PB-problem.
	PBBuilder&         startPB(ClaspConfig& config);
	//! Starts definition of a problem of type t.
	ProgramBuilder&    start(ClaspConfig& config, ProblemType t);
	//! Starts definition of a problem given in stream.
	ProgramBuilder&    start(ClaspConfig& config, std::istream& stream);
	//! Enables support for program updates if supported by the program.
	/*!
	 * \pre program() != 0 and not prepared().
	 * \return true if program updates are supported. Otherwise, false.
	 */
	bool               enableProgramUpdates();
	//! Enables support for (asynchronous) solve interrupts.
	void               enableSolveInterrupts();

	//@}
	enum EnumMode { enum_volatile, enum_static };
	
	//! Tries to read the next program part from the stream passed to start().
	/*!
	 * \return false if nothing was read because the stream is exhausted, solving was interrupted,
	 * or the problem is unconditionally unsat.
	 */
	bool               read();
	
	//! Finishes the definition of a problem and prepares it for solving.
	/*!
	 * \pre !solving()
	 * \post prepared() || !ok()
	 * \param m Mode to be used for handling enumeration-related knowledge.
	 *          If m is enum_volatile, enumeration knowledge is learnt under an
	 *          assumption that is retracted on program update. Otherwise,
	 *          no special assumption is used and enumeration-related knowledge
	 *          might become unretractable.
	 * \note If solved() is true, prepare() first starts a new solving step.
	 */
	void               prepare(EnumMode m = enum_volatile);
	
	//! Solves the current problem.
	/*!
	 * If prepared() is false, the function first calls prepare() to prepare the problem for solving.
	 * \pre !solving()
	 * \post solved()
	 * \param eh An optional event handler that is notified on each model and
	 *           once the solve operation has completed.
	 * \param a A list of unit-assumptions under which solving should operate.
	 */
	Result             solve(EventHandler* eh = 0, const LitVec& a = LitVec());

	class ModelGenerator {
	public:
		explicit ModelGenerator(SolveStrategy& impl);
		ModelGenerator(const ModelGenerator&);
		~ModelGenerator();
		bool         next()   const;
		const Model& model()  const;
		void         stop()   const;
		Result       result() const;
	private:
		ModelGenerator& operator=(const ModelGenerator&);
		SolveStrategy* impl_;
	};
	
	//! Starts solving of the current problem signaling models via the returned generator object.
	/*!
	* Instead of signaling results via a callback, the returned generator object
	* can be used to query models one by one.
	* \pre !solving()
	* \note It is the caller's responsibility to finish the solve operation,
	* either by extracting models until ModelGenerator::next() returns false, or
	* by calling ModelGenerator::stop().
	*/
	ModelGenerator     startSolve(const LitVec& a = LitVec());

#if WITH_THREADS
	class  AsyncResult;
	struct AsyncSolve;
	//! Asynchronously solves the current problem.
	/*!
	 * \see solve(EventHandler* eh, const LitVec&);
	 * \note The optional event handler is notified in the context of the
	 *       asynchronous operation.
	 */
	AsyncResult        solveAsync(EventHandler* eh = 0, const LitVec& a = LitVec());

	//! Asynchronously solves the current problem signaling models one by one.
	/*!
	 * The function behaves similar to solveAsync() but allows to get models
	 * one by one via the returned result object.
	 * \pre !solving()
	 *
	 * To iterate over models use a loop like:
	 * \code
	 * for (AsyncResult it = facade.startSolveAsync(); !it.end(); it.next()) {
	 *   printModel(it.model());
	 * }
	 * \endcode
	 *
	 * \note It is the caller's responsibility to finish the solve operation,
	 * either by extracting models until AsyncResult::end() returns true, or
	 * by calling AsyncResult::cancel().
	 */
	AsyncResult        startSolveAsync(const LitVec& a = LitVec());

	//! A type for accessing the result(s) of an asynchronous solve operation.
	class AsyncResult {
	public:
		typedef  StepReady Ready;
		typedef const Model& ModelRef;
		explicit AsyncResult(SolveData& x);
		~AsyncResult();
		AsyncResult(const AsyncResult&);
		AsyncResult& operator=(AsyncResult temp)            { swap(*this, temp); return *this; }
		friend void swap(AsyncResult& lhs, AsyncResult& rhs){ std::swap(lhs.state_, rhs.state_); }
		/*!
		 * \name Blocking operations
		 */
		//@{
		//! Waits until a result is ready and returns it.
		Result   get()              const;
		//! Waits until a result is ready.
		void     wait()             const;
		//! Waits for a result but for at most sec seconds.
		bool     waitFor(double sec)const;
		//! Tries to cancel the active async operation.
		bool     cancel()           const;
		//! Waits for a result and returns whether the operation is exhausted.
		bool     end()              const;
		//@}

		/*!
		* \name Non-blocking operations
		*/
		//@{
		//! Tests whether a result is ready.
		bool     ready()            const;
		//! Returns the active result in r if it is ready.
		bool     ready(Result& r)   const;
		//! Tests whether the asynchronous operation was interrupted and if so returns the interruption signal.
		int      interrupted()      const;
		//! Tests whether a result is ready and has a stored exception.
		bool     error()            const;
		//! Tests whether the asynchronous operation is still active.
		bool     running()          const;
		//! Returns the current model provided that end() is false.
		ModelRef model()            const;
		//! Kicks off search for the next result if not end().
		bool     next()             const;
	private:
		AsyncSolve* state_;
	};
#endif
	//! Tries to interrupt the active solve operation.
	/*!
	 * The function sends the given signal to the active solve operation.
	 * If no solve operation is active (i.e. solving() is false), the signal 
	 * is queued and applied to the next solve operation.
	 *
	 * \param sig The signal to raise or 0, to re-raises a previously queued signal.
	 * \return false if no operation was interrupted, because
	 *         there is no active solve operation, 
	 *         or the operation does not support interrupts,
	 *         or sig was 0 and there was no queued signal.
	 *
	 * \see enableSolveInterrupts()
	 */
	bool               interrupt(int sig);
	
	//! Forces termination of the current solving step.
	/*!
	 * \post solved()
	 * \return summary(true)
	 */
	const Summary&     shutdown();
	
	//! Starts update of the active problem.
	/*!
	 * \pre solving() is false and program updates are enabled (incremental() is true).
	 * \post !solved()
	 * \param updateConfig If true, the function applies any configuration changes.
	 * \param sigQ An action to be performed for any queued signal.
	 *        The default is to apply the signal to the next solve operation, while
	 *        SIGN_IGN can be used to discard queued signals.
	 */
	ProgramBuilder&    update(bool updateConfig, void (*sigQ)(int));
	ProgramBuilder&    update(bool updateConfig = false);
private:
	struct Statistics;
	typedef SingleOwnerPtr<ProgramBuilder> BuilderPtr;
	typedef SingleOwnerPtr<SolveData>      SolvePtr;
	typedef SingleOwnerPtr<Summary>        SummaryPtr;
	typedef SingleOwnerPtr<Statistics>     StatsPtr;
	void   init(ClaspConfig& cfg, bool discardProblem);
	void   initBuilder(ProgramBuilder* in);
	bool   isAsp() const { return program() && type_ == Problem_t::Asp; }
	void   discardProblem();
	void   startStep(uint32 num);
	Result stopStep(int signal, bool complete);
	void   updateStats();
	bool   onModel(const Solver& s, const Model& m);
	void   doUpdate(ProgramBuilder* p, bool updateConfig, void (*sig)(int));
	ProblemType  type_;
	Summary      step_;
	LitVec       assume_;
	ClaspConfig* config_;
	BuilderPtr   builder_;
	SummaryPtr   accu_;
	StatsPtr     stats_; // statistics: only if requested
	SolvePtr     solve_; // NOTE: last so that it is destroyed first;
};
}
#endif
