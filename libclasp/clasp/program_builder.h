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

#ifndef CLASP_PROGRAM_BUILDER_H_INCLUDED
#define CLASP_PROGRAM_BUILDER_H_INCLUDED

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif

#include <clasp/solver_types.h>
#include <clasp/program_rule.h>
#include <clasp/util/misc_types.h>
#include <string>
#include <map>
#include <algorithm>
#include <iosfwd>
#include <stdexcept>

namespace Clasp {

class Solver;
class ClauseCreator;
class PrgNode;
class PrgBodyNode;
class PrgAtomNode;
class ProgramBuilder;
class Preprocessor;
class DefaultUnfoundedCheck;
class MinimizeConstraint;
typedef PodVector<PrgAtomNode*>::type AtomList;
typedef PodVector<PrgBodyNode*>::type BodyList;
typedef PodVector<PrgNode*>::type NodeList;

const ValueRep value_weak_true = 3; /**< true but no proof */

/**
 * \defgroup problem Problem specification
 * Classes and functions for defining/processing logic programs
 */
//@{

//! A list of variables and their types
/*!
 * A VarList stores the type of each variable, which allows for easy variable-cloning.
 * It also stores flags for each literal to be used in "mark and test"-algorithms.
 */
class VarList {
public:
	VarList() { add(Var_t::atom_var); }
	//! adds a new var of type t to the list
	Var   add(VarType t) {
		vars_.push_back( static_cast<uint8>(t << 5) );
		return (Var)vars_.size()-1;
	}
	//! marks v as equivalent to both a body and an atom
	void  setAtomBody(Var v) { setFlag(v, eq_f); }
	//! adds to s all variables contained in this var list that are >= startVar 
	void  addTo(Solver& s, Var startVar);
	//! returns true if this var list is empty
	bool empty() const { return vars_.size() == 1; }
	//! returns the number of vars contained in this list
	VarVec::size_type size() const { return vars_.size(); }
	//! clears the var list
	void clear() { Vars().swap(vars_); add(Var_t::atom_var); }
	
	//! removes all vars > startVar
	void shrink(Var startVar) {
		startVar = std::max(Var(1), startVar);
		vars_.resize(startVar);
	}
	/*!
	 * \name Mark and Test
	 * Functions supporting O(1) "mark and test" operations on literals
	 */
	//@{
	bool marked(Literal p)      const { return hasFlag(p.var(), p.sign()?uint8(bm_f):uint8(bp_f)); }
	bool markedHead(Literal p)  const { return hasFlag(p.var(), p.sign()?uint8(hn_f):uint8(hp_f)); }
	void mark(Literal p)              { setFlag(p.var(), p.sign()?uint8(bm_f):uint8(bp_f)); }
	void markHead(Literal p)          { setFlag(p.var(), p.sign()?uint8(hn_f):uint8(hp_f)); }
	void unmark(Var v)                { clearFlag(v, uint8(bp_f+bm_f)); }
	void unmarkHead(Var v)            { clearFlag(v, uint8(hp_f+hn_f)); }
	//@}
private:
	typedef PodVector<uint8>::type Vars;
	enum Flags {
		bp_f  = 0x1u,
		bm_f  = 0x2u,
		hp_f  = 0x4u,
		hn_f  = 0x8u,
		eq_f  = 0x10u
	};
	void setFlag(Var v, uint8 f)      { assert(v < vars_.size()); vars_[v] |= f; }
	void clearFlag(Var v, uint8 f)    { assert(v < vars_.size()); vars_[v] &= ~f; }
	bool hasFlag(Var v, uint8 f) const{ assert(v < vars_.size()); return (vars_[v] & f) != 0; }
	VarType type(Var v) const { return VarType(vars_[v] >> 5); }
	Vars          vars_;
};

//! Program statistics for *one* incremental step
struct PreproStats {
	PreproStats() : trStats(0) { reset(); }
	~PreproStats()             { delete trStats; }
	PreproStats(const PreproStats& o) {
		reset();
		if (o.trStats) { trStats = new TrStats(*o.trStats);  }
	}
	PreproStats& operator=(const PreproStats& o) {
		if (this != &o) {
			std::memcpy(this, &o, sizeof(*this)-sizeof(trStats));
			TrStats* n = o.trStats ? new TrStats(*o.trStats) : 0;
			std::swap(trStats, n);
			delete n;
		}
		return *this;
	}
	void reset() {
		delete trStats; trStats = 0;
		std::memset(this, 0, sizeof(*this)-sizeof(trStats));
	}
	uint32 sumEqs()          const { return numEqs(Var_t::atom_var) + numEqs(Var_t::body_var) + numEqs(Var_t::atom_body_var); }
	uint32 numEqs(VarType t) const { return eqs[t-1]; }
	void   incEqs(VarType t)       { ++eqs[t-1]; }
	void   updateTrStats(RuleType rt, uint32 newRules) {
		if (!trStats) { trStats = new TrStats(); }
		++trStats->rules[rt];
		trStats->rules[0] += newRules;
	}
	void   accu(const PreproStats& o);
	uint32 bodies;    /**< How many body-objects were created? */
	uint32 atoms;     /**< Number of program atoms */
	uint32 sccs;      /**< How many strongly connected components? */
	uint32 ufsNodes;  /**< How many nodes in the positive BADG? */
	uint32 rules[7];  /**< Number of rules. rules{0]: sum, rules[RuleType rt]: rules of type rt */
	uint32 eqs[3];    /**< How many equivalences?: eqs[0]: Atom-Atom, eqs[1]: Body-Body, eqs[2]: Other */
	struct TrStats {  
		TrStats() { std::memset(this, 0, sizeof(TrStats)); }
		TrStats(const TrStats& o) : auxAtoms(o.auxAtoms) {
			for (int i = 0; i != sizeof(rules)/sizeof(uint32); ++i) {
				rules[i] = o.rules[i];
			}
		}
		TrStats& operator=(const TrStats& o) {
			if (this != &o) { std::memcpy(this, &o, sizeof(TrStats)); }
			return *this;
		}
		uint32 auxAtoms;/**< Number of aux atoms created */
		uint32 rules[7];/**< rules[0]: rules created, rules[RuleType rt]: rules of type rt translated */
	}*     trStats;   /**< Rule translation statistics (optional) */
};


//! Exception type for signaling an invalid incremental program update.
class RedefinitionError : public std::logic_error {
public:
	explicit RedefinitionError(const std::string& m) : std::logic_error(m) {}
};


//! Interface for defining a logic program.
/*!
 * Use this class to specify a logic program. Once the program is completly defined,
 * call endProgram() to load the logic program into a Solver-object.
 *
 */
class ProgramBuilder {
public:
	ProgramBuilder();
	~ProgramBuilder();

