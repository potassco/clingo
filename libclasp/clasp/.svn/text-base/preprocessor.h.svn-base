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

#ifndef CLASP_PREPROCESSOR_H_INCLUDED
#define CLASP_PREPROCESSOR_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif
#include <clasp/literal.h>
namespace Clasp {

class ProgramBuilder;
class PrgBodyNode;
class PrgAtomNode;

/**
 * \ingroup problem Problem specification
 */
//@{

//! Preprocesses (i.e. simplifies) a logic program
/*!
 * Preprocesses (i.e. simplifies) a logic program and associates variables with
 * the nodes of the simplified logic program.
 */
class Preprocessor {
public:
	Preprocessor() : prg_(0), dfs_(true), backprop_(false) {}
	//! Possible eq-preprocessing types
	enum EqType {
		no_eq,    /*!< no eq-preprocessing, associate a new var with each supported atom and body */
		body_eq,  /*!< associate vars to atoms, then check which bodies are equivalent to atoms   */
		full_eq   /*!< check for all kinds of equivalences between atoms and bodies               */
	};

	void enableBackprop(bool b) { backprop_ = b; }

	//! starts preprocessing of the logic program
	/*!
	 * Computes the maximum consequences of prg and associates a variable 
	 * with each supported atom and body.
	 * \param prg The logic program to preprocess
	 * \param t   Type of eq-preprocessing
	 * \param maxIters If t == full_eq, maximal number of iterations during eq preprocessing
	 * \param dfs If t == full_eq, classify in df-order (true) or bf-order (false)
	 */
	bool preprocess(ProgramBuilder& prg, EqType t, uint32 maxIters, bool dfs = true) {
		prg_  = &prg;
		dfs_  = dfs;
		false_= 0;
		ok_   = true;
		return t == full_eq
			? preprocessEq(maxIters)
			: preprocessSimple(t == body_eq);
	}
	
	//! Marks the atom with the id atomId for simplification
	/*!
	 * \pre preprocess() was called with strong set to true
	 */
	void    setSimplifyBodies(uint32 atomId) {
		if (atomId < nodes_.size()) nodes_[atomId].asBody = 1;
	}
	//! Marks the body with the id bodyId for head-simplification
	/*!
	 * \pre preprocess() was called with strong set to true
	 */
	void    setSimplifyHeads(uint32 bodyId)   { 
		if (bodyId < nodes_.size()) nodes_[bodyId].sHead = 1; 
	}

	uint32 replaceComp(uint32 id) const;
private:
	Preprocessor(const Preprocessor&);
	Preprocessor& operator=(const Preprocessor&);
	void    updatePreviouslyDefinedAtoms(Var startAtom, bool strong);
	bool    preprocessSimple(bool bodyEq);
	bool    applyCompute(LitVec& compute, uint32 start, bool strong);
	Literal newBodyLit(PrgBodyNode* b);
	// ------------------------------------------------------------------------
	// Eq-Preprocessing
	struct NodeInfo {
		NodeInfo() : known(0), mBody(0), sBody(0), sHead(0), aSeen(0), asBody(0)  {}
		uint32 known  :27;  // Number of predecessors already classified, only used for bodies
		uint32 mBody  : 1;  // A flag for marking bodies
		uint32 sBody  : 1;  // Did the body change? (e.g. equivalent atoms, atoms with known truth value)
		uint32 sHead  : 1;  // Did the heads of a body change?
		uint32 aSeen  : 1;  // First time we see this atom?
		uint32 asBody : 1;  // Do we need to update the atom's body-list?
	};
	bool    preprocessEq(uint32 maxIters);
	bool    classifyProgram(uint32 startAt, uint32& stopAt);
	bool    simplifyClassifiedProgram(uint32 startAt, uint32& stopAt);
	uint32  nextBodyId(VarVec::size_type& idx) {
		if (follow_.empty() || idx == follow_.size()) { return varMax; }
		if (dfs_) {
			uint32 id = follow_.back();
			follow_.pop_back();
			return id;
		}
		return follow_[idx++];;
	}
	uint32  addBodyVar(Var bodyId, PrgBodyNode*);
	uint32  addAtomVar(Var atomId, PrgAtomNode*, PrgBodyNode* body);
	void    setSimplifyBody(uint32 bodyId)    { nodes_[bodyId].sBody = 1; }
	Var     getRootAtom(Literal p) const {
		return p.index() < litToNode_.size()
			? litToNode_[p.index()]>>1
			: varMax;
	}
	void    setRootAtom(Literal p, uint32 atomId) {
		if (p.index() >= litToNode_.size()) litToNode_.resize(p.index()+1, (varMax<<1));
		litToNode_[p.index()] = atomId<<1;
	}
	bool    hasBody(Literal p) const {
		return p.index() < litToNode_.size() 
			&& (litToNode_[p.index()] & 1) != 0;
	}
	void    setHasBody(Literal p) {
		if (p.index() >= litToNode_.size()) litToNode_.resize(p.index()+1, (varMax<<1));
		store_set_bit(litToNode_[p.index()], 0);
	}
	bool    allowMerge(PrgBodyNode* body, PrgBodyNode* root, uint32 rootId);
	bool    mergeBodies(PrgBodyNode* body, Var bodyRoot);
	bool    reclassify(PrgAtomNode* a, uint32 atomId, uint32 diffLits);
	bool    newFactBody(PrgBodyNode* b, uint32 id, uint32 oldHash);
	void    newFalseBody(PrgBodyNode* b, uint32 oldHash);
	void    removeFromIndex(PrgBodyNode* b, uint32 oldHash);
	uint32  findEqBody(PrgBodyNode* b, uint32 hash);
	// ------------------------------------------------------------------------
	ProgramBuilder*           prg_;       // program to preprocess
	VarVec                    follow_;    // bodies yet to classify
	PodVector<NodeInfo>::type nodes_;     // information about the program nodes
	VarVec                    litToNode_; // the roots of our equivalence classes
	Var                       false_;     // var used for integrity constraints
	uint32                    pass_;      // current iteration number
	uint32                    maxPass_;   // force stop after maxPass_ iterations
	bool                      ok_;        // true until preprocessing finds a conflict
	bool                      dfs_;       // classify bodies in DF or BF order
	bool                      backprop_;  // back-propagate truth values
};
//@}
}
#endif
