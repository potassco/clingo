//
// Copyright (c) 2015-2016, Benjamin Kaufmann
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
	
	/*!
	 * If lock is not null, calls to cb are wrapped in a lock->lock()/lock->unlock() pair
	 */
	ClingoPropagator(const LitVec& watches, Potassco::AbstractPropagator& cb, ClingoPropagatorLock* lock = 0);

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
	typedef PodVector<Lit_t>::type        TrailVec;
	typedef PodVector<Constraint*>::type  ClauseDB;
	typedef Potassco::AbstractPropagator* Callback;
	typedef ClingoPropagatorLock*         ClingoLock;
	typedef const LitVec&                 Watches;
	bool addClause(Solver& s, uint32 state);
	void toClause(Solver& s, const Potassco::LitSpan& clause, Potassco::Clause_t prop);
	Watches    watches_; // set of watched literals
	Callback   call_;    // actual theory propagator
	ClingoLock lock_;    // optional lock for protecting calls to theory propagator
	TrailVec   trail_;   // assignment trail: watched literals that are true
	VarVec     undo_;    // offsets into trail marking beginnings of decision levels
	LitVec     clause_;  // active clause to be added (received from theory propagator)
	ClauseDB   db_;      // clauses added with flag static
	size_t     init_;    // offset into watches separating old and newly added ones
	size_t     level_;   // highest decision level in trail
	size_t     prop_;    // offset into trail: literals [0, prop_) were propagated
	size_t     epoch_;   // number of calls into callback
};

//! Initialization adaptor for a Potassco::AbstractPropagator.
/*!
 * The class provides a function for registering watches for the propagator.
 * Furthermore, it can be added to a clasp configuration so that 
 * a (suitably adapted) propagator is added to solvers that are attached to the configuration.
 */
class ClingoPropagatorInit : public ClaspConfig::Configurator {
public:
	//! Creates a new adaptor.
	/*!
	 * \param cb The (theory) propagator that should be added to solvers.
	 * \param lock An optional lock that should be applied during theory propagation.
	 */
	ClingoPropagatorInit(Potassco::AbstractPropagator& cb, ClingoPropagatorLock* lock = 0);
	~ClingoPropagatorInit();
	// base class
	virtual void prepare(SharedContext&);
	//! Adds a ClingoPropagator adapting the propagator() to s.
	virtual bool addPost(Solver& s);

	// for clingo
	//! Registers a watch for lit and returns encodeLit(lit).
	Potassco::Lit_t addWatch(Literal lit);

	//! Returns the propagator that was given on construction.
	Potassco::AbstractPropagator* propagator() const { return prop_; }
private:
	ClingoPropagatorInit(const ClingoPropagatorInit&);
	ClingoPropagatorInit& operator=(const ClingoPropagatorInit&);
	Potassco::AbstractPropagator* prop_;
	ClingoPropagatorLock* lock_;
	LitVec watches_;
	VarVec seen_;
};
///@}
}
#endif