	//! Defines the possible modes for handling extended rules, i.e. choice, cardinality, and weight rules
	enum ExtendedRuleMode {
		mode_native           = 0, /**< Handle extended rules natively                          */
		mode_transform        = 1, /**< Transform extended rules to normal rules                */
		mode_transform_choice = 2, /**< Transform only choice rules to normal rules             */
		mode_transform_card   = 3, /**< Transform only cardinality rules to normal rules        */
		mode_transform_weight = 4, /**< Transform cardinality- and weight rules to normal rules */
		mode_transform_integ  = 5, /**< Transform cardinality-based integrity constraints       */
		mode_transform_dynamic= 6  /**< Heuristically decide whether or not to transform a particular extended rule */
	};

	/*!
	 * \name Setup and step control functions
	 */
	//@{

	//! Starts the definition of a logic program. 
	/*!
	 * This function shall be called exactly once before a new program is defined.
	 * It discards any previously added program.
	 *
	 * \param index The symbol table in which the program builder stores information about atoms
	 * \param ufs The object to use for unfounded set detection. If 0, unfounded set detection is disabled.
	 * \note If ufsChecker is not 0 it shall point to a heap-allocated object. The ownership is transferred!
	 */
	ProgramBuilder& startProgram(AtomIndex& index, DefaultUnfoundedCheck* ufs);

	//! Sets the mode for handling extended rules.
	/*!
	 * The default is to handle all extended rules natively.
	 */
	void setExtendedRuleMode(ExtendedRuleMode m) {
		erMode_ = m;
	}

	//! Sets options for eq preprocessing
	/*
	 * \param maxIters max number of iterations (0 = disable eq preprocessing)
	 * \param dfs run preprocessor in df-order (true) or bf-order (false)
	 * \note The default eq options are maxIters = -1 (run to fixpoint) and dfs = false
	 */
	void setEqOptions(uint32 maxIters, bool dfs = false) {
		eqIters_ = maxIters;
		eqDfs_   = dfs;
	}

	//! Unfreezes a currently frozen program and starts an incremental step.
	/*!
	 * \pre The program is either frozen or at step 0.
	 *
	 * If a program is to be defined incrementally, this function must be called
	 * exactly once for each step before any new rules or atoms are added.
	 * \note Program update only works correctly under the following assumptions:
	 *  - Atoms introduced in step i are either:
	 *    - solely defined in step i OR,
	 *    - marked as frozen in step i and solely defined in step i+k OR,
	 *    - forced to false by a acompute statement in step 0
	 *
	 * \post The program is no longer frozen and calling program mutating functions is valid again. 
	 * \throws std::logic_error precondition is violated.
	 */
	ProgramBuilder& updateProgram();

