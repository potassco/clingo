// 
// Copyright (c) 2010-2012, Benjamin Kaufmann
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

#ifndef CLASP_DEPENDENCY_GRAPH_H_INCLUDED
#define CLASP_DEPENDENCY_GRAPH_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

#include <clasp/logic_program.h>
#include <clasp/solver_strategies.h>
#include <algorithm>
namespace Clasp {
class Solver;
class SharedContext;
class SharedDependencyGraph;
struct SolverStats;
struct SolveTestEvent : SolveEvent<SolveTestEvent> {
	SolveTestEvent(const Solver& s, uint32 scc, bool partial);
	int    result;     // -1: before test, 0: unstable, 1: stable
	uint32 scc     :31;// scc under test
	uint32 partial : 1;// partial test?
	uint64 confDelta;  // conflicts before test
	uint64 choiceDelta;// choices before test
	double time;       // time for test
	
	uint64 conflicts() const;
	uint64 choices()   const;
};

//! (Positive) Body-Atom-Dependency Graph.
/*!
 * \ingroup shared
 *
 * Represents the PBADG of a logic program. Once initialized, the
 * PBDAG is static and read-only and thus can be shared between multiple solvers.
 *
 * \note Initialization is *not* thread-safe, i.e. must be done only once by one thread.
 */
class SharedDependencyGraph {
public:
	SharedDependencyGraph(Configuration* nonHcfCfg = 0);
	~SharedDependencyGraph();
	typedef uint32            NodeId;
	typedef Asp::PrgNode      PrgNode;
	typedef Asp::PrgBody      PrgBody;
	typedef Asp::PrgAtom      PrgAtom;
	typedef Asp::PrgDisj      PrgDisj;
	typedef Asp::LogicProgram LogicProgram;
	typedef Asp::NonHcfSet    NonHcfSet;
	typedef Asp::AtomList     AtomList;

	//! Type for storing a non head-cycle-free component of a disjunctive program.
	class NonHcfComponent {
	public:
		explicit NonHcfComponent(const SharedDependencyGraph& dep, SharedContext& generator, uint32 scc, const VarVec& atoms, const VarVec& bodies);
		~NonHcfComponent();
		void assumptionsFromAssignment(const Solver& generator, LitVec& assumptionsOut) const;
		bool test(uint32 scc, const Solver& generator, const LitVec& assumptions, VarVec& unfoundedOut)  const;
		bool simplify(uint32 scc, const Solver& generator) const;
		const SharedContext& ctx() const { return *prg_; }
		void update(const SharedContext& generator);
	private:
		friend class SharedDependencyGraph;
		NonHcfComponent(const NonHcfComponent&);
		NonHcfComponent& operator=(const NonHcfComponent&);
		class ComponentMap;
		SharedContext* prg_;
		ComponentMap*  comp_;
	};
	typedef std::pair<uint32, NonHcfComponent*> ComponentPair;
	typedef const ComponentPair* NonHcfIter;
	//! Base type for nodes.
	struct Node {
		Node(Literal l = Literal(0, false), uint32 sc = PrgNode::noScc) 
			: lit(l), scc(sc), data(0), adj_(0), sep_(0) {}
		Literal lit;       // literal of this node
		uint32  scc   : 28;// scc of this node
		uint32  data  :  4;// additional atom/body data 
		NodeId* adj_;      // list of adjacent nodes
		NodeId* sep_;      // separates successor/predecessor nodes
	};
	//! An atom node.
	/*!
	 * The PBDAG stores a node of type AtomNode for each non-trivially connected atom.
   * The predecessors of an AtomNode are the bodies that define the atom. Its successors
   * are those bodies from the same SCC that contain the atom positively.
   */
	struct AtomNode : public Node {
		enum Property { property_in_choice = 1u, property_in_disj = 2u, property_in_ext = 4u, property_in_non_hcf = 8u};
		AtomNode() {}
		void  set(Property p)         { data |= (uint32)p; }
		void  setProperties(uint32 f) { assert(f < 8); data |= f; }
		//! Contained in the head of a choice rule?
		bool          inChoice()     const { return (data & property_in_choice) != 0; }
		//! Contained in the head of a non-hcf disjunctive rule?
		bool          inDisjunctive()const { return (data & property_in_disj) != 0; }
		//! Contained in an extended body?
		bool          inExtended()   const { return (data& property_in_ext) != 0; }
		//! Contained in a non-hcf SCC?
		bool          inNonHcf()     const { return (data & property_in_non_hcf) != 0; }
		//! Bodies (i.e. predecessors): bodies from other SCCs precede those from same SCC.
		const NodeId* bodies_begin() const { return adj_; }
		const NodeId* bodies_end()   const { return sep_; }
		NodeId        body(uint32 i) const { return bodies_begin()[i]; }
		//! Successors from same SCC [B1,...Bn, idMax].
		/*!
		 * \note If extended() is true, the atom is adjacent to some extended body.
		 * In that case, the returned list looks like this:
		 * [Bn1, ..., Bnj, idMax, Bext1, pos1, ..., Bextn, posn, idMax], where
		 * each Bni is a normal body, each Bexti is an extended body and posi is the
		 * position of this atom in Bexti. 
		 */
		const NodeId* succs()        const { return sep_; }
		//! Calls the given function object p once for each body containing this atom.
		template <class P>
		void visitSuccessors(const P& p) const {
			const NodeId* s = succs();
			for (; *s != idMax; ++s) { p(*s); }
			if (inExtended()) {
				for (++s; *s != idMax; s += 2) { p(*s, *(s+1)); }
			}
		}
	};
	enum { sentinel_atom = 0u };
	
