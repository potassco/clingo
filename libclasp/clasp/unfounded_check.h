// 
// Copyright (c) 2006-2007, Benjamin Kaufmann
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

#ifndef CLASP_UNFOUNDED_CHECK_H_INCLUDED
#define CLASP_UNFOUNDED_CHECK_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif
#include <clasp/solver.h>
#include <clasp/literal.h> 
#include <clasp/program_builder.h>
#include <clasp/constraint.h>
#include <deque>
namespace Clasp {
class LoopFormula;


//! Clasp's default unfounded set checker.
/*!
 * \ingroup solver
 * Searches for unfounded atoms by checking the positive dependency graph (PDG)
 * Basic Idea:
 *  - For each (non-false) atom a, let source(a) be a body B in body(a) that provides an external support for a
 *    - If no such B exists, a must be false
 *  - If source(a) becomes false and a is not false:
 *    - Let Q = {};
 *    - add a to Q
 *    - For each B' s.th B' is not external to Q
 *      - add { a' | source(a') = B } to Q
 *  - Try to find new sources for all atoms a in Q
 */
class DefaultUnfoundedCheck : public PostPropagator {
public:
	//! Defines the supported reasons for explaining assignments
	enum ReasonStrategy {
		common_reason,    /*!< one reason for each unfounded set but one clause for each atom */
		distinct_reason,  /*!< distinct reason and clause for each unfounded atom */
		shared_reason,    /*!< one shared loop formula for each unfounded set */
		only_reason,      /*!< store only the reason but don't learn a nogood */
	};
	
	DefaultUnfoundedCheck(ReasonStrategy r = common_reason);
	~DefaultUnfoundedCheck();

	ReasonStrategy reasonStrategy() const {
		return strategy_;
	}
	void clear();
	
	//! must be called once before endInit() is called to add strongly connected atoms
	void startInit(Solver& s);
	
	//! initialize all SCCs
	/*!
	 * \param sccAtoms  Atoms of the logic program that are strongly connected.
	                    Must be a subset of prgAtoms; preferably ordered by SCC
	 * \param prgAtoms  All atoms of the logic program
	 * \param prgBodies All bodies of the logic program
	 */
	bool endInit(const AtomList& sccAtoms, const AtomList& prgAtoms, const BodyList& prgBodies);

	//! number of nodes in the positive dependency graph
	LitVec::size_type nodes() const { return atoms_.size() + bodies_.size(); }
	
	const Solver& solver() const { return *solver_; }
	
	// base interface
	void   reset();
	bool   propagateFixpoint(Solver& s);
	bool   propagate(Solver& s) { return DefaultUnfoundedCheck::propagateFixpoint(s); }
	uint32 priority() const     { return uint32(priority_single_high); }
	
	// public - so that tests can access the data-structures
	struct UfsNode;
	struct UfsAtomNode;
	class  UfsBodyNode;
	typedef PodVector<UfsNode*>::type NodeVec;
	typedef PodVector<UfsAtomNode*>::type AtomVec;
	typedef PodVector<UfsBodyNode*>::type BodyVec;
	
	//! Base class for nodes in the PDG.
	struct UfsNode {
		UfsNode(Literal a_lit, uint32 a_scc) 
			: lit(a_lit), scc(a_scc)
			, pickedOrTodo(0), typeOrUnfounded(0) { }
		Literal lit;                  /*!< literal in the solver */
		uint32  scc             : 30; /*!< component number */
		uint32  pickedOrTodo    : 1;  /*!< in picked (body) resp. todo (atom) queue */
		uint32  typeOrUnfounded : 1;  /*!< in unfounded (atom) queue resp. type of body (normal/extended) */
	};
	