	//! Finishes the definition of the logic program (or its current increment).
	/*!
	 * Applies program mutating operations issued in the current step and transforms 
	 * the new program into the solver's internal representation. 
	 *
	 * \param solver The solver object in which the representation of the lp should be stored.
	 * \param finalizeSolver if true, endProgram() calls solver.endAddConstraints()
	 * \param backprop Enable backpropagation in preprocessing
	 * \return false if the program is initially conflicting, true otherwise.
	 *
	 * \post
	 *  - If true is returned, the program is considered to be "frozen" and calling 
	 *    program mutating functions is invalid until the next call to updateProgram().
	 *  - If false is returned, the state of the object is undefined and startProgram() 
	 *    and disposeProgram() are the only remaining valid operations. 
	 *  . 
	 *
	 * \note To load the same program into different solvers, 
	 *       call endProgram for each solver.
	 */
	bool endProgram(Solver& solver, bool finalizeSolver, bool backprop = false);

	//! Creates a minimize constraint from the minimize statements contained in the program.
	/*!
	 * \pre The program is frozen and not trivially UNSAT, i.e. endProgram() was already called and did not return false.
	 * \return 0 if the program does not contain minimize statements
	 */
	MinimizeConstraint* createMinimize(Solver& s, bool inHeu = false);

	//! Writes the (possibly simplified) program in lparse-format to the given stream.
	void writeProgram(std::ostream& os);

	//! Disposes (parts of) the internal representation of the logic program.
	/*!
	 * \param forceFullDispose If set to true, the whole program is disposed. Otherwise,
	 *  only the rules (of the current step) are disposed but atoms and any incremental
	 *  control data remain.
	 */
	void disposeProgram(bool forceFullDispose);
	//@}
	
	/*!
	 * \name Program mutating functions
	 * 
	 * Functions in this group shall only be called if the program is currently not 
	 * frozen. That is, only between the call to startProgram() (resp. updateProgram() if in 
	 * incremental setting) and endProgram(). A std::logic_error is raised if this precondition is violated. 
	 *
	 */
	//@{

	//! Adds a new atom to the program.
	/*!
	 * \return The new atom's id. 
	 */
	Var newAtom();

	//! Sets the name of the given atom and adds it to the program's symbol table.
	/*!
	 * \pre 
	 *   - The atom is either not yet known or was added in the current step (atomId >= startAtom()).
	 *   - The atom was not yet added to the symbol table, i.e. 
	 *     setAtomName() is called at most once for an atom.
	 *   . 
	 * \param atomId The id of the atom for which a name should be set
	 * \param name The new name of the atom with the given id.
	 * \note If atomId is not yet known, an atom with the given id is implicitly created. 
	 *
	 * \throws RedefinitionError precondition is violated. 
	 * \throws std::logic_error  program is frozen.
	 */
	ProgramBuilder& setAtomName(Var atomId, const char* name);

	//! Forces the atom's truth-value to value. 
	/*!
	 * \pre The atom is either not yet known, false, or an atom from the current step.
	 * \param atomId Id of the Atom for which a truth-value should be set.
	 * \param pos If true, atom is set to true (forced to be in every answer set). Otherwise
	 *            atom is set to false (not part of any answer set).
	 * \note If atomId is not yet known, an atom with the given id is implicitly created. 
	 */
	ProgramBuilder& setCompute(Var atomId, bool value);

	//! Protects an otherwise undefined atom from preprocessing. 
	/*!
	 * If the atom is defined in this or a previous step, the operation has no effect. 
	 * \note If atomId is not yet known, an atom with the given id is implicitly created. 
	 */
	ProgramBuilder& freeze(Var atomId);

	//! Removes any protection from the given atom. 
	/*!
	 * If the atom is defined in this or a previous step, the operation has no effect. 
	 * \note
	 *   - The effect is logically equivalent to adding a rule atomId :- false. 
	 *   - A call to unfreeze() always overwrites a call to freeze() even if the 
	 *     latter comes after the former
	 *   . 
	 */
	ProgramBuilder& unfreeze(Var atomId);

	//! Adds the given rule to the program.
	/*!
	 * \pre The head of the rule does not contain an atom defined in a 
	 *      previous incremental step. 
	 *
	 * Simplifies the given rule and adds it to the program if it
	 * is neither tautological (e.g. a :- a) nor contradictory (e.g. a :- b, not b). 
	 * Atoms in the simplified rule that are not yet known are implicitly created. 
	 *
	 * \throws RedefinitionError if the precondition is violated. 
	 * \note If the head of the simplified rule mentions an atom from a previous step,
	 *       that atom shall either be frozen or false. In the former case, 
	 *       unfreeze() is implicitly called. In the latter case, the rule is interpreted 
	 *       as an integrity constraint. 
	 */
	ProgramBuilder& addRule(PrgRule& r);
	
	//@}

