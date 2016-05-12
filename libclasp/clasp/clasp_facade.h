// 
// Copyright (c) 2006-2009, Benjamin Kaufmann
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
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif

#if !defined(CLASP_VERSION)
#define CLASP_VERSION "1.3.10"
#endif

#if !defined(CLASP_LEGAL)
#define CLASP_LEGAL \
"Copyright (C) Benjamin Kaufmann\n"\
"License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n"\
"clasp is free software: you are free to change and redistribute it.\n"\
"There is NO WARRANTY, to the extent permitted by law."
#endif

#include <clasp/literal.h>
#include <clasp/solve_algorithms.h>
#include <clasp/solver.h>
#include <clasp/heuristics.h>
#include <clasp/lookahead.h>
#include <clasp/program_builder.h>
#include <clasp/unfounded_check.h>
#include <clasp/reader.h>
#include <clasp/util/misc_types.h>
#include <string>

/*!
 * \file 
 * This file provides a facade around the clasp library. 
 * I.e. a simplified interface for (incrementally) solving a problem using
 * some configuration (set of parameters).
 */
namespace Clasp {

/////////////////////////////////////////////////////////////////////////////////////////
// Parameter configuration
/////////////////////////////////////////////////////////////////////////////////////////
//! Options for configuaring the ProgramBuilder
struct ApiOptions {
	ApiOptions();
	ProgramBuilder* createApi(AtomIndex& index);
	typedef ProgramBuilder::ExtendedRuleMode ExtRuleMode;
	typedef DefaultUnfoundedCheck::ReasonStrategy LoopMode;
	ExtRuleMode transExt; /**< handling of extended rules	*/
	LoopMode    loopRep;  /**< how to represent loops? */
	int  eq;              /**< iterations of equivalence preprocessing */
	bool backprop;        /**< enable backpropagation in eq-preprocessing? */
	bool eqDfs;           /**< enable df-order in eq-preprocessing? */
	bool supported;       /**< compute supported models? */
};

//! Options that control the enumeration algorithm
struct EnumerateOptions {
	EnumerateOptions();
	Enumerator* createEnumerator() const;
	typedef std::auto_ptr<std::pair<int, int> > Limits;
	bool consequences() const { return brave || cautious; }
	const char* cbType()const {
		if (consequences()) { return brave ? "Brave" : "Cautious"; }
		return "none";
	}
	int  numModels;       /**< number of models to compute */
	int  projectOpts;     /**< options for projection */
	Limits limits;        /**< search limits          */
	struct Optimize {     /**< Optimization options */
		Optimize() : no(false), all(false), heu(false) {}
		WeightVec vals;     /**< initial values for optimize statements */
		bool   no;          /**< ignore optimize statements */
		bool   all;         /**< compute all optimal models */
		bool   heu;         /**< consider optimize statements in heuristics */
	}    opt;
	bool project;         /**< enable projection */
	bool record;          /**< enable solution recording */
	bool restartOnModel;  /**< restart after each model */
	bool brave;           /**< enable brave reasoning */
	bool cautious;        /**< enable cautious reasoning */
};
//! Options that control the decision heuristic
struct HeuristicOptions {
	HeuristicOptions();
	typedef Lookahead::Type LookaheadType;
	DecisionHeuristic* createHeuristic() const;
	std::string   heuristic;   /**< decision heuristic */
	LookaheadType lookahead;   /**< type of lookahead */
	int           lookaheadNum;/**< number of times lookahead is used */
	int           loops;       /**< consider loops in heuristic (0: no, 1: yes, -1: let heuristic decide) */
	union Extra {
	int      berkMax;          /**< only for Berkmin */
	int      vmtfMtf;          /**< only for Vmtf    */
	} extra;
	bool     berkMoms;         /**< only for Berkmin */
	bool     berkHuang;	       /**< only for Berkmin */
	bool     nant;             /**< only for unit    */
};

class ClaspFacade;

//! Interface for controling incremental solving
class IncrementalControl {
public:
	IncrementalControl();
	virtual ~IncrementalControl(); 
	//! Called before an incremental step is started
	virtual void initStep(ClaspFacade& f)  = 0;
	//! Called after an incremental step finished
	/*!
	 * \return
	 *  - true to signal that solving should continue with next step
	 *  - false to terminate the incremental solving loop
	 */
	virtual bool nextStep(ClaspFacade& f)  = 0;
private:
	IncrementalControl(const IncrementalControl&);
	IncrementalControl& operator=(const IncrementalControl&);
};

//! Parameter-object that groups & validates options
class ClaspConfig {
public:
	explicit ClaspConfig(Solver* s = 0) : solver(s), onlyPre(false) {}
	bool validate(std::string& err);
	ApiOptions       api;
	EnumerateOptions enumerate;
	SolveParams      solve;
	HeuristicOptions heuristic;
	Solver*          solver;
	bool             onlyPre;    /**< stop after preprocessing step? */
private:
	ClaspConfig(const ClaspConfig&);
	ClaspConfig& operator=(const ClaspConfig&);
};
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspFacade
/////////////////////////////////////////////////////////////////////////////////////////
//! Provides a simplified interface for (incrementally) solving a given problem
class ClaspFacade : public Enumerator::Report {
public:
	//! Defines the possible solving states
	enum State { 
		state_not_started,
		state_read,        /*!< problem is read from input */
		state_preprocess,  /*!< problem is prepared */
		state_solve,       /*!< search is active */
		num_states
	};
	//! Defines important event types
	enum Event { 
		event_state_enter, /*!< a new state was entered */
		event_state_exit,  /*!< about to exit from the active state */
		event_p_prepared,  /*!< problem was transformed to nogoods */
		event_model        /*!< a model was found */
	};
	//! Defines possible solving results
	enum Result { result_unsat, result_sat, result_unknown }; 
	//! Callback interface for notifying about important steps in solve process
	class Callback {
	public:
		virtual ~Callback() {}
		//! State transition. Called on entering/exiting a state
		/*!
		 * \param e is either event_state_enter or event_state_exit
		 * \note f.state() returns the active state
		 */
		virtual void state(Event e, ClaspFacade& f) = 0;
		//! Some operation triggered an important event
		/*!
		 * \param e an event that is neither event_state_enter nor event_state_exit 
		 */
		virtual void event(Event e, ClaspFacade& f) = 0;		
		//! The solver is about to (re-)start
		/*!
		 * \note
		 *   The function is only called if e.enableProgressReport(&f) is called, where
		 *   e is the current enumerator and f is the current facade object.
		 */
		virtual void reportRestart(const Solver& /* s */, uint64 /* maxCfl */, uint32 /* maxL */) {}
		//! Some configuration option is unsafe/unreasonable w.r.t the current problem
		virtual void warning(const char* msg)       = 0;
	};
	ClaspFacade();
	/*!
	 * Solves the problem given in problem using the given configuration.
	 * \pre config is valid, i.e. config.valid() returned true
	 * \note Once solve() returned, the result of the computation can be
	 * queried via the function result().
	 * \note if config.onlyPre is true, solve() returns after
	 * the preprocessing step (i.e. once the solver is prepared) and does not start a search.
	 */
	void solve(Input& problem, ClaspConfig& config, Callback* c);