	//! An atom in the PDG
	struct UfsAtomNode : public UfsNode {
		UfsAtomNode(Literal a_lit, uint32 a_scc) : UfsNode(a_lit, a_scc), preds(0), succs(0), source_(1) {}
		~UfsAtomNode() {
			delete [] preds;
			delete [] succs;
		}
		//! returns the body that is currently watched as possible source
		/*!
		 * \note The returned body is only valid source if hasSource() returns true.
		 */
		UfsBodyNode*  watch()     const { return (UfsBodyNode*)clear_bit(source_, 0); }
		//! returns true if atom has currently a source, i.e. a body that can still define it
		bool          hasSource() const { return !test_bit(source_, 0); }  
		//! sets b as source for this atom
		void          updateSource(UfsBodyNode* b)  {
			assert(b);
			++b->watches;
			if (source_ != 1) --watch()->watches;
			source_ = (uintp)b;
		}
		void          markSourceInvalid() { store_set_bit(source_, 0); }
		void          resurrectSource()   { store_clear_bit(source_, 0); }
		UfsBodyNode** preds;  /*!< predecessors: [other scc, same scc] */
		UfsBodyNode** succs;  /*!< successors from same scc */
		//! returns true if atom is currently in todo-Queue
		bool inTodoQueue()      const { return pickedOrTodo != 0; }
		//! returns true if atom is currently in unfounded-Queue
		bool inUnfoundedQueue() const { return typeOrUnfounded != 0; }
	private:
		uintp source_;  // current source, LSB: 0: valid, 1: invalid
	};
	//! A body in the PDG
	class UfsBodyNode : public UfsNode {
		friend class DefaultUnfoundedCheck;
	public:
		UfsBodyNode(Literal a_lit, uint32 a_scc) : UfsNode(a_lit, a_scc), preds(0), succs(0), watches(0) { }
		virtual ~UfsBodyNode() { delete [] preds; delete [] succs; }
		
		UfsAtomNode** preds;    /*!< predecessors from same scc                             */
		UfsAtomNode** succs;    /*!< successors: [same scc, other scc]                      */
		uint32        watches;  /*!< number of atoms watching this body as potential source */
		//! returns true if body was already visited during one algorithm
		bool picked()   const { return pickedOrTodo != 0; }

		// Initializes the body node from the given program node
		/*!
		 * Shall return true only if body is (currently) external to all non-trivial sccs.
		 * \param prgBody   The program body corresponding to this UfsBody
		 * \param prgAtoms  The list of all atoms
		 * \param ufs       The unfounded set checker in which this body is used
		 * \param initList  The atom list that is currently initialized; either preds or succs
		 * \param listLen   The number of atoms in initList
		 */
		virtual bool init(const PrgBodyNode& prgBody, const AtomList& prgAtoms, DefaultUnfoundedCheck& ufs, UfsAtomNode** initList, uint32 listLen) = 0;
		
		//! Checks whether the body can source its heads
		/*!
		 * \return  true if the body currently can source its heads
		 */
		virtual bool isValidSource(const DefaultUnfoundedCheck& ufs)  = 0;

		//! Notifies the body about the fact that its subgoal a has a new source
		/*!
		 * \pre     a in B+ and scc(a) == scc(*this)
		 * \return  true if the body now can source its heads 
		 */
		virtual bool atomSourced(const UfsAtomNode& a) = 0;
	
		//! Notifies the body about the fact that its subgoal a has lost its source
		/*!
		 * \pre     a in B+ and scc(a) == scc(*this)
		 * \return  true if the body no longer can source its heads
		 */
		virtual bool atomUnsourced(const UfsAtomNode& a) = 0;

		//! Shall enqueue all predecessors of this body that currently lack a source
		virtual void enqueueUnsourced(DefaultUnfoundedCheck& ufs) = 0;
		
		/*!
		 * If this body is part of the reason for the current unfounded set stored in ufs
		 * shall call ufs->addReasonLit(l) for each false literal l that prevents the body
		 * from beeing a valid source.
		 * \pre     ufs currently stores an unfounded set
		 */
		void addIfReason(DefaultUnfoundedCheck& ufs, uint32 uScc) {
			if      (uScc == scc)               { doAddIfReason(ufs); }
			else if (ufs.solver().isFalse(lit)) { ufs.addReasonLit(lit, this); }
		}

		//! Call-back function for watches
		virtual void onWatch(DefaultUnfoundedCheck& /*ufs*/, Literal /*p*/, uint32 /*data*/) {}
	private:    
		virtual void doAddIfReason(DefaultUnfoundedCheck& ufs)              = 0;
	};
			
	//! inefficient - only used for testing
	UfsBodyNode* bodyNode(Literal b) const {
		for (BodyVec::size_type i = 0; i < bodies_.size(); ++i) {
			if (bodies_[i]->lit == b) { return bodies_[i]; }
		}
		return 0;
	}
	//! inefficient - only used for testing
	UfsAtomNode* atomNode(Literal h) const {
		for (AtomVec::size_type i = 0; i < atoms_.size(); ++i) {
			if (atoms_[i]->lit == h) { return atoms_[i]; }
		}
		return 0;
	}
	// helpers
	// The body b can no longer be used as source pointer.
	// Mark it for later source pointer removal
	void enqueueInvalid(UfsBodyNode* b) {
		invalid_.push_back(b);
	}