	/*!
	 * \name Rule creation functions
	 * 
	 * Functions in this group may be used to construct logic program rules.
	 * The construction of a rule must start with a call to startRule() and
	 * ends with a call to endRule(). Functions for adding elements to a
	 * rule shall only be called between calls to startRule()/endRule() and
	 * only one rule can be under construction at any one time.
	 */
	//@{

	//! Starts the construction of a rule.
	/*! 
	 * \param t The type of the new rule.
	 * \param bound The lower bound (resp. min weight) of the rule to be created.
	 *
	 * \note the bound-parameter is only interpreted if the rule to be created is
	 * either a constraint- or a weight-rule.
	 */
	ProgramBuilder& startRule(RuleType t = BASICRULE, weight_t bound = -1) {
		rule_.clear();
		rule_.setType(t);
		if ((t == CONSTRAINTRULE || t == WEIGHTRULE) && bound > 0) {
			rule_.setBound(bound);
		}
		return *this;
	}

	//! Sets the bound (resp. min weight) of the currently active rule.
	/*!
	 * \param bound The lower bound (resp. min weight) of the rule to be created.
	 * \pre The rule under construction is either a constraint or weight rule.
	 */
	ProgramBuilder& setBound(weight_t bound) { // only valid for CONSTRAINT and WEIGHTRULES
		rule_.setBound(bound);
		return *this;
	}

	//! Adds the atom with the given id as a head to the currently active rule.
	ProgramBuilder& addHead(Var atomId) {
		assert(atomId > 0);
		rule_.addHead(atomId);
		return *this;
	}
	
	//! Adds a subgoal to the currently active rule.
	/*!
	 * \pre atomId > 0 && weight >= 0
	 * \param atomId The id of the atom to be added to the rule.
	 * \param pos true if the atom is positive. Fals otherwise
	 * \param weight The weight the new predecessor should have in the rule.
	 * \note The weight parameter is only used if the active rule is a weight or optimize rule.
	 */
	ProgramBuilder& addToBody(Var atomId, bool pos, weight_t weight = 1) {
		rule_.addToBody(atomId, pos, weight);
		return *this;
	}
	
	//! Finishes the construction of the active rule and adds it to the program.
	/*!
	 * \see ProgramBuilder::addRule(PrgRule&);
	 */
	ProgramBuilder& endRule() {
		return addRule(rule_);
	}
	
	//@}
	
	/*!
	 * \name Query functions
	 *
	 */
	 //@{

	//! Returns true if the program contains any minimize statements.
	bool   hasMinimize() const { return minimize_ != 0; }
	//! Returns the number of atoms in the logic program.
	uint32 numAtoms() const  { return (uint32)atoms_.size()-1; }
	//! Returns the number of bodies in the current (slice of the) logic program.
	uint32 numBodies() const { return (uint32)bodies_.size(); }
	//! Returns the id of the first atom of the current step.
	Var    startAtom() const { return incData_?incData_->startAtom_:1; }

	//! Returns the internal literal that is associated with the given atom.
	/*!
	 * \pre atomId is a known atom
	 * \return A literal that is valid in the current solving context. 
	 * \note Untill endProgram() is called, atoms from the current step are
	 *       associated with the always-false literal negLit(0).
	 * \throws std::logic_error if precondition is violated.
	 */
	Literal getLiteral(Var atomId) const;
	//! Returns a vector of internal literals that, when assumed true, makes all frozen atoms false. 
	/*!
	 * \pre The program is currently frozen. 
	 */
	void    getAssumptions(LitVec& out) const;
	PreproStats stats;
	//@}
	// FOR TESTING
	PrgAtomNode*  getAtom(Var atomId) const { return atoms_[atomId]; }
	PrgBodyNode*  getBody(Var bodyId) const { return bodies_[bodyId]; }
	Var           getEqAtom(Var a)    const { return getEqNode(atoms_, a);  }
	const LitVec& getCompute()        const { return compute_; }
private:
	ProgramBuilder(const ProgramBuilder&);
	ProgramBuilder& operator=(const ProgramBuilder&);
	friend class PrgRule;
	friend class PrgBodyNode;
	friend class PrgAtomNode;
	friend class Preprocessor;
	class CycleChecker;
	typedef PodVector<PrgRule*>::type RuleList;
	typedef std::multimap<uint32, uint32>   BodyIndex; // hash -> bodies[offset]
	typedef BodyIndex::iterator             IndexIter;
	typedef std::pair<IndexIter, IndexIter> BodyRange;
	typedef std::map<uint32, uint32>        EqNodes;
	typedef std::pair<PrgBodyNode*, uint32> Body;
	// ------------------------------------------------------------------------
	// Program definition
	PrgAtomNode*  resize(Var atomId);
	bool          inCompute(Literal x) const;
	void          addRuleImpl(const PrgRule& r, const PrgRule::RData& rd);
	bool          handleNatively(const PrgRule& r, const PrgRule::RData& rd) const;
	bool          transformNoAux(const PrgRule& r, const PrgRule::RData& rd) const;
	void          transformExtended();
	void          transformIntegrity(uint32 maxAux);
	void          clearRuleState(const PrgRule& r);
	Body          findOrCreateBody(const PrgRule& r, const PrgRule::RData& rd);
	bool          mergeEqAtoms(Var a, Var root);
	Var           getEqBody(Var b) const { return getEqNode(bodies_, b);}
	void          updateRule(PrgRule& r);
	template <class C>
	Var getEqNode(C& vec, Var id)  const {
		if (!vec[id]->eq()) return id;
		typedef typename C::value_type NodeType;
		NodeType n = vec[id];
		NodeType r;
		Var root   = n->eqNode();
		for (r = vec[root]; r->eq(); r = vec[root]) {
			// if n == r and r == r' -> n == r'
			n->setEq(root = r->eqNode());
		}
		return root;
	}
	void          setConflict();
	void          updateFrozenAtoms(const Solver&);
	void          normalize();
	// ------------------------------------------------------------------------
	// Statistics
	void upRules(RuleType r, int i)   { stats.rules[0] += i; stats.rules[r] += i; }
	void incTr(RuleType rt, uint32 n) { stats.updateTrStats(rt, n);               }
	void incTrAux(uint32 n)           { if (stats.trStats) { stats.trStats->auxAtoms += n; }}
	void incEqs(VarType t)            { stats.incEqs(t); }
	// ------------------------------------------------------------------------
	// ------------------------------------------------------------------------
	// Nogood creation
	void cloneVars(Solver& s);
	void minimize(Solver& s);
	bool addConstraints(Solver& s, CycleChecker& c);
	void freezeMinimize(Solver&);
	// ------------------------------------------------------------------------
	void writeRule(PrgBodyNode*, std::ostream&);