	//! A body node.
	/*!
	 * The PBDAG stores a node of type BodyNode for each body that defines 
	 * a non-trivially connected atom.
   * The predecessors of a BodyNode are the body's subgoals.
	 * Its successors are the heads that are defined by the body.
	 * \note Normal bodies only store the positive subgoals from the same SCC, while
	 * extended rule bodies store all subgoals. In the latter case, the positive subgoals from
	 * the same SCC are stored as AtomNodes. All other subgoals are stored as literals.
	 */
	struct BodyNode : public Node {
		enum Type { type_normal    = 0u, type_count = 1u, type_sum = 3u };
		enum Flag { flag_has_delta = 4u, flag_seen  = 8u };
		explicit BodyNode(PrgBody* b, uint32 scc) : Node(b->literal(), scc) {
			if (scc == PrgNode::noScc || b->type() == Asp::BodyInfo::NORMAL_BODY) {
				data = type_normal;
			}
			else if (b->type() == Asp::BodyInfo::COUNT_BODY){ data = type_count; }
			else if (b->type() == Asp::BodyInfo::SUM_BODY)  { data = type_sum;   }
			else                                            { assert("UNKNOWN BODY TYPE!\n"); }
		}
		bool seen() const { return (data & flag_seen) != 0; }
		void seen(bool b) { if (b) data |= flag_seen; else data &= ~uint32(flag_seen); }
		
		//! Heads (i.e. successors): atoms from same SCC precede those from other SCCs.
		/*!
		 * \note Disjunctive heads are stored in flattened atom-lists, where the
	   *       lists are terminated on both ends with the special sentinal atom 0.
	   *       E.g. given
		 *        x :- B.
		 *        y :- B.
		 *       a|b:- B.
		 *       a|c:- B.
		 *       would result in: [x,y,0,a,b,0,0,a,c,0]
		 */
		const NodeId* heads_begin() const { return adj_; }
		const NodeId* heads_end()   const { return sep_ - extended(); }
		//! Any disjunctive heads?
		bool          delta()       const { return (data & flag_has_delta) != 0; }
		//! Predecessors from same SCC [a1,...an, idMax].
		/*!
		 * \note If extended() is true, the body stores all its subgoals and preds looks
		 * like this: [a1, [w1], ..., aj, [wj], idMax, l1, [w1], ..., lk, [wk], idMax], where
		 * each ai is an atom from the same SCC, each li is a literal of a subgoal from 
		 * other SCCs and wi is an optional weight (only for weight rules).
		 */
		const NodeId* preds()       const { assert(scc != PrgNode::noScc); return sep_; }
		//! Returns idx of atomId in preds.
		uint32        get_pred_idx(NodeId atomId) const { 
			const uint32 inc = pred_inc();
			      uint32 idx = 0;
			for (const NodeId* x = preds(); *x != idMax; x += inc, ++idx) {
				if (*x == atomId) return idx;
			}
			return idMax;
		}
		NodeId        get_pred(uint32 idx) const { return *(preds() + (idx*pred_inc())); }
		//! Increment to jump from one pred to the next.
		uint32        pred_inc()           const { return 1 + sum(); }
		//! Weight of ith subgoal.
		/*!
		 * \pre i in [0, num_preds())
		 */
		uint32        pred_weight(uint32 i, bool ext) const { 
			return !sum()
				? 1 
				: *(preds() + (i*pred_inc()) + (1+uint32(ext)));
		}
		//! Number of predecessors (counting external subgoals).
		uint32        num_preds() const {
			if (scc == PrgNode::noScc) return 0;
			uint32 p        = 0;
			const NodeId* x = preds();
			const uint32 inc= pred_inc();
			for (; *x != idMax; x += inc) { ++p; }
			x += extended();
			for (; *x != idMax; x += inc) { ++p; }
			return p;
		}
		//! Is the body an extended body?
		bool          extended()const { return (data & type_count) != 0; }
		//! Is the body a sum body?
		bool          sum()     const { return (data & type_sum) == type_sum; }
		//! Bound of extended body.
		weight_t      ext_bound() const { return sep_[-1]; }
	};
	//! Adds SCCs to the graph.
	/*!
	 * \param prg       The logic program for which the dependency graph is to be created.
	 * \param sccAtoms  Atoms of the logic program that are strongly connected.
	 * \param nonHcfs   Sorted list of non-hcf sccs
	 */
	void addSccs(LogicProgram& prg, const AtomList& sccAtoms, const NonHcfSet& nonHcfs);

