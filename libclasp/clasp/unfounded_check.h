// 
// Copyright (c) 2010, Benjamin Kaufmann
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
#include <clasp/dependency_graph.h>
#include <clasp/constraint.h>
namespace Clasp {
class LoopFormula;

//! Clasp's default unfounded set checker.
/*!
 * \ingroup constraint
 * Searches for unfounded atoms by checking the positive dependency graph (PDG)
 *
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
	typedef SharedDependencyGraph DependencyGraph;
	typedef DependencyGraph::NodeId    NodeId;
	typedef DependencyGraph::BodyNode  BodyNode;
	typedef DependencyGraph::AtomNode  AtomNode;
	//! Defines the supported reasons for explaining assignments.
	enum ReasonStrategy {
		common_reason,    /*!< one reason for each unfounded set but one clause for each atom */
		distinct_reason,  /*!< distinct reason and clause for each unfounded atom */
		shared_reason,    /*!< one shared loop formula for each unfounded set */
		only_reason,      /*!< store only the reason but don't learn a nogood */
		no_reason,        /*!< do no compute reasons for unfounded sets (only valid if learning is disabled!) */
	};
	
	explicit DefaultUnfoundedCheck(ReasonStrategy st = common_reason);
	~DefaultUnfoundedCheck();

	ReasonStrategy reasonStrategy() const { return strategy_; }
	void           setReasonStrategy(ReasonStrategy rs);
	
	DependencyGraph* graph() const { return graph_; }
	uint32           nodes() const { return static_cast<uint32>(atoms_.size() + bodies_.size()); }

	// base interface
	uint32 priority() const { return uint32(priority_reserved_ufs); }
	bool   init(Solver&);
	void   reset();
	bool   propagateFixpoint(Solver& s, PostPropagator* ctx);
	bool   isModel(Solver& s);
	bool   valid(Solver& s);
	bool   simplify(Solver& s, bool);
	void   destroy(Solver* s, bool detach);