	PrgRule       rule_;        // active rule
	RuleState     ruleState_;   // which atoms appear in the active rule?
	RuleList      extended_;    // extended rules to be translated
	BodyIndex     bodyIndex_;   // hash -> body
	BodyList      bodies_;      // all bodies
	AtomList      atoms_;       // all atoms
	VarVec        initialSupp_; // bodies that are (initially) supported
	LitVec        compute_;     // atoms that are forced to true/false
	VarList       vars_;        // created vars
	struct MinimizeRule {
		WeightLitVec lits_;
		MinimizeRule* next_;
	} *minimize_;               // list of minimize-rules
	struct Incremental  {
		Incremental();
		uint32  startAtom_;       // first atom of current iteration
		uint32  startVar_;        // first var of the current iteration
		uint32  startAux_;        // first aux atom of current iteration
		uint32  startScc_;        // first scc of current iteration
		VarVec  freeze_;          // list of frozen atoms
		VarVec  unfreeze_;        // list of atom that are unfreezed in this iteration
	}* incData_;                // additional state to handle incrementally defined programs 
	AtomIndex*       atomIndex_;// atom symbol table
	typedef SingleOwnerPtr<DefaultUnfoundedCheck> Ufs;
	Ufs              ufs_;      // Unfounded set checker to use if program is not tight
	uint32           eqIters_;  // process body/atom equalities
	ExtendedRuleMode erMode_;   // How to handle extended rules?
	bool             eqDfs_;    // use df-order during eq-preprocessing?
	bool             normalize_;
	bool             frozen_;   // is the program currently frozen?

};


//! A node of a body-atom-dependency graph.
/*!
 * A node is either a body or an atom. Each node
 * has a literal and a value. Furthermore, once
 * strongly-connected components are identified, nodes
 * store their SCC-number. All trivial SCCs are represented
 * with the special Scc-number PrgNode::noScc. 
 */
class PrgNode {
public:
	static const uint32 noScc = (1U << 28) - 1;
	
	//! creates a new node
	/*!
	 * \note The ctor creates a node that corresponds to a literal that is false
	 */
	explicit PrgNode(Literal lit = negLit(sentVar)) 
		: lit_(lit), root_(0), value_(value_free), scc_(noScc), eq_(0), seen_(0), ignore_(0), frozen_(0) {}
	
	//! returns true if node has an associated variable in a solver
	/*!
	 * If hasVar() returns false, the node is not relevant and must not be used any further.
	 */
	bool    hasVar()    const { return lit_ != negLit(sentVar); }
	//! returns the variable associated with this node or sentVar if no var is associated with this node
	Var     var()       const { return lit_.var(); }
	//! returns the literal associated with this node or a sentinel literal if no var is associated with this node
	Literal literal()   const { return lit_; }

	//! returns the value currently assigned to this node
	ValueRep  value()     const { return value_; }
	