	//! Removes inactive non-hcfs.
	void simplify(const Solver& s);

	//! Number of atoms in graph.
	uint32 numAtoms() const { return (uint32)atoms_.size();  }
	//! Number of bodies in graph.
	uint32 numBodies()const { return (uint32)bodies_.size(); }
	//! Sum of atoms and bodies.
	uint32 nodes()    const { return numAtoms()+numBodies(); }

	//! Returns AtomNode of atom with given id.
	const AtomNode& getAtom(NodeId atomId) const {
		assert(atomId < atoms_.size());
		return atoms_[atomId];
	}
	NodeId id(const AtomNode& n) const { return static_cast<uint32>(&n - &atoms_[0]); }
	//! Returns BodyNode of body with given id.
	const BodyNode& getBody(NodeId bodyId) const {
		assert(bodyId < bodies_.size());
		return bodies_[bodyId];
	}
	//! Calls the given function object p once for each body-literal.
	template <class P>
	void visitBodyLiterals(const BodyNode& n, const P& p) {
		const NodeId* x  = n.preds();
		const uint32 inc = n.pred_inc();
		uint32       i   = 0;
		for (; *x != idMax; x += inc, ++i) { p(getAtom(*x).lit, i, false); }
		x += n.extended();
		for (; *x != idMax; x += inc, ++i) { p(Literal::fromRep(*x),i, true); }
	}
	NonHcfIter     nonHcfBegin() const { return components_.empty() ? NonHcfIter(0) : &components_[0]; }
	NonHcfIter     nonHcfEnd()   const { return nonHcfBegin() + components_.size(); }
	uint32         numNonHcfs()  const { return (uint32)components_.size(); }
	Configuration* nonHcfConfig()const { return config_; }
	void           accuStats()   const;
private:
	typedef PodVector<AtomNode>::type      AtomVec;
	typedef PodVector<BodyNode>::type      BodyVec;
	typedef PodVector<ComponentPair>::type ComponentMap;
	SharedDependencyGraph(const SharedDependencyGraph&);
	SharedDependencyGraph& operator=(const SharedDependencyGraph&);
	inline bool      relevantPrgAtom(const Solver& s, PrgAtom* a) const;
	inline bool      relevantPrgBody(const Solver& s, PrgBody* b) const;
	NodeId           createBody(PrgBody* b, uint32 bScc);
	NodeId           createAtom(Literal lit, uint32 aScc);
	NodeId           addBody(const LogicProgram& prg, PrgBody*);
	NodeId           addDisj(const LogicProgram& prg, PrgDisj*);
	uint32           addHeads(const LogicProgram& prg, PrgBody*, VarVec& atoms) const;
	uint32           getAtoms(const LogicProgram& prg, PrgDisj*, VarVec& atoms) const;
	void             addPreds(const LogicProgram& prg, PrgBody*, uint32 bScc, VarVec& preds) const;
	void             initBody(uint32 id, const VarVec& preds, const VarVec& atHeads);
	void             initAtom(uint32 id, uint32 prop, const VarVec& adj, uint32 preds);
	void             addNonHcf(SharedContext& ctx, uint32 scc);
	AtomVec        atoms_;
	BodyVec        bodies_;
	ComponentMap   components_;
	uint32         seenComponents_;
	Configuration* config_;
};

}
#endif