	// Mark atom a for later source pointer removal
	// Pre: a->hasSource() == true
	void enqueuePropagateUnsourced(UfsAtomNode* a) {
		assert(a->hasSource() == true);
		a->markSourceInvalid();
		sourceQueue_.push_back(a);
	}
	// Add atom a to the list of atoms for which a source pointer is needed
	// Pre: a->hasSource() == false
	void enqueueFindSource(UfsAtomNode* a) {
		if (a->typeOrUnfounded == 0 && !solver_->isFalse(a->lit)) {
			unfounded_.push_back(a);
			a->typeOrUnfounded = 1;
		}
	}
	void addWatch(Literal p, UfsBodyNode* b, uint32 data);
	void addReasonLit(Literal p, const UfsBodyNode* reason);
private:  
	// A watch-structure used to notify extended bodies affected by literal assignments
	struct Watch {
		Watch(UfsBodyNode* b, uint32 data)
			: body_(b), data_(data) {}
		void setData(uint32 d) { data_ = d; }
		void notify(Literal p, DefaultUnfoundedCheck* ufs) const {
			body_->onWatch(*ufs, p, data_);
		}
	private:
		UfsBodyNode*  body_;
		uint32        data_;
	};
	typedef std::deque<UfsAtomNode*>  Queue;
	typedef PodVector<Watch>::type    Watches;
// -------------------------------------------------------------------------------------------  
// constraint interface
	PropResult     propagate(const Literal& p, uint32&, Solver& s);
	ConstraintType reason(const Literal&, LitVec&);
// -------------------------------------------------------------------------------------------
// Graph initialization
	bool relevantPrgAtom(PrgAtomNode* a) const { return a->hasVar() && a->scc() != PrgNode::noScc && !solver_->isFalse(a->literal()); }
	bool relevantPrgBody(PrgBodyNode* b) const { return b->hasVar() && !solver_->isFalse(b->literal()); }
	UfsAtomNode*  getAtom(PrgAtomNode* a);
	UfsBodyNode*  addBody(PrgBodyNode*, const AtomList& progAtoms);
	void initAtom(UfsAtomNode& an, const PrgAtomNode& a, const AtomList& prgAtoms, const BodyList& prgBodies);            
// -------------------------------------------------------------------------------------------  
// Finding unfounded sets
	bool findUnfoundedSet();
	bool findSource(UfsAtomNode* head);
	void setSource(UfsBodyNode* body, UfsAtomNode* head);
	void removeSource(UfsBodyNode* body);
	void propagateSource(bool forceTodo=false);
// -------------------------------------------------------------------------------------------  
// Propagating unfounded sets
	bool assertAtom();
	bool assertSet(); 
	bool assertAtom(Literal a);
	void computeReason();
	void createLoopFormula();
// -------------------------------------------------------------------------------------------  
// more helpers
	void enqueueTodo(UfsAtomNode* head) {
		if (head->pickedOrTodo == 0) {
			todo_.push_back(head);
			head->pickedOrTodo = 1;
		}
	}
	UfsAtomNode* dequeueTodo() {
		UfsAtomNode* head = todo_.front();
		todo_.pop_front();
		head->pickedOrTodo = 0;
		return head;
	}
	UfsAtomNode* dequeueUnfounded() {
		UfsAtomNode* head = unfounded_.front();
		unfounded_.pop_front();
		head->typeOrUnfounded = 0;
		return head;
	} 
// -------------------------------------------------------------------------------------------  
	AtomVec         atoms_;       // Atoms of the positive dependency graph
	BodyVec         bodies_;      // Bodies of the positive dependency graph
	BodyVec         invalid_;     // These bodies became invalid during unit propagation - they can no longer be source
	Watches         watches_;     // Watches for handling choice-/cardinality- and weight rules
	AtomVec         sourceQueue_; // Used during source-pointer propagation
	Queue           todo_;        // Atom that recently lost their source
	Queue           unfounded_;   // Atoms that are unfounded wrt the current assignment (limited to one scc)
	BodyVec         picked_;      // Bodies seen during reason computation
	LitVec          pickedAtoms_; // Literals in a reason from non-false extended bodies
	LitVec          loopAtoms_;   // only used if strategy_ == shared_reason
	Solver*         solver_;      // my solver
	LitVec*         reasons_;     // only used if strategy_ == only_reason. reasons_[v] reason why v is unfounded
	ClauseCreator*  activeClause_;// activeClause_[0] is the current unfounded atom
	ReasonStrategy  strategy_;    // what kind of reasons to compute?
};
}
#endif