	//! returns the literal that must be true in order to fulfill the truth-value of this node
	/*!
	 * \note if value() == value_free, the special sentinel literal that is true
	 * in every solver is returned.
	 *
	 * \return
	 *  - if value() == value_free : posLit(sentVar)
	 *  - if value() == value_true or value_weak_true : literal()
	 *  - if value() == value_false: ~literal()
	 */
	Literal   trueLit()   const { 
		if (value_ != value_free) { return value_ == value_false ? ~lit_ : lit_; }
		return Literal();
	}
	
	//! returns the node's component number or PrgNode::noScc if node is trivially connected or sccs where not yet identified
	uint32    scc()       const { return scc_; }
	
	//! returns true if this node is equivalent to some other node
	/*!
	 * If eq() is true, the node is no longer relevant and must not be used any further.
	 * The only sensible operation is to call root() in order to get the id of this node's
	 * root-node.
	 */
	bool      eq()        const { return eq_ != 0; }
	
	//! returns true if this node should be ignored during SCC checking
	/*!
	 * \note for bodies only: if ignore() is true, the body is known to be false and no longer relevant
	 */
	bool      ignore()    const { return ignore_ != 0; }

	//! returns true if this atom is frozen, i.e. to be defined in a later step
	bool      frozen()    const { return frozen_ != 0; }

	//! returns the id of the node to which this node is equivalent
	/*! 
	 * \pre eq() is true
	 */
	Var       eqNode()    const { assert(eq()); return root_; }
	
	//! returns the id of this node in the unfounded set checker
	/*! 
	 * \pre eq() is false
	 */
	Var       ufsNode()   const { assert(!eq()); return root_; }
	
	void    setLiteral(Literal x)   { lit_ = x; }
	void    clearLiteral(bool clVal){ lit_ = negLit(sentVar); if (clVal) value_ = value_free; }
	bool    setValue(ValueRep v)    {
		if (v == value_weak_true && ignore() && !frozen()) { v = value_true; }
		if (value_ == value_free || v == value_ || (value_ == value_weak_true && v == value_true)) {
			value_ = v;
			return true;
		}
		return v == value_weak_true && value_ == value_true; 
	}
	bool    mergeValue(PrgNode* rhs){
		if (value_ != rhs->value()) {
			if (value() == value_false || value() == value_true) {
				return rhs->setValue(value());
			}
			return rhs->value() != value_free 
				? setValue(rhs->value())
				: rhs->setValue(value());
		}
		return true;
	}
	void    setIgnore(bool b)       { ignore_ = (uint32)b; }
	void    setFrozen(bool b)       { frozen_ = (uint32)b; }
	void    setEq(uint32 oId)       {
		root_   = oId;
		eq_     = 1;
		ignore_ = 1;
	}
	void    setUfsNode(uint32 nId, bool atom)  {
		assert(!eq() && (!atom || scc() != noScc));
		root_   = nId;
		if (atom) ignore_ = 1;
	}
	
	/*!
	 * \name SCC checking
	 * Intended to be used only by the SCC checker
	 */
	//@{
	uint32    dfsIdx()    const  { return root_;  }
	bool      visited()   const  { return seen_ != 0; }
	void      setVisited(bool v) { seen_   = (uint32)v; }
	void      setScc(uint32 scc) { assert(scc <= noScc); scc_ = scc; }
	void      setDfsIdx(uint32 r){ assert(!eq() && !ignore()); assert(r < (1U << 30)); root_ = r; }
	void      resetSccFlags()    {
		if (!eq()) {
			ignore_ = ignore_ && scc_ == noScc;
			scc_    = noScc;
			root_   = 0;
			seen_   = 0;
		}
	}
	//@}
protected:
	void assign(const PrgNode* other) {
		lit_   = other->lit_;
		root_  = other->root_;
		value_ = other->value_;
		scc_   = other->scc_;
		eq_    = other->eq_;
		seen_  = other->seen_;
		ignore_= other->ignore_;
		frozen_= other->frozen_;
	}
private:
	PrgNode(const PrgNode&);
	PrgNode& operator=(const PrgNode&);
	Literal lit_;         // associated literal in the solver
	uint32  root_   : 30; // used twofold:
	                      //  - relevant nodes (i.e. eq() == false):
	                      //     depth search index used to find root of a SCC and
	                      //     afterwards index of this node in the unfounded set checker (if relevant)
	                      //  - eq nodes (i.e. eq() == true):
	                      //     id of node to which this node is equivalent
	uint32  value_  :  2; // truth-value assigned to the node (either compute or derived during preprocessing)
	uint32  scc_    : 28; // component id of this node (noScc if trivially connected)
	uint32  eq_     :  1; // node is eq to some other node (id is stored in root_)
	uint32  seen_   :  1; // SCC-check: true if node was already visited
	uint32  ignore_ :  1; // ignore during scc check (node is false or has a support that is true)
	                      // For bodies: body is no longer relevant (e.g. one of its subgoals is known to be false)
	uint32  frozen_ :  1; // For atoms: frozen, i.e. defined in a later incremental step
};

//! A body-node represents a rule-body in a body-atom-dependency graph.
class PrgBodyNode : public PrgNode {
public:
	//! creates a new body node and connects the node to all its heads and predecessors
	/*!
	 * \param id      The id of the new body object
	 * \param rule    The rule for which this body object is created
	 * \param rInfo   The rule's simplification object
	 * \param prg     The program in which the new body is used
	 */
	static PrgBodyNode* create(uint32 id, const PrgRule& rule, const PrgRule::RData& rInfo, ProgramBuilder& prg);
	
