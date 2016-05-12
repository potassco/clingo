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
#ifdef _MSC_VER
#pragma warning(disable : 4996) // std::copy was declared deprecated
#endif

#include <clasp/dependency_graph.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <clasp/solve_algorithms.h>
#include <clasp/util/timer.h>
namespace Clasp {

/////////////////////////////////////////////////////////////////////////////////////////
// class SharedDependencyGraph
/////////////////////////////////////////////////////////////////////////////////////////
SharedDependencyGraph::SharedDependencyGraph(Configuration* cfg) : config_(cfg) {
	// add sentinal atom needed for disjunctions
	createAtom(negLit(0), PrgNode::noScc);
	VarVec adj;	adj.push_back(idMax);
	initAtom(sentinel_atom, 0, adj, 0);
	seenComponents_ = 0;
}

SharedDependencyGraph::~SharedDependencyGraph() {
	for (AtomVec::size_type i = 0; i != atoms_.size(); ++i) {
		delete [] atoms_[i].adj_;
	}
	for (AtomVec::size_type i = 0; i != bodies_.size(); ++i) {
		delete [] bodies_[i].adj_;
	}
	while (!components_.empty()) {
		delete components_.back().second;
		components_.pop_back();
	}
}

bool SharedDependencyGraph::relevantPrgAtom(const Solver& s, PrgAtom* a) const { 
	return !a->ignoreScc() && a->inUpper() && a->scc() != PrgNode::noScc && !s.isFalse(a->literal());
}
bool SharedDependencyGraph::relevantPrgBody(const Solver& s, PrgBody* b) const { 
	return !s.isFalse(b->literal()); 
}

// Creates a positive-body-atom-dependency graph (PBADG)
// The PBADG contains a node for each atom A of a non-trivial SCC and
// a node for each body B, s.th. there is a non-trivially connected atom A with
// B in body(A).
// Pre : b->seen() = 1 for all new and relevant bodies b
// Post: b->seen() = 0 for all bodies that were added to the PBADG
void SharedDependencyGraph::addSccs(LogicProgram& prg, const AtomList& sccAtoms, const NonHcfSet& nonHcfs) {
	// Pass 1: Create graph atom nodes and estimate number of bodies
	atoms_.reserve(atoms_.size() + sccAtoms.size());
	AtomList::size_type numBodies = 0;
	SharedContext& ctx = *prg.ctx();
	for (AtomList::size_type i = 0, end = sccAtoms.size(); i != end; ++i) {
		PrgAtom* a = sccAtoms[i];
		if (relevantPrgAtom(*ctx.master(), a)) {
			// add graph atom node and store link between program node and graph node for later lookup
			a->resetId(createAtom(a->literal(), a->scc()), true);
			// atom is defined by more than just a bunch of clauses
			ctx.setFrozen(a->var(), true);
			numBodies += a->supports();
		}
	}
	// Pass 2: Init atom nodes and create body nodes
	VarVec adj, ext;
	bodies_.reserve(bodies_.size() + numBodies/2);
	PrgBody* prgBody; PrgDisj* prgDis;
	for (AtomList::size_type i = 0, end = sccAtoms.size(); i != end; ++i) {
		PrgAtom*   a  = sccAtoms[i];
		if (relevantPrgAtom(*ctx.master(), a)) { 
			uint32 prop = 0;
			for (PrgAtom::sup_iterator it = a->supps_begin(), endIt = a->supps_end(); it != endIt; ++it) {
				assert(it->isBody() || it->isDisj());
				NodeId bId= PrgNode::maxVertex;
				if (it->isBody() && !it->isGamma()) {
					prgBody = prg.getBody(it->node());
					bId     = relevantPrgBody(*ctx.master(), prgBody) ? addBody(prg, prgBody) : PrgNode::maxVertex;
				}
				else if (it->isDisj()) {
					prgDis  = prg.getDisj(it->node());
					bId     = addDisj(prg, prgDis);
					prop   |= AtomNode::property_in_disj;
					ctx.setInDisj(a->var(), true);
				}
				if (bId != PrgNode::maxVertex) {
					if (!bodies_[bId].seen()) {
						bodies_[bId].seen(true);
						adj.push_back(bId);
					}
					if (it->isChoice()) {
						// mark atom as in choice
						prop |= AtomNode::property_in_choice;
					}
				}
			}
			uint32 nPred= (uint32)adj.size();
			for (PrgAtom::dep_iterator it = a->deps_begin(), endIt = a->deps_end(); it != endIt; ++it) {
				if (!it->sign()) {
					prgBody = prg.getBody(it->var());
					if (relevantPrgBody(*ctx.master(), prgBody) && prgBody->scc(prg) == a->scc()) {
						NodeId bodyId = addBody(prg, prgBody);
						if (!bodies_[bodyId].extended()) {
							adj.push_back(bodyId);
						}
						else {
							ext.push_back(bodyId);
							ext.push_back(bodies_[bodyId].get_pred_idx(a->id()));
							assert(bodies_[bodyId].get_pred(ext.back()) == a->id());
							prop |= AtomNode::property_in_ext;
						}
					}
				}
			}
			if (!ext.empty()) {
				adj.push_back(idMax);
				adj.insert(adj.end(), ext.begin(), ext.end());
			}
			adj.push_back(idMax);
			initAtom(a->id(), prop, adj, nPred);
			adj.clear(); ext.clear();
		}
	}	
	// "update" existing non-hcf components
	for (NonHcfIter it = nonHcfBegin(), end = nonHcfEnd(); it != end; ++it) {
		it->second->update(ctx);
	}
	// add new non-hcf components
	for (NonHcfSet::const_iterator it = nonHcfs.begin() + seenComponents_, end = nonHcfs.end(); it != end; ++it) {
		addNonHcf(ctx, *it);
	}
	seenComponents_ = nonHcfs.size();
}

uint32 SharedDependencyGraph::createAtom(Literal lit, uint32 aScc) {
	NodeId id    = (uint32)atoms_.size();
	atoms_.push_back(AtomNode());
	AtomNode& ua = atoms_.back();
	ua.lit       = lit;
	ua.scc       = aScc;
	return id;
}

void SharedDependencyGraph::initAtom(uint32 id, uint32 prop, const VarVec& adj, uint32 numPreds) {
	AtomNode& ua = atoms_[id];
	ua.setProperties(prop);
	ua.adj_      = new NodeId[adj.size()];
	ua.sep_      = ua.adj_ + numPreds;
	NodeId* sExt = ua.adj_;
	NodeId* sSame= sExt + numPreds;
	uint32  aScc = ua.scc;
	for (VarVec::const_iterator it = adj.begin(), end = adj.begin()+numPreds; it != end; ++it) {
		BodyNode& bn = bodies_[*it];
		if (bn.scc != aScc) { *sExt++ = *it; }
		else                { *--sSame= *it; }
		bn.seen(false);
	}
	std::reverse(sSame, ua.adj_ + numPreds);
	std::copy(adj.begin()+numPreds, adj.end(), ua.sep_);
}

uint32 SharedDependencyGraph::createBody(PrgBody* b, uint32 bScc) {
	NodeId id = (uint32)bodies_.size();
	bodies_.push_back(BodyNode(b, bScc));
	return id;
}

// Creates and initializes a body node for the given body b.
uint32 SharedDependencyGraph::addBody(const LogicProgram& prg, PrgBody* b) {
	if (b->seen()) {     // first time we see this body - 
		VarVec preds, atHeads;
		uint32 bScc  = b->scc(prg);
		NodeId bId   = createBody(b, bScc);
		addPreds(prg, b, bScc, preds);
		addHeads(prg, b, atHeads);
		initBody(bId, preds, atHeads);
		b->resetId(bId, false);
		prg.ctx()->setFrozen(b->var(), true);
	}
	return b->id();
}

// Adds all relevant predecessors of b to preds.
// The returned list looks like this:
// [[B], a1, [w1], ..., aj, [wj], idMax, l1, [w1], ..., lk, [wk], idMax], where
// B is the bound of b (only for card/weight rules),
// ai is a positive predecessor from bScc,
// wi is the weight of ai (only for weight rules), and
// li is a literal of a subgoal from some other scc (only for cardinality/weight rules)
void SharedDependencyGraph::addPreds(const LogicProgram& prg, PrgBody* b, uint32 bScc, VarVec& preds) const {
	if (bScc == PrgNode::noScc) { preds.clear(); return; }
	const bool weights = b->type() == Asp::BodyInfo::SUM_BODY;
	for (uint32 i = 0; i != b->size() && !b->goal(i).sign(); ++i) {
		PrgAtom* pred = prg.getAtom(b->goal(i).var());
		if (relevantPrgAtom(*prg.ctx()->master(), pred) && pred->scc() == bScc) {
			preds.push_back( pred->id() );
			if (weights) { preds.push_back(b->weight(i)); }
		}
	}
	if (b->type() != Asp::BodyInfo::NORMAL_BODY) {
		preds.insert(preds.begin(), b->bound());
		preds.push_back(idMax);
		for (uint32 n = 0; n != b->size(); ++n) {
			PrgAtom* pred = prg.getAtom(b->goal(n).var());
			bool     ext  = b->goal(n).sign() || pred->scc() != bScc;
			Literal lit   = b->goal(n).sign() ? ~pred->literal() : pred->literal();
			if (ext && !prg.ctx()->master()->isFalse(lit)) {
				preds.push_back(lit.asUint());
				if (weights) { preds.push_back(b->weight(n)); }
			}
		}
	}
	preds.push_back(idMax);
}

// Splits the heads of b into atoms and disjunctions.
// Disjunctions are flattened to sentinel-enclose datom-lists.
uint32 SharedDependencyGraph::addHeads(const LogicProgram& prg, PrgBody* b, VarVec& heads) const {
	for (PrgBody::head_iterator it = b->heads_begin(), end = b->heads_end(); it != end; ++it) {
		if (it->isAtom() && !it->isGamma()) {
			PrgAtom* a = prg.getAtom(it->node());
			if (relevantPrgAtom(*prg.ctx()->master(), a)) {
				heads.push_back(a->id());
			}
		}
		else if (it->isDisj()) {
			assert(prg.getDisj(it->node())->inUpper() && prg.getDisj(it->node())->supports() == 1);
			PrgDisj* d = prg.getDisj(it->node());
			// flatten disjunction and enclose in sentinels
			heads.push_back(sentinel_atom);
			getAtoms(prg, d, heads);
			heads.push_back(sentinel_atom);
		}
	}
	return heads.size();
}

// Adds the atoms from the given disjunction to atoms and returns the disjunction's scc.
uint32 SharedDependencyGraph::getAtoms(const LogicProgram& prg, PrgDisj* d, VarVec& atoms) const {
	uint32 scc = PrgNode::noScc;
	for (PrgDisj::atom_iterator it = d->begin(), end = d->end(); it != end; ++it) {
		PrgAtom* a = prg.getAtom(it->node());
		if (relevantPrgAtom(*prg.ctx()->master(), a)) {
			assert(scc == PrgNode::noScc || scc == a->scc());
			atoms.push_back(a->id());
			scc = a->scc();
		}
	}
	return scc;
}

// Initializes preds and succs lists of the body node with the given id.
void SharedDependencyGraph::initBody(uint32 id, const VarVec& preds, const VarVec& atHeads) {
	BodyNode* bn = &bodies_[id];
	uint32 nSuccs= atHeads.size();
	bn->adj_     = new NodeId[nSuccs + preds.size()];
	bn->sep_     = bn->adj_ + nSuccs;
	NodeId* sSame= bn->adj_;
	NodeId* sExt = sSame + nSuccs;
	uint32  bScc = bn->scc;
	uint32  hScc = PrgNode::noScc;
	uint32  disj = 0;
	for (VarVec::const_iterator it = atHeads.begin(), end = atHeads.end(); it != end;) {
		if (*it) {
			hScc = getAtom(*it).scc;
			if (hScc == bScc) { *sSame++ = *it++; }
			else              { *--sExt  = *it++; }
		}
		else {
			hScc = getAtom(it[1]).scc; ++disj;
			if (hScc == bScc) { *sSame++ = *it++; while ( (*sSame++ = *it++) ) { ; } }
			else              { *--sExt  = *it++; while ( (*--sExt  = *it++) ) { ; } }
		}
	}
	std::copy(preds.begin(), preds.end(), bn->sep_);
	bn->sep_ += bn->extended();
	if (disj) { bodies_[id].data |= BodyNode::flag_has_delta; }
}

uint32 SharedDependencyGraph::addDisj(const LogicProgram& prg, PrgDisj* d) {
	assert(d->inUpper() && d->supports() == 1);
	if (d->seen()) { // first time we see this disjunction
		PrgBody* prgBody = prg.getBody(d->supps_begin()->node());
		uint32   bId     = PrgNode::maxVertex;
		if (relevantPrgBody(*prg.ctx()->master(), prgBody)) {
			bId = addBody(prg, prgBody);
		}
		d->resetId(bId, false);
	}
	return d->id();
}

void SharedDependencyGraph::addNonHcf(SharedContext& ctx, uint32 scc) {
	VarVec sccAtoms, sccBodies;
	// get all atoms from scc
	for (uint32 i = 0; i != numAtoms(); ++i) {
		if (getAtom(i).scc == scc) { 
			sccAtoms.push_back(i);
			atoms_[i].set(AtomNode::property_in_non_hcf);
		}
	}
	// get all bodies defining an atom in scc
	const Solver& generator = *ctx.master(); (void)generator;
	for (uint32 i = 0; i != sccAtoms.size(); ++i) {
		const AtomNode& a = getAtom(sccAtoms[i]);
		for (const NodeId* bodyIt = a.bodies_begin(), *bodyEnd = a.bodies_end(); bodyIt != bodyEnd; ++bodyIt) {
			BodyNode& B = bodies_[*bodyIt];
			if (!B.seen()) {
				assert(generator.value(B.lit.var()) != value_free || !generator.seen(B.lit));
				sccBodies.push_back(*bodyIt);
				B.seen(true);
			}
		}
	}
	for (uint32 i = 0; i != sccBodies.size(); ++i) { bodies_[sccBodies[i]].seen(false); }
	components_.push_back( ComponentPair(scc, new NonHcfComponent(*this, ctx, scc, sccAtoms, sccBodies)) );
}
void SharedDependencyGraph::accuStats() const {
	for (NonHcfIter it = nonHcfBegin(), end = nonHcfEnd(); it != end; ++it) {
		it->second->prg_->accuStats();
	}
}
void SharedDependencyGraph::simplify(const Solver& s) {
	ComponentMap::iterator j = components_.begin();
	for (ComponentMap::iterator it = components_.begin(), end = components_.end(); it != end; ++it) {
		if (it->second->simplify(it->first, s)) { *j++ = *it; }
		else                                    { delete it->second; }
	}
	components_.erase(j, components_.end());
}
/////////////////////////////////////////////////////////////////////////////////////////
// class SharedDependencyGraph::NonHcfComponent::ComponentMap
/////////////////////////////////////////////////////////////////////////////////////////
class SharedDependencyGraph::NonHcfComponent::ComponentMap {
public:
	ComponentMap() { static_assert(sizeof(Mapping) == sizeof(uint64), "Invalid padding!"); }
	struct Mapping {
		explicit Mapping(NodeId id) : node(id), varOff(0), varUsed(0), isEq(0) { }
		uint32 node;        // node id in dep-graph of generator program P
		uint32 varOff : 30; // var offset in tester solver
		uint32 varUsed:  1; // has var in tester solver?
		uint32 isEq   :  1; // node literal in P is eq to some other node literal in P
		Var     var()       const { return varOff * varUsed; }
		Literal atPos()     const { return posLit(var()); }
		Literal atAux()     const { return posLit(varOff+1); }
		Literal atUnf()     const { return posLit(varOff+2); }
		Literal bodyAux()   const { return posLit(var()); }
		bool operator<(const Mapping& other) const { return node < other.node; }
	};
	typedef  SharedDependencyGraph       SccGraph;
	typedef  PodVector<Mapping>::type    NodeMap;
	typedef  NodeMap::iterator           MapIt;
	typedef  NodeMap::const_iterator     MapIt_c;
	typedef  std::pair<MapIt_c, MapIt_c> MapRange;
	void     addVars(Solver& generator, const SccGraph& dep, const VarVec& atoms, const VarVec& bodies, SharedContext& out);
	void     addAtomConstraints(SharedContext& out);
	void     addBodyConstraints(const Solver& generator, const SccGraph& dep, uint32 scc, SharedContext& out);
	void     mapGeneratorAssignment(const Solver& generator, const SccGraph& dep, LitVec& out) const;
	void     mapTesterModel(const Solver& tester, VarVec& out) const;
	bool     simplify(const Solver& generator, Solver& tester);
	MapRange atoms() const { return MapRange(mapping.begin(), mapping.begin() + numAtoms); }
	MapRange bodies()const { return MapRange(mapping.begin() + numAtoms, mapping.end()); }
	MapIt_c  findAtom(NodeId nodeId) const { return std::lower_bound(mapping.begin(), mapping.begin()+numAtoms, Mapping(nodeId)); }
	NodeMap  mapping; // maps nodes of P to literals in C;
	uint32   numAtoms;// number of atoms
};
// Adds necessary variables for all atoms and bodies to the component program.
// Input-Vars: (set via assumptions)
//  a+: for each atom a, a+ is true iff a is true in P
//  B*: for each body B, B* is true iff B is not false in P
// Aux-Var: (derived)
//  a*: for each atom a, a* is true iff not a+ OR au
// Output: (unfounded sets)
//  au: for each atom a, au is true iff a is unfounded w.r.t to assignment of P.
void SharedDependencyGraph::NonHcfComponent::ComponentMap::addVars(Solver& generator, const SccGraph& dep, const VarVec& atoms, const VarVec& bodies, SharedContext& comp) {
	mapping.reserve(atoms.size() + bodies.size());
	for (VarVec::const_iterator it = atoms.begin(), end = atoms.end(); it != end; ++it) { 
		const AtomNode& at = dep.getAtom(*it);
		Literal gen        = at.lit;
		if (generator.isFalse(gen)) { continue; }
		Mapping map(*it);
		map.varUsed = !generator.isTrue(gen) || generator.level(gen.var()) > 0;
		map.varOff  = map.varUsed ? comp.addVar(Var_t::atom_var) : comp.numVars();
		// add additional vars for au and ax
		comp.addVar(Var_t::atom_var);
		comp.addVar(Var_t::atom_var);
		comp.setFrozen(map.atUnf().var(), true);
		comp.setFrozen(map.atPos().var(), true);
		mapping.push_back(map);
	}
	numAtoms = (uint32)mapping.size();
	std::stable_sort(mapping.begin(), mapping.end());
	// add necessary vars for bodies 
	for (VarVec::const_iterator it = bodies.begin(), end = bodies.end(); it != end; ++it) {
		Literal gen = dep.getBody(*it).lit;
		if (generator.isFalse(gen))  { continue; }
		Mapping map(*it);
		map.varUsed = !generator.isTrue(gen) || generator.level(gen.var()) > 0;
		if (map.varUsed && !generator.seen(gen)) {
			map.varOff= comp.addVar(Var_t::atom_var);
			generator.markSeen(gen);
		}
		else { // eq to TRUE or existing body
			map.isEq  = 1;
			map.varOff= comp.numVars()+1;
			if (map.varUsed) {
				for (MapRange r = this->bodies(); r.first != r.second;) {
					--r.second;
					if (dep.getBody(r.second->node).lit == gen) {
						map.varOff = r.second->varOff;
						break;
					}
				}
				assert(map.varOff <= comp.numVars());
			}
		}
		comp.setFrozen(map.bodyAux().var(), true);
		mapping.push_back(map);
	}
	for (MapRange r = this->bodies(); r.first != r.second; ++r.first) {
		if (!r.first->isEq) {
			Var v = dep.getBody(r.first->node).lit.var();
			generator.clearSeen(v);
		}
	}
}

// Adds constraints stemming from the given atoms to the component program.
// 1. [au(a0) v ... v au(an-1)], where 
//   - ai is an atom in P from the given atom set, and 
//   - au(ai) is the corresponding output-atom in the component program C.
// 2. For each atom ai in atom set, [ax(ai) <=> ~ap(ai) v au(ai)], where
//   ap(ai), ax(ai), au(ai) are the input, aux resp. output atoms in C.
void SharedDependencyGraph::NonHcfComponent::ComponentMap::addAtomConstraints(SharedContext& comp) {
	ClauseCreator cc1(comp.master()), cc2(comp.master());
	cc1.addDefaultFlags(ClauseCreator::clause_force_simplify);
	cc1.start();
	for (MapRange r = atoms(); r.first != r.second; ++r.first) {
		const Mapping& m = *r.first;
		cc1.add(m.atUnf());
		cc2.start().add(~m.atPos()).add(m.atUnf()).add(~m.atAux()).end(); // [~a+ v au v ~a*]
		cc2.start().add(m.atAux()).add(m.atPos()).end();  // [a* v a+]
		cc2.start().add(m.atAux()).add(~m.atUnf()).end(); // [a* v ~au]
	}
	cc1.end();
}

// Adds constraints stemming from the given bodies to the component program.
// For each atom ai and rule a0 | ai | ...| an :- B, s.th. B in bodies
//  [~au(ai) v ~bc(B) V ~ax(aj), j != i V au(p), p in B+ ^ C], where
// ax(ai), au(ai) is the aux resp. output atom of ai in C.
void SharedDependencyGraph::NonHcfComponent::ComponentMap::addBodyConstraints(const Solver& generator, const SccGraph& dep, uint32 scc, SharedContext& comp) {
	ClauseCreator cc(comp.master()); 
	cc.addDefaultFlags(ClauseCreator::clause_force_simplify);
	ClauseCreator dc(comp.master());
	MapIt j = mapping.begin() + numAtoms;
	for (MapRange r = bodies(); r.first != r.second; ++r.first) {
		const BodyNode& B = dep.getBody(r.first->node);
		if (generator.isFalse(B.lit)) { continue; }
		if (B.extended())             { throw std::runtime_error("Extended bodies not supported - use '--trans-ext=weight'"); }
		for (const NodeId* hIt = B.heads_begin(), *hEnd = B.heads_end(); hIt != hEnd; ++hIt) {
			uint32 hScc = *hIt ? dep.getAtom(*hIt).scc : dep.getAtom(hIt[1]).scc;
			if (hScc != scc) { 
				// the head is not relevant to this non-hcf - skip it
				if (!*hIt) { do { ++hIt; } while (*hIt); }
				continue;
			}
			// [~B* v ~au V ~o* for all o != a in B.disHead V bu for each b in B+ ^ C]
			cc.start().add(~r.first->bodyAux());
			if (B.scc == scc) { // add subgoals from same scc
				for (const NodeId* aIt = B.preds(); *aIt != idMax; ++aIt) {
					MapIt_c atMapped = findAtom(*aIt);
					cc.add(atMapped->atUnf());
				}
			}
			if (*hIt) { // normal head
				MapIt_c atMapped = findAtom(*hIt);
				assert(atMapped != atoms().second);
				cc.add(~atMapped->atUnf());
				cc.end();
			}
			else { // disjunctive head
				const NodeId* dHead = ++hIt;
				for (; *hIt; ++hIt) {
					dc.start();
					dc = cc;
					MapIt_c atMapped = findAtom(*hIt);
					dc.add(~atMapped->atUnf());
					for (const NodeId* other = dHead; *other; ++other) {
						if (*other != *hIt) {
							assert(dep.getAtom(*other).scc == scc);
							atMapped = findAtom(*other);
							dc.add(~atMapped->atAux());
						}
					}
					dc.end();
				}
			}
		}
		if (r.first->isEq == 0) { *j++ = *r.first; }
	}
	mapping.erase(j, mapping.end());
}

// Maps the generator assignment given in s to a list of tester assumptions.
void SharedDependencyGraph::NonHcfComponent::ComponentMap::mapGeneratorAssignment(const Solver& s, const SccGraph& dep, LitVec& assume) const {
	Literal  gen;
	assume.clear(); assume.reserve(mapping.size());
	for (MapRange r = atoms(); r.first != r.second; ++r.first) {
		const Mapping& at = *r.first;
		assert(at.varUsed || at.atPos() == posLit(0));
		if (!at.varUsed) { continue; }
		gen = dep.getAtom(at.node).lit;
		assume.push_back(at.atPos() ^ (!s.isTrue(gen)));
		if (s.isFalse(gen)) { assume.push_back(~at.atUnf()); }
	}
	for (MapRange r = bodies(); r.first != r.second; ++r.first) {
		gen = dep.getBody(r.first->node).lit;
		assume.push_back(r.first->bodyAux() ^ s.isFalse(gen));
	}
}
// Maps the tester model given in s back to a list of unfounded atoms in the generator.
void SharedDependencyGraph::NonHcfComponent::ComponentMap::mapTesterModel(const Solver& s, VarVec& out) const {
	assert(s.numFreeVars() == 0);
	out.clear();
	for (MapRange r = atoms(); r.first != r.second; ++r.first) {
		if (s.isTrue(r.first->atUnf())) {
			out.push_back(r.first->node);
		}
	}
}
bool SharedDependencyGraph::NonHcfComponent::ComponentMap::simplify(const Solver& generator, Solver& tester) {
	if (!tester.popRootLevel(UINT32_MAX)) { return false; }
	const SharedDependencyGraph& dep = *generator.sharedContext()->sccGraph;
	const bool rem = !tester.sharedContext()->isShared();
	MapIt j        = rem ? mapping.begin() : mapping.end();
	for (MapIt_c it = mapping.begin(), aEnd = it + numAtoms, end = mapping.end(); it != end; ++it) {
		const Mapping& m = *it;
		const bool  atom = it < aEnd;
		Literal        g = atom ? dep.getAtom(m.node).lit : dep.getBody(m.node).lit;
		if (!m.varUsed || generator.topValue(g.var()) == value_free) {
			if (rem) { *j++ = m; }
			continue;
		}
		bool isFalse = generator.isFalse(g);
		bool ok      = atom || tester.force(m.bodyAux() ^ isFalse);
		if (atom) {
			if   (!isFalse) { ok = tester.force(m.atPos()); if (ok && rem) { *j++ = m; } }
			else            { ok = tester.force(~m.atPos()) && tester.force(~m.atUnf()); numAtoms -= (ok && rem); }
		}
		if (!ok) {
			if (rem) { j = std::copy(it, end, j); }
			break;
		}
	}
	mapping.erase(j, mapping.end());
	return tester.simplify();
}
/////////////////////////////////////////////////////////////////////////////////////////
// class SharedDependencyGraph::NonHcfComponent
/////////////////////////////////////////////////////////////////////////////////////////
SolveTestEvent::SolveTestEvent(const Solver& s, uint32 a_scc, bool part) 
	: SolveEvent<SolveTestEvent>(s, Event::verbosity_max)
	, result(-1), scc(a_scc), partial(part) {
	confDelta   = s.stats.conflicts;
	choiceDelta = s.stats.choices;
	time        = 0.0;
}
uint64 SolveTestEvent::choices() const {
	return solver->stats.choices - choiceDelta;
}
uint64 SolveTestEvent::conflicts() const {
	return solver->stats.conflicts - confDelta;
}

SharedDependencyGraph::NonHcfComponent::NonHcfComponent(const SharedDependencyGraph& dep, SharedContext& genCtx, uint32 scc, const VarVec& atoms, const VarVec& bodies) 
	: prg_(new SharedContext())
	, comp_(new ComponentMap()){
	Solver& generator = *genCtx.master();
	prg_->setConcurrency(genCtx.concurrency(), SharedContext::mode_reserve);
	prg_->setConfiguration(dep.nonHcfConfig(), false);
	comp_->addVars(generator, dep, atoms, bodies, *prg_);
	prg_->startAddConstraints();
	comp_->addAtomConstraints(*prg_);
	comp_->addBodyConstraints(generator, dep, scc, *prg_);
	prg_->enableStats(generator.stats.level());
	prg_->endInit(true);
}

SharedDependencyGraph::NonHcfComponent::~NonHcfComponent() { 
	delete prg_;
	delete comp_;
}

void SharedDependencyGraph::NonHcfComponent::update(const SharedContext& generator) {
	prg_->enableStats(generator.master()->stats.level());
	for (uint32 i = 0; generator.hasSolver(i); ++i) {
		if (!prg_->hasSolver(i)) { prg_->attach(prg_->addSolver());   }
		else                     { prg_->initStats(*prg_->solver(i)); }
	}
}

void SharedDependencyGraph::NonHcfComponent::assumptionsFromAssignment(const Solver& s, LitVec& assume) const {
	assert(s.sharedContext()->sccGraph.get() != 0);
	comp_->mapGeneratorAssignment(s, *s.sharedContext()->sccGraph, assume);
}

bool SharedDependencyGraph::NonHcfComponent::test(uint32 scc, const Solver& generator, const LitVec& assume, VarVec& unfoundedOut) const {
	assert(generator.id() < prg_->concurrency() && "Invalid id!");
	// Forwards to message handler of generator so that messages are 
	// handled during long running tests.
	struct Tester : MessageHandler {
		Tester(Solver& s, MessageHandler* gen) : solver(&s), generator(gen) { if (gen) { s.addPost(this); } }
		~Tester() { if (generator) { solver->removePost(this); } }
		bool handleMessages()                            { return generator->handleMessages(); }
		bool propagateFixpoint(Solver&, PostPropagator*) { return Tester::handleMessages() || !terminate(); }
		bool terminate()                                 { solver->setStopConflict(); return true; }
		int test(const LitVec& assume) {
			return int(BasicSolve(*solver).satisfiable(assume, solver->stats.choices == 0) == false);
		}
		Solver*         solver;
		MessageHandler* generator;
	} tester(*prg_->solver(generator.id()), static_cast<MessageHandler*>(generator.getPost(PostPropagator::priority_reserved_msg)));
	SolveTestEvent ev(*tester.solver, scc, generator.numFreeVars() != 0);
	tester.solver->stats.addTest(ev.partial);
	generator.sharedContext()->report(ev);
	ev.time = ThreadTime::getTime();
	if ((ev.result = tester.test(assume)) == 0) {
		tester.solver->stats.addModel(tester.solver->decisionLevel());
		comp_->mapTesterModel(*tester.solver, unfoundedOut); 
	}
	ev.time = ThreadTime::getTime() - ev.time;
	tester.solver->stats.addCpuTime(ev.time);
	generator.sharedContext()->report(ev);
	return ev.result != 0;
}
bool SharedDependencyGraph::NonHcfComponent::simplify(uint32, const Solver& s) const {
	assert(s.sharedContext()->sccGraph.get() != 0);
	return comp_->simplify(s, *prg_->solver(s.id()));
}
}
