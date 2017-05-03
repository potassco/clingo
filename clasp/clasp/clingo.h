//
// Copyright (c) 2015-2017 Benjamin Kaufmann
//
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
#ifndef CLASP_CLINGO_H_INCLUDED
#define CLASP_CLINGO_H_INCLUDED
/*!
 * \file
 * \brief Types for implementing theory propagation from clingo.
 */
#include <potassco/clingo.h>
#include <clasp/clasp_facade.h>
namespace Clasp {

/*!
 * \defgroup clingo Clingo
 * \brief Additional classes mainly used by clingo.
 * \ingroup facade
 * @{ */

//! Lock interface called by libclasp during (multi-threaded) theory propagation.
/*!
 * The interface may be implemented by the application to lock
 * certain global data structures. For example, in clingo,
 * this interface wraps python's global interpreter lock.
 */
class ClingoPropagatorLock {
public:
	virtual ~ClingoPropagatorLock();
	virtual void lock()   = 0;
	virtual void unlock() = 0;
};

//! Supported check modes for clingo propagators.
struct ClingoPropagatorCheck_t {
	enum Type {
		No       = 0u, //!< Never call AbstractPropagator::check().
		Total    = 1u, //!< Call AbstractPropagator::check() only on total assignment.
		Fixpoint = 2u  //!< Call AbstractPropagator::check() on every propagation fixpoint.
	};
};

//! Initialization adaptor for a Potassco::AbstractPropagator.
/*!
* The class provides a function for registering watches for the propagator.
* Furthermore, it can be added to a clasp configuration so that
* a (suitably adapted) propagator is added to solvers that are attached to the configuration.
*/
class ClingoPropagatorInit : public ClaspConfig::Configurator {
public:
	typedef ClingoPropagatorCheck_t::Type CheckType; 
	//! Creates a new adaptor.
	/*!
	* \param cb The (theory) propagator that should be added to solvers.
	* \param lock An optional lock that should be applied during theory propagation.
	*
	* If lock is not null, calls to cb are wrapped in a lock->lock()/lock->unlock() pair
	*/
	ClingoPropagatorInit(Potassco::AbstractPropagator& cb, ClingoPropagatorLock* lock = 0, CheckType check = ClingoPropagatorCheck_t::Total);
	~ClingoPropagatorInit();
	// base class
	virtual void prepare(SharedContext&);
	//! Adds a ClingoPropagator adapting the propagator() to s.
	virtual bool addPost(Solver& s);

	// for clingo
	//! Sets the type of checks to enable during solving.
	/*!
	* \param checkMode A set of ClingoPropagatorCheck_t::Type values.
	*/
	void enableClingoPropagatorCheck(CheckType checkMode);
	//! Registers a watch for lit and returns encodeLit(lit).
	Potassco::Lit_t addWatch(Literal lit);

	//! Returns the propagator that was given on construction.
	Potassco::AbstractPropagator* propagator() const { return prop_; }
	ClingoPropagatorLock*         lock()       const { return lock_; }
	const LitVec&                 watches()    const { return watches_; }
	CheckType                     checkMode()  const { return check_; }
private:
	ClingoPropagatorInit(const ClingoPropagatorInit&);
	ClingoPropagatorInit& operator=(const ClingoPropagatorInit&);
	Potassco::AbstractPropagator* prop_;
	ClingoPropagatorLock* lock_;
	LitVec    watches_;
	VarVec    seen_;
	CheckType check_;
};

//! Adaptor for a Potassco::AbstractPropagator.
/*!
 * The class adapts a given Potassco::AbstractPropagator so that
 * it is usable as a PostPropagator within libclasp.
 * \note This class is meant to be a final class.
 */
class ClingoPropagator : public Clasp::PostPropagator {
public:
	typedef Potassco::AbstractPropagator::ChangeList ChangeList;
	typedef Clasp::PostPropagator::PropResult PPair;

	explicit ClingoPropagator(ClingoPropagatorInit* init);

	// PostPropagator
	virtual uint32 priority() const;
	virtual bool   init(Solver& s);
	virtual bool   propagateFixpoint(Clasp::Solver& s, Clasp::PostPropagator* ctx);
	virtual PPair  propagate(Solver&, Literal, uint32&);
	virtual bool   isModel(Solver& s);
	virtual void   reason(Solver&, Literal, LitVec&);
	virtual void   undoLevel(Solver& s);
	virtual bool   simplify(Solver& s, bool reinit);
	virtual void   destroy(Solver* s, bool detach);
private:
	typedef LitVec::size_type size_t;
	typedef Potassco::Lit_t Lit_t;
	class Control;
	enum State { state_ctrl = 1u, state_prop = 2u };
	struct ClauseTodo {
		bool empty() const { return mem.empty(); }
		void clear()       { mem.clear(); }
		LitVec    mem;
		ClauseRep clause;
		uint32    flags;
	};
	typedef PodVector<Lit_t>::type        TrailVec;
	typedef PodVector<Constraint*>::type  ClauseDB;
	typedef ClingoPropagatorInit          Propagator;
	typedef ClingoPropagatorLock*         ClingoLock;
	typedef const LitVec&                 Watches;
	bool addClause(Solver& s, uint32 state);
	void toClause(Solver& s, const Potassco::LitSpan& clause, Potassco::Clause_t prop);
	void registerUndo(Solver& s);
	Propagator* call_;  // wrapped theory propagator
	TrailVec    trail_; // assignment trail: watched literals that are true
	VarVec      undo_;  // offsets into trail marking beginnings of decision levels
	ClauseDB    db_;    // clauses added with flag static
	ClauseTodo  todo_;  // active clause to be added (received from theory propagator)
	size_t      init_;  // offset into watches separating old and newly added ones
	size_t      prop_;  // offset into trail: literals [0, prop_) were propagated
	size_t      epoch_; // number of calls into callback
	uint32      level_; // highest undo level
	int32       front_; // global assignment position for fixpoint checks
	Literal     aux_;   // max active literal
};
///@}
}
#endif