	//! destroys a body node created via create(uint32 id, const PrgRule& rule, const PrgRule::RData& rInfo, ProgramBuilder& prg)
	void destroy();
	
	//! returns the rule type that is implemented with this body
	RuleType rtype() const;
	//! returns true if this body is the body of a choice rule
	bool isChoice() const { return type_ == CHOICE_BODY; }
	
	bool resetSupported() {
		if (!extended()) {
			return (extra_.unsupp = posSize()) == 0;
		}
		else if (!hasWeights()) {
			return (extra_.ext->unsupp = bound() - negSize()) <= 0;
		}
		else {
			weight_t snw = 0;
			for (uint32 i = 0; i != negSize(); ++i) {
				snw += weight(i, false);
			}
			return (extra_.ext->unsupp = bound() - snw) <= 0;
		}
	}

	//! returns true if the body node is supported.
	/*!
	 * A body of a basic- or choice-rule is supported, iff all of its positive subgoals are supported.
	 * A body of a weight-rule is supported if the sum of the weights of the supported positive +
	 * the sum of the negative weights is >= lowerBound().
	 */
	bool isSupported() const { return !extended() ? extra_.unsupp <= 0 : extra_.ext->unsupp <= 0; }

	//! notifies the body node about the fact that its positive subgoal v is supported
	/*!
	 * \return
	 *  - true if the body is now also supported
	 *  - false otherwise
	 *  .
	 */
	bool onPosPredSupported(Var /* v */);
	
	//! simplifies the body, i.e. its predecessors-lists
	/*!
	 * - removes true/false atoms from B+/B- resp.
	 * - removes/merges duplicate subgoals
	 * - checks whether body must be false (e.g. contains false/true atoms in B+/B- resp. or contains p and ~p)
	 * - computes a new hash value
	 *
	 * \param prg The program in which this body is used
	 * \param bodyId The body's id in the program
	 * \param[out] a pair of hash-values. hashes.first is the old hash value. hash.second the new hash value.
	 * \param pre The preprocessor that changed this body
	 * \param strong If true, treats atoms that have no variable associated as false. Otherwise
	 *               such atoms are ignored during simplification
	 * \note If body must be false, calls setValue(value_false) and sets the
	 * ignore flag to true.
	 * \return
	 *  - true if simplification was successful
	 *  - false if simplification detected a conflict
	 */
	bool simplifyBody(ProgramBuilder& prg, uint32 bodyId, std::pair<uint32, uint32>& hashes, Preprocessor& pre, bool strong);
	
	//! simplifies the heads of this body
	/*!
	 * Removes superfluous heads and sets the body to false if for some atom a
	 * in the head of this body, Ta -> FB. In that case, all heads atoms are removed, because
	 * a false body can't define an atom.
	 * If strong is true, removes head atoms that are not associated with a variable.
	 * \return 
	 *  - setValue(value_false) if setting a head of this body to true would
	 *  make the body false (i.e. the body is a selfblocker)
	 *  - true otherwise
	 */
	bool simplifyHeads(ProgramBuilder& prg, Preprocessor& pre, bool strong);

	bool backpropagate(ProgramBuilder& prg, LitVec& compute);

	//! adds the body-oriented nogoods as a set of constraints to the solver.
	/*
	 * \return false on conflict
	 */
	bool toConstraint(Solver&, ClauseCreator& c, const ProgramBuilder& prg);
	
	//! returns the bound of this body
	/*!
	 * \note The bound of a basic- or choice-rule is the equal to the size of the body
	 */
	weight_t bound() const;

	weight_t sumWeights() const;

	//! returns the number of atoms in the body
	uint32 size()    const { return size_; }
	//! returns the number of positive atoms in the body
	uint32 posSize() const { return posSize_; }
	//! returns the number of negative atoms in the body
	uint32 negSize() const { return size() - posSize(); }

	//! returns the idx'th positive subgoal
	/*! 
	 * \pre idx < posSize()
	 * \note first positive subgoal has index 0
	 */
	Var pos(uint32 idx) const { assert(idx < posSize()); return goals_[idx].var(); }

	//! returns the idx'th negative subgoal
	/*! 
	 * \pre idx < negSize()
	 * \note first negative subgoal has index 0
	 */
	Var neg(uint32 idx) const { assert(idx < negSize()); return goals_[posSize_+idx].var(); }