private:
	DefaultUnfoundedCheck(const DefaultUnfoundedCheck&);
	DefaultUnfoundedCheck& operator=(const DefaultUnfoundedCheck&);
	enum UfsType {
		ufs_none,
		ufs_poly,
		ufs_non_poly
	};
	enum WatchType {
		watch_source_false = 0,
		watch_head_false   = 1,
		watch_head_true    = 2,
		watch_subgoal_false= 3,
	};
	// data for each body
	struct BodyData {
		BodyData() : watches(0), picked(0) {}
		uint32 watches : 31; // how many atoms watch this body as source?
		uint32 picked  :  1; // flag used in computeReason()
		uint32 lower_or_ext; // unsourced preds or index of extended body
	};
	struct BodyPtr {
		BodyPtr(const BodyNode* n, uint32 i) : node(n), id(i) {}
		const BodyNode* node;
		uint32          id;
	};
	// data for extended bodies
	struct ExtData {
		ExtData(weight_t bound, uint32 preds) : lower(bound) {
			for (uint32 i = 0; i != flagSize(preds); ++i) { flags[i] = 0; }
		}
		bool addToWs(uint32 idx, weight_t w) {
			const uint32 fIdx = (idx / 32);
			const uint32 m    = (1u << (idx & 31));
			assert((flags[fIdx] & m) == 0);
			flags[fIdx]      |= m;
			return (lower -= w) <= 0;
		}
		bool inWs(uint32 idx) const {
			const uint32 fIdx = (idx / 32);
			const uint32 m    = (1u << (idx & 31));
			return (flags[fIdx] & m) != 0;
		}
		void removeFromWs(uint32 idx, weight_t w) {
			if (inWs(idx)) {
				lower += w;
				flags[(idx / 32)] &= ~(uint32(1) << (idx & 31));
			}
		}
		static   uint32 flagSize(uint32 preds) { return (preds+31)/32; }
		weight_t lower;
		uint32   flags[0];
	};
	// data for each atom
	struct AtomData {
		AtomData() : source(nill_source), todo(0), ufs(0), validS(0) {}
		// returns the body that is currently watched as possible source
		NodeId watch()     const   { return source; }
		// returns true if atom has currently a source, i.e. a body that can still define it
		bool   hasSource() const   { return validS; }
		// mark source as invalid but keep the watch
		void   markSourceInvalid() { validS = 0; }
		// restore validity of source
		void   resurrectSource()   { validS = 1; }
		// sets b as source for this atom
		void   setSource(NodeId b) {
			source = b;
			validS = 1;
		}
		static const uint32 nill_source = (uint32(1) << 29)-1;
		uint32 source : 29; // id of body currently watched as source
		uint32 todo   :  1; // in todo-queue?
		uint32 ufs    :  1; // in ufs-queue?
		uint32 validS :  1; // is source valid?
	};
	// Watch-structure used to update extended bodies affected by literal assignments
	struct ExtWatch {
		NodeId bodyId;
		uint32 data;
	};
	// Minimality checker for disjunctive logic programs.
	struct MinimalityCheck {
		typedef SolveParams::FwdCheck FwdCheck;
		explicit MinimalityCheck(const FwdCheck& fwd);
		bool     partialCheck(uint32 level);
		void     schedNext(uint32 level, bool ok);
		FwdCheck fwd;
		uint32   high;
		uint32   low;
		uint32   next;
		uint32   scc;
	};
	// -------------------------------------------------------------------------------------------  
	// constraint interface
	PropResult propagate(Solver&, Literal, uint32& data) {
		uint32 index = data >> 2;
		uint32 type  = (data & 3u);
		if (type != watch_source_false || bodies_[index].watches) {
			invalidQ_.push_back(data);
		}
		return PropResult(true, true);
	}
	void reason(Solver& s, Literal, LitVec&);
	// -------------------------------------------------------------------------------------------
	// initialization
	BodyPtr getBody(NodeId bId) const { return BodyPtr(&graph_->getBody(bId), bId); }
	void    initBody(const BodyPtr& n);
	void    initExtBody(const BodyPtr& n);
	void    initSuccessors(const BodyPtr& n, weight_t lower);
	void    addWatch(Literal, uint32 data, WatchType type);
	void    addExtWatch(Literal p, const BodyPtr& n, uint32 data);
	struct  InitExtWatches {
		void operator()(Literal p, uint32 idx, bool ext) const { 
			self->addExtWatch(~p, *B, (idx<<1)+uint32(ext)); 
			if (ext && !self->solver_->isFalse(p)) {
				extra->addToWs(idx, B->node->pred_weight(idx, true));
			}
		}
		DefaultUnfoundedCheck* self;
		const BodyPtr*         B;
		ExtData*               extra;
	};
	struct RemExtWatches {
		void operator()(Literal p, uint32, bool) const { s->removeWatch(~p, self); }
		Constraint* self;
		Solver*     s;
	};
	// -------------------------------------------------------------------------------------------  
	// propagating source pointers
	void propagateSource();
	struct AddSource { // an atom in a body has a new source, check if body is now a valid source
		explicit AddSource(DefaultUnfoundedCheck* u) : self(u) {}
		// normal body
		void operator()(NodeId bId) const {
			BodyPtr n(self->getBody(bId));
			if (--self->bodies_[bId].lower_or_ext == 0 && !self->solver_->isFalse(n.node->lit)) { self->forwardSource(n); }
		}
		// extended body
		void operator()(NodeId bId, uint32 idx) const;
		DefaultUnfoundedCheck* self;
	};
	struct RemoveSource {// an atom in a body has lost its source, check if body is no longer a valid source 
		explicit RemoveSource(DefaultUnfoundedCheck* u, bool add = false) : self(u), addTodo(add) {}
		// normal body
		void operator()(NodeId bId) const { 
			if (++self->bodies_[bId].lower_or_ext == 1 && self->bodies_[bId].watches != 0) { 
				self->forwardUnsource(self->getBody(bId), addTodo); 
			}
		}
		// extended body
		void operator()(NodeId bId, uint32 idx) const;
		DefaultUnfoundedCheck* self;
		bool                   addTodo;
	};
	void setSource(NodeId atom, const BodyPtr& b);
	void removeSource(NodeId bodyId);
	void forwardSource(const BodyPtr& n);
	void forwardUnsource(const BodyPtr& n, bool add);
	void updateSource(AtomData& atom, const BodyPtr& n);
	// -------------------------------------------------------------------------------------------  
	// finding & propagating unfounded sets
	void updateAssignment(Solver& s);
	bool findSource(NodeId atom);
	bool isValidSource(const BodyPtr&);
	void addUnsourced(const BodyPtr&);
	bool falsifyUfs(UfsType t);
	bool assertAtom(Literal a, UfsType t);
	void computeReason(UfsType t);
	void addIfReason(const BodyPtr&, uint32 uScc);
	void addDeltaReason(const BodyPtr& body, uint32 uScc);
	void addReasonLit(Literal);
	void createLoopFormula();
	struct  AddReasonLit {
		void operator()(Literal p, NodeId, bool) const { if (self->solver_->isFalse(p)) self->addReasonLit(p); }
		DefaultUnfoundedCheck* self;
	};
	UfsType findUfs(Solver& s, bool checkNonHcf);
	UfsType findNonHcfUfs(Solver& s);
	// -------------------------------------------------------------------------------------------  
	bool pushTodo(NodeId at) { return (atoms_[at].todo == 0 && (todo_.push(at), atoms_[at].todo = 1) != 0); }
	bool pushUfs(NodeId at)  { return (atoms_[at].ufs  == 0 && (ufs_.push(at),  atoms_[at].ufs  = 1) != 0); }
	void resetTodo()         { while (!todo_.empty()){ atoms_[todo_.pop_ret()].todo = 0; } todo_.clear();   }
	void resetUfs()          { while (!ufs_.empty()) { atoms_[ufs_.pop_ret()].ufs   = 0; } ufs_.clear();    } 
	// -------------------------------------------------------------------------------------------  
	typedef PodVector<AtomData>::type       AtomVec;
	typedef PodVector<BodyData>::type       BodyVec;
	typedef PodVector<ExtData*>::type       ExtVec;
	typedef PodVector<ExtWatch>::type       WatchVec;
	typedef PodQueue<NodeId>                IdQueue;
	typedef SingleOwnerPtr<MinimalityCheck> MiniPtr;
	// -------------------------------------------------------------------------------------------  
	Solver*          solver_;      // my solver
	DependencyGraph* graph_;       // PBADG
	MiniPtr          mini_;        // minimality checker (only for DLPs)
	AtomVec          atoms_;       // data for each atom       
	BodyVec          bodies_;      // data for each body
	IdQueue          todo_;        // ids of atoms that recently lost their source
	IdQueue          ufs_;         // ids of atoms that are unfounded wrt the current assignment (limited to one scc)
	VarVec           invalidQ_;    // ids of invalid elements to be processed
	VarVec           sourceQ_;     // source-pointer propagation queue
	ExtVec           extended_;    // data for each extended body
	WatchVec         watches_;     // watches for handling choice-, cardinality- and weight rules
	VarVec           pickedExt_;   // extended bodies visited during reason computation
	LitVec           loopAtoms_;   // only used if strategy_ == shared_reason
	LitVec           activeClause_;// activeClause_[0] is the current unfounded atom
	LitVec*          reasons_;     // only used if strategy_ == only_reason. reasons_[v] reason why v is unfounded
	ClauseInfo       info_;        // info on active clause
	ReasonStrategy   strategy_;    // what kind of reasons to compute?
};
}
#endif