	/*!
	 * Incrementally solves the problem given in problem using the given configuration.
	 * \pre config is valid, i.e. config.valid() returned true
	 * \note Call result() to get the result of the computation
	 * \note config.onlyPre is ignored in incremental setting!
	 *
	 * solveIncremental() runs a simple loop that is controlled by the
	 * given IncrementalControl object inc.
	 * \code
	 * do {
	 *   inc.initStep(*this);
	 *   read();
	 *   preprocess();
	 *   solve();
	 * } while (inc.nextStep(*this));
	 * \endcode
	 * 
	 */
	void solveIncremental(Input& problem, ClaspConfig& config, IncrementalControl& inc, Callback* c);

	//! returns the result of a computation
	Result result() const { return result_; }
	//! returns true if search-space was completed. Otherwise false.
	bool   more()   const { return more_; }
	//! returns the active state
	State  state()  const { return state_; }
	//! returns the current incremental step (starts at 0)
	int    step()   const { return step_; }
	//! returns the current input problem
	Input* input() const { return input_; }

	const ClaspConfig* config() const { return config_; }

	//! returns the ProgramBuilder-object that was used to transform a logic program into nogoods
	/*!
	 * \note A ProgramBuilder-object is only created if input()->format() == Input::SMODELS
	 * \note The ProgramBuilder-object is destroyed after the event
	 *       event_p_prepared was fired. Call releaseApi to disable auto-deletion of api.
	 *       In that case you must later manually delete it!
	 */
	ProgramBuilder* api() const  { return api_.get();     }
	ProgramBuilder* releaseApi() { return api_.release(); }

	void warning(const char* w) const { if (cb_) cb_->warning(w); }
private:
	ClaspFacade(const ClaspFacade&);
	ClaspFacade& operator=(const ClaspFacade&);
	typedef SingleOwnerPtr<ProgramBuilder> Api;
	// -------------------------------------------------------------------------------------------  
	// Status information
	void setState(State s, Event e)   { state_ = s; if (cb_) cb_->state(e, *this); }
	void fireEvent(Event e)           { if (cb_) cb_->event(e, *this); }
	// -------------------------------------------------------------------------------------------
	// Enumerator::Report interface
	void reportModel(const Solver&, const Enumerator&) {
		result_ = result_sat;
		fireEvent(event_model);
	}
	void reportSolution(const Solver& s, const Enumerator&, bool complete) {
		more_ = !complete;
		if (!more_ && s.stats.solve.models == 0) {
			result_ = result_unsat;
		}
		setState(state_, event_state_exit);
	}
	void reportRestart(const Solver& s, uint64 maxCfl, uint32 maxL) {
		if (cb_) cb_->reportRestart(s, maxCfl, maxL);
	}
	// -------------------------------------------------------------------------------------------
	// Internal setup functions
	void   validateWeak();
	void   init(Input&, ClaspConfig&, IncrementalControl*, Callback* c);
	bool   read();
	bool   preprocess();
	uint32 computeProblemSize() const;
	bool   configureMinimize(MinimizeConstraint* min) const;
	bool   initEnumerator(MinimizeConstraint* min)    const;
	// -------------------------------------------------------------------------------------------
	ClaspConfig*        config_;
	IncrementalControl* inc_;
	Callback*           cb_;
	Input*              input_;
	Api                 api_;
	Result              result_;
	State               state_;
	int                 step_;
	bool                more_;
};

}
#endif