	Literal goal(uint32 idx) const { 
		assert(idx < size()); return goals_[idx];
	}

	//! returns the weight of the specified subgoal.
	/*!
	 * \param idx the index of the subgoal
	 * \param pos true for a positive, false for a negative subgoal
	 */
	weight_t weight(uint32 /* idx */, bool /* pos */) const;
	
	//! establishes set property for the heads of this body
	void buildHeadSet();
	
	//! returns true if x is a head of this body
	/*!
	 * \pre buildHeadSet() was called
	 **/
	bool hasHead(Var x) const {
		return std::binary_search(heads.begin(), heads.end(), x);
	}

	//! returns true if *this and other are equivalent w.r.t their predecessors
	bool equal(const PrgBodyNode& other) const;
	
	bool compatibleType(PrgBodyNode* other) const {
		return type_ == other->type_
			|| (!isChoice() && !other->isChoice());
	}
	void   sortBody();
	uint32 reinitDeps(uint32 id, ProgramBuilder& prg);
	VarVec    heads;          // successors of this body
private:
	PrgBodyNode(uint32 id, const PrgRule& rule, const PrgRule::RData& rInfo, ProgramBuilder& prg);
	~PrgBodyNode();
	PrgBodyNode(const PrgBodyNode&);
	PrgBodyNode& operator=(const PrgBodyNode&);
	enum BodyType { NORMAL_BODY = 0, CHOICE_BODY = 1, COUNT_BODY = 2, SUM_BODY = 3};
	struct Extended {
		static Extended* createExt(PrgBodyNode* self, uint32 bound, bool weights);
		void   destroy();
		weight_t  sumWeights;
		weight_t  bound;
		weight_t  unsupp;
		weight_t  weights[0];
	};
	struct Weights {
		Weights(const PrgBodyNode& self) : self_(&self) {}
		weight_t operator()(Literal p) const {
			if (self_->hasWeights()) {
				bool pos = p.sign() == false;
				for (uint32 i = pos?0:self_->posSize(), end = pos?self_->posSize():self_->size(); i != end; ++i) {
					if (self_->goals_[i].var() == p.var()) {
						return self_->extra_.ext->weights[i];
					}
				}
				assert(false);
			}
			return 1;
		}
		const PrgBodyNode* self_;
	};
	bool addPredecessorClauses(Solver& s, ClauseCreator& c, const AtomList&);
	bool      extended() const { return type_ > CHOICE_BODY; }
	bool      hasWeights()const{ return type_ == SUM_BODY; }
	weight_t  weight(uint32 idx) const { return !hasWeights() ? 1 : extra_.ext->weights[idx]; }
	weight_t  findWeight(Literal p, const AtomList& progAtoms) const;
	uint32    findLit(Literal p, const AtomList& progAtoms) const;
	
	uint32    size_;          // |B|
	uint32    posSize_  : 30; // |B+|
	uint32    type_     :  2; // body type
	union Extra {
		weight_t  unsupp;       // <= 0 -> body is supported
		Extended* ext;          // only used for extended rules
	} extra_;
	Literal   goals_[0];      // B+: [0, posSize_), B-: [posSize_, size_)
};

//! An atom-node in a body-atom-dependency graph
class PrgAtomNode : public PrgNode {
public:
	//! adds the atom-oriented nogoods to as a set of constraints to the solver.
	/*
	 * \return false on conflict
	 */
	bool toConstraint(Solver&, ClauseCreator& c, ProgramBuilder& prg);
	
	//! simplifies this atom, i.e. its predecessors-list
	/*!
	 * - removes false/irrelevant bodies
	 *
	 * \param atomId The id of this atom
	 * \param prg The program in which this atom is defined
	 * \param pre The preprocessor that changed this atom or its predecessors
	 * \param strong If true, updates bodies that were replaced with equivalent bodies
	 * \note If atom must be false, calls setValue(value_false) and sets the
	 * ignore flag to true.
	 * \return
	 *  - true if simplification was successful
	 *  - false if simplification detected a conflict
	 *  - if strong, the second return value is the number of different literals associated with the bodies of this atom
	 */
	typedef std::pair<bool, uint32> SimpRes;
	SimpRes simplifyBodies(Var atomId, ProgramBuilder& prg, bool strong);
	bool    hasPred(Var bodyId) const { return std::find(preds.begin(), preds.end(), bodyId) != preds.end(); }

	void    assign(const PrgAtomNode* a) {
		PrgNode::assign(a);
		posDep = a->posDep;
		negDep = a->negDep;
		preds  = a->preds;
	}

	VarVec    posDep;     // Bodies in which this atom occurs positively
	VarVec    negDep;     // Bodies in which this atom occurs negatively
	VarVec    preds;      // Bodies having this atom as head
};
//@}
}
#endif
