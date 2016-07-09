//
// Copyright (c) 2013-2016, Benjamin Kaufmann
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
#include <clasp/logic_program.h>
#include <clasp/shared_context.h>
#include <clasp/solver.h>
#include <clasp/minimize_constraint.h>
#include <clasp/util/misc_types.h>
#include <clasp/asp_preprocessor.h>
#include <clasp/clause.h>
#include <clasp/dependency_graph.h>
#include <clasp/parser.h>
#include <potassco/theory_data.h>
#include <stdexcept>
#include <cctype>
#include <cstdio>
namespace Clasp { namespace Asp {
/////////////////////////////////////////////////////////////////////////////////////////
// Statistics
/////////////////////////////////////////////////////////////////////////////////////////
#define RK(x) RuleStats::x
const char* RuleStats::toStr(int k) {
	CLASP_FAIL_IF(k < 0 || uint32(k) > numKeys(), "Invalid key");
	switch (k) {
		case Normal   : return "Normal";
		case Choice   : return "Choice";
		case Minimize : return "Minimize";
		case Acyc     : return "Acyc";
		case Heuristic: return "Heuristic";
		default:        return "None";
	}
}
uint32 RuleStats::sum() const {
	return std::accumulate(key, key + numKeys(), uint32(0));
}
const char* BodyStats::toStr(int t) {
	CLASP_FAIL_IF(t < 0 || uint32(t) >= numKeys(), "Invalid body type!");
	switch (t) {
		default           : return "Normal";
		case Body_t::Count: return "Count";
		case Body_t::Sum  : return "Sum";
	}
}
uint32 BodyStats::sum() const {
	return std::accumulate(key, key + numKeys(), uint32(0));
}

namespace {
template <unsigned i>
double sumBodies(const LpStats* self) { return self->bodies[i].sum(); }
template <unsigned i>
double sumRules(const LpStats* self) { return self->rules[i].sum(); }
double sumEqs(const LpStats* self)   { return self->eqs(); }
}
#define LP_STATS( APPLY )                                        \
	APPLY("atoms"               , VALUE(atoms))                  \
	APPLY("atoms_aux"           , VALUE(auxAtoms))               \
	APPLY("disjunctions"        , VALUE(disjunctions[0]))        \
	APPLY("disjunctions_non_hcf", VALUE(disjunctions[1]))        \
	APPLY("bodies"              , FUNC(sumBodies<0>))            \
	APPLY("bodies_tr"           , FUNC(sumBodies<1>))            \
	APPLY("sum_bodies"          , VALUE(bodies[0][Body_t::Sum])) \
	APPLY("sum_bodies_tr"       , VALUE(bodies[1][Body_t::Sum])) \
	APPLY("count_bodies"        , VALUE(bodies[0][Body_t::Count]))\
	APPLY("count_bodies_tr"     , VALUE(bodies[1][Body_t::Count]))\
	APPLY("sccs"                , VALUE(sccs))                   \
	APPLY("sccs_non_hcf"        , VALUE(nonHcfs))                \
	APPLY("gammas"              , VALUE(gammas))                 \
	APPLY("ufs_nodes"           , VALUE(ufsNodes))               \
	APPLY("rules"               , FUNC(sumRules<0>))             \
	APPLY("rules_normal"        , VALUE(rules[0][RK(Normal)]))   \
	APPLY("rules_choice"        , VALUE(rules[0][RK(Choice)]))   \
	APPLY("rules_minimize"      , VALUE(rules[0][RK(Minimize)])) \
	APPLY("rules_acyc"          , VALUE(rules[0][RK(Acyc)]))     \
	APPLY("rules_heuristic"     , VALUE(rules[0][RK(Heuristic)]))\
	APPLY("rules_tr"            , FUNC(sumRules<1>))             \
	APPLY("rules_tr_normal"     , VALUE(rules[1][RK(Normal)]))   \
	APPLY("rules_tr_choice"     , VALUE(rules[1][RK(Choice)]))   \
	APPLY("rules_tr_minimize"   , VALUE(rules[1][RK(Minimize)])) \
	APPLY("rules_tr_acyc"       , VALUE(rules[1][RK(Acyc)]))     \
	APPLY("rules_tr_heuristic"  , VALUE(rules[1][RK(Heuristic)]))\
	APPLY("eqs"                 , FUNC(sumEqs))                  \
	APPLY("eqs_atom"            , VALUE(eqs_[Var_t::Atom-1]))    \
	APPLY("eqs_body"            , VALUE(eqs_[Var_t::Body-1]))    \
	APPLY("eqs_other"           , VALUE(eqs_[Var_t::Hybrid-1]))

void LpStats::reset() {
	std::memset(this, 0, sizeof(LpStats));
}

void LpStats::accu(const LpStats& o) {
	atoms    += o.atoms;
	auxAtoms += o.auxAtoms;
	ufsNodes += o.ufsNodes;
	if (sccs == PrgNode::noScc || o.sccs == PrgNode::noScc) {
		sccs    = o.sccs;
		nonHcfs = o.nonHcfs;
	}
	else {
		sccs   += o.sccs;
		nonHcfs+= o.nonHcfs;
	}
	for (int i = 0; i != 2; ++i) {
		disjunctions[i] += o.disjunctions[i];
		for (uint32 k = 0; k != BodyStats::numKeys(); ++k) {
			bodies[i][k] += o.bodies[i][k];
		}
		for (uint32 k = 0; k != RuleStats::numKeys(); ++k) {
			rules[i][k] += o.rules[i][k];
		}
	}
	for (int i = 0; i != sizeof(eqs_)/sizeof(eqs_[0]); ++i) {
		eqs_[i] += o.eqs_[i];
	}
}

static const char* lpStats_s[] = {
#define KEY(x, y) x,
	LP_STATS(KEY)
#undef KEY
	"lp"
};
uint32 LpStats::size() {
	return (sizeof(lpStats_s)/sizeof(const char*))-1;
}
const char* LpStats::key(uint32 i) {
	return i < size() ? lpStats_s[i] : throw std::out_of_range("LpStats::key");
}
StatisticObject LpStats::at(const char* k) const {
#define MAP_IF(x, A) if (std::strcmp(k, x) == 0)  return A;
#define VALUE(X) StatisticObject::value(&(X))
#define FUNC(F) StatisticObject::value<LpStats, F>(this)
	LP_STATS(MAP_IF)
#undef VALUE
#undef FUNC
#undef MAP_IF
	throw std::out_of_range("LpStats::at");
}
#undef LP_STATS
/////////////////////////////////////////////////////////////////////////////////////////
// class LogicProgram
/////////////////////////////////////////////////////////////////////////////////////////
static PrgAtom trueAtom_g(0, false);
static const uint32 falseId = PrgNode::noNode;
static const uint32 bodyId  = PrgNode::noNode + 1;
static bool init_trueAtom_g = (trueAtom_g.setEq(0), true);
inline bool isAtom(Id_t uid) { return Potassco::atom(Potassco::lit(uid)) < bodyId; }
inline bool isBody(Id_t uid) { return Potassco::atom(Potassco::lit(uid)) >= bodyId; }
inline Id_t nodeId(Id_t uid) { return Potassco::atom(Potassco::lit(uid)) - (isAtom(uid) ? 0 : bodyId); }
inline bool signId(Id_t uid) { return Potassco::lit(uid) < 0; }

namespace {
struct LessBodySize {
	LessBodySize(const BodyList& bl) : bodies_(&bl) {}
	bool operator()(Id_t b1, Id_t b2 ) const {
		return (*bodies_)[b1]->size() < (*bodies_)[b2]->size()
			|| ((*bodies_)[b1]->size() == (*bodies_)[b2]->size() && (*bodies_)[b1]->type() < (*bodies_)[b2]->type());
	}
private:
	const BodyList* bodies_;
};
// Adds nogoods representing this node to the solver.
template <class NT>
bool toConstraint(NT* node, const LogicProgram& prg, ClauseCreator& c) {
	if (node->value() != value_free && !prg.ctx()->addUnary(node->trueLit())) {
    	return false;
	}
	return !node->relevant() || node->addConstraints(prg, c);
}
}

LogicProgram::LogicProgram() : theory_(0), input_(1, 1), auxData_(0), incData_(0) {
	CLASP_FAIL_IF(!init_trueAtom_g, "invalid static init");
}
LogicProgram::~LogicProgram() { dispose(true); }
LogicProgram::Incremental::Incremental() : startScc(0) {}
void LogicProgram::dispose(bool force) {
	// remove rules
	std::for_each(bodies_.begin(), bodies_.end(), DestroyObject());
	std::for_each(disjunctions_.begin(), disjunctions_.end(), DestroyObject());
	std::for_each(extended_.begin(), extended_.end(), DeleteObject());
	std::for_each(minimize_.begin(), minimize_.end(), DeleteObject());
	PodVector<ShowPair>::destruct(show_);
	delete auxData_; auxData_ = 0;
	MinList().swap(minimize_);
	RuleList().swap(extended_);
	BodyList().swap(bodies_);
	DisjList().swap(disjunctions_);
	bodyIndex_.clear();
	disjIndex_.clear();
	VarVec().swap(initialSupp_);
	if (theory_) {
		theory_->reset();
	}
	if (force || !incData_) {
		deleteAtoms(0);
		AtomList().swap(atoms_);
		AtomState().swap(atomState_);
		delete theory_;
		delete incData_;
		VarVec().swap(propQ_);
		stats.reset();
	}
	activeHead_.reset();
	activeBody_.reset();
}
void LogicProgram::deleteAtoms(uint32 start) {
	for (AtomList::const_iterator it = atoms_.begin() + start, end = atoms_.end(); it != end; ++it) {
		if (*it != &trueAtom_g) { delete *it; }
	}
}
bool LogicProgram::doStartProgram() {
	dispose(true);
	// atom 0 is always true
	PrgAtom* trueAt = new PrgAtom(0, false);
	atoms_.push_back(trueAt);
	trueAt->assignValue(value_true);
	trueAt->setInUpper(true);
	trueAt->setLiteral(lit_true());
	atomState_.set(0, AtomState::fact_flag);
	incData_ = 0;
	auxData_ = new Aux();
	input_   = AtomRange(1, 1);
	statsId_ = 0;
	return true;
}
void LogicProgram::setOptions(const AspOptions& opts) {
	opts_ = opts;
	if (opts.suppMod) { opts_.noSCC = 1; }
	if (opts.suppMod && ctx() && ctx()->sccGraph.get()) {
		ctx()->warn("'supp-models' ignored for non-tight programs.");
		opts_.suppMod = 0;
		opts_.noSCC   = 0;
	}
}
ProgramParser* LogicProgram::doCreateParser() {
	return new AspParser(*this);
}
bool LogicProgram::doUpdateProgram() {
	if (!incData_) { incData_ = new Incremental(); }
	if (!frozen()) { return true; }
	// delete bodies/disjunctions...
	dispose(false);
	setFrozen(false);
	auxData_ = new Aux();
	if (theory_) { theory_->update(); }
	incData_->update.clear();
	incData_->frozen.swap(incData_->update);
	// reset prop queue and add supported atoms from previous steps
	// {ai | ai in P}.
	PrgBody* support = input_.hi > 1 ? getTrueBody() : 0;
	propQ_.clear();
	for (Atom_t i = 1, end = input_.hi; i != end; ++i) {
		if (getRootId(i) >= input_.hi) {
			// atom is equivalent to some aux atom - make i the new root
			uint32 r = getRootId(i);
			std::swap(atoms_[i], atoms_[r]);
			atoms_[r]->setEq(i);
		}
		// remove dangling references
		PrgAtom* a = atoms_[i];
		a->clearSupports();
		a->clearDeps(PrgAtom::dep_all);
		a->setIgnoreScc(false);
		if (a->relevant() || a->frozen()) {
			ValueRep v = a->value();
			a->setValue(value_free);
			a->resetId(i, !a->frozen());
			if (ctx()->master()->value(a->var()) != value_free && !a->frozen()) {
				v = ctx()->master()->isTrue(a->literal()) ? value_true : value_false;
			}
			if (v != value_free) { assignValue(a, v, PrgEdge::noEdge()); }
			if (!a->frozen() && a->value() != value_false) {
				a->setIgnoreScc(true);
				support->addHead(a, PrgEdge::GammaChoice);
			}
		}
		else if (a->removed() || (!a->eq() && a->value() == value_false)) {
			a->resetId(i, true);
			a->setValue(value_false);
			atomState_.set(i, AtomState::false_flag);
		}
	}
	// delete any introduced aux atoms
	// this is safe because aux atoms are never part of the input program
	// it is necessary in order to free their ids, i.e. the id of an aux atom
	// from step I might be needed for a program atom in step I+1
	deleteAtoms(input_.hi);
	atoms_.erase(atoms_.begin()+input_.hi, atoms_.end());
	uint32 nAtoms = (uint32)atoms_.size();
	atomState_.resize(nAtoms);
	input_ = AtomRange(nAtoms, nAtoms);
	stats.reset();
	statsId_ = 0;
	return true;
}
bool LogicProgram::doEndProgram() {
	if (!frozen() && ctx()->ok()) {
		prepareProgram(!opts_.noSCC);
		addConstraints();
		addDomRules();
		addAcycConstraint();
	}
	return ctx()->ok();
}

bool LogicProgram::clone(SharedContext& oCtx) {
	assert(frozen());
	if (&oCtx == ctx()) {
		return true;
	}
	for (Var v = oCtx.numVars() + 1; ctx()->validVar(v); ++v) {
		oCtx.addVar(Var_t::Atom, ctx()->varInfo(v).rep);
	}
	SharedContext* t = ctx();
	setCtx(&oCtx);
	bool ok = addConstraints();
	if (ok) {
		oCtx.output    = ctx()->output;
		oCtx.heuristic = t->heuristic;
	}
	setCtx(t);
	return ok;
}

void LogicProgram::addMinimize() {
	CLASP_ASSERT_CONTRACT(frozen());
	for (MinList::iterator it = minimize_.begin(), end = minimize_.end(); it != end; ++it) {
		const BodyData::BodyLitVec& lits = (*it)->lits;
		const weight_t prio = (*it)->prio;
		for (BodyData::BodyLitVec::const_iterator xIt = lits.begin(), xEnd = lits.end(); xIt != xEnd; ++xIt) {
			addMinLit(prio, WeightLiteral(getLiteral(Potassco::id(xIt->lit)), xIt->weight));
		}
		// Make sure minimize constraint is not empty
		if (lits.empty()) addMinLit(prio, WeightLiteral(lit_false(), 1));
	}
}
static bool transform(const LogicProgram& prg, const PrgBody& body, Potassco::BodyView& out, Potassco::BodyLitVec& lits) {
	lits.clear();
	out.type = Potassco::Body_t::Normal;
	weight_t sw = 0, st = 0;
	for (uint32 i = 0, end = body.size(); i != end; ++i) {
		Potassco::WeightLit_t w = { toInt(body.goal(i)), body.weight(i) };
		bool relevant = !prg.frozen() || prg.inProgram(Potassco::atom(w));
		if      (relevant)  { sw += w.weight; lits.push_back(w); }
		else if (w.lit < 0) { st += w.weight; }
	}
	if (st >= body.bound()) {
		out.type = Potassco::Body_t::Normal;
	}
	else if (body.type() != Body_t::Normal) {
		out.type  = body.type() == Body_t::Sum ? Potassco::Body_t::Sum : Potassco::Body_t::Count;
		out.bound = body.bound() - st;
	}
	out.lits = Potassco::toSpan(lits);
	return sw >= (body.bound() - st);
}
void LogicProgram::accept(Potassco::AbstractProgram& out) {
	if (!ok()) {
		out.rule(toHead(Potassco::toSpan<Potassco::Atom_t>()), toBody(Potassco::toSpan<Potassco::WeightLit_t>()));
		return;
	}
	// visit external directives
	if (incData_) {
		for (VarVec::const_iterator it = incData_->frozen.begin(), end = incData_->frozen.end(); it != end; ++it) {
			out.external(*it, static_cast<Potassco::Value_t>(getAtom(*it)->freezeValue()));
		}
	}
	// visit eq- and assigned atoms
	Potassco::BodyLitVec bodyLits(1, Potassco::WeightLit_t());
	bodyLits.back().weight = 1;
	for (Atom_t i = startAtom(); i < atoms_.size(); ++i) {
		if (atoms_[i]->eq()) {
			Potassco::AtomSpan head = Potassco::toSpan(&i, 1);
			bodyLits[0].lit = Potassco::lit(getRootId(i));
			if (isFact(Potassco::atom(bodyLits[0]))) {
				out.rule(toHead(head), toBody(Potassco::toSpan<Potassco::WeightLit_t>()));
			}
			else if (inProgram(Potassco::atom(bodyLits[0]))) {
				out.rule(toHead(head), toBody(Potassco::toSpan(bodyLits)));
			}
		}
		else if (!atomState_.isFact(i) && atoms_[i]->value() != value_free) {
			Potassco::AtomSpan head = Potassco::toSpan<Potassco::Atom_t>();
			if (atoms_[i]->value() != value_false) {
				bodyLits[0].lit = Potassco::neg(i);
				out.rule(toHead(head), toBody(Potassco::toSpan(bodyLits)));
			}
			else if (inProgram(i)) {
				bodyLits[0].lit = Potassco::lit(i);
				out.rule(toHead(head), toBody(Potassco::toSpan(bodyLits)));
			}
		}
	}
	// visit program rules
	typedef PodVector<Potassco::Atom_t>::type LpAtomVec;
	LpAtomVec choice;
	Potassco::Atom_t normal;
	const bool simp = frozen();
	for (BodyList::iterator bIt = bodies_.begin(); bIt != bodies_.end(); ++bIt) {
		PrgBody* b = *bIt;
		Potassco::BodyView body;
		if (b->relevant() && (b->inRule() || b->value() == value_false) && transform(*this, *b, body, bodyLits)) {
			if (b->value() == value_false) {
				out.rule(toHead(Potassco::toSpan<Potassco::Atom_t>()), body);
				continue;
			}
			uint32 numDis = 0;
			for (PrgBody::head_iterator hIt = b->heads_begin(); hIt != b->heads_end(); ++hIt) {
				if (hIt->isGamma() || (simp && !getHead(*hIt)->hasVar())) { continue; }
				if (hIt->isAtom() && hIt->node() && inProgram(hIt->node())) {
					if      (hIt->isNormal()) { out.rule(Potassco::toHead(normal = hIt->node()), body); }
					else if (hIt->isChoice()) { choice.push_back(hIt->node()); }
					if (simp && getRootAtom(hIt->node())->var() == b->var() && body.type != Potassco::Body_t::Normal) {
						// replace complex body with head atom
						Potassco::WeightLit_t w = { Potassco::lit(hIt->node()), 1 };
						if (getRootAtom(hIt->node())->literal() != b->literal()) { w.lit *= -1; }
						bodyLits.assign(1, w);
						body = toBody(Potassco::toSpan(bodyLits));
					}
				}
				else if (hIt->isDisj()) { ++numDis; }
			}
			if (!choice.empty()) {
				out.rule(toHead(Potassco::toSpan(choice), Potassco::Head_t::Choice), body);
			}
			for (PrgBody::head_iterator hIt = b->heads_begin(); hIt != b->heads_end() && numDis; ++hIt) {
				if (hIt->isDisj()) {
					PrgDisj* d = getDisj(hIt->node());
					choice.assign(d->begin(), d->end());
					out.rule(toHead(Potassco::toSpan(choice)), body);
					--numDis;
				}
			}
			choice.clear();
		}
	}
	PodVector<Potassco::WeightLit_t>::type min;
	for (MinList::iterator it = minimize_.begin(), end = minimize_.end(); it != end; ++it) {
		Potassco::WeightLitSpan lits = Potassco::toSpan((*it)->lits);
		for (const Potassco::WeightLit_t* x = Potassco::begin(lits), *xEnd = Potassco::end(lits); x != xEnd; ++x) {
			if (x->weight == 0 || !inProgram(Potassco::atom(*x))) { // simplify literals
				min.assign(Potassco::begin(lits), x);
				for (; x != xEnd; ++x) {
					if (x->weight != 0 && (x->weight < 0 || x->lit < 0 || inProgram(Potassco::atom(*x)))) {
						min.push_back(*x);
					}
				}
				lits = Potassco::toSpan(min);
				break;
			}
		}
		out.minimize((*it)->prio, lits);
	}
	// visit output directives
	Potassco::LitVec cond;
	for (ShowVec::const_iterator it = show_.begin(); it != show_.end(); ++it) {
		if (extractCondition(it->first, cond)) {
			out.output(Potassco::toSpan(it->second.c_str(), std::strlen(it->second.c_str())), Potassco::toSpan(cond));
		}
	}
	// visit projection directives
	if (!auxData_->project.empty()) {
		out.project(Potassco::toSpan(auxData_->project.back() ? auxData_->project : VarVec()));
	}
	// visit assumptions
	if (!auxData_->assume.empty()) {
		out.assume(Potassco::toSpan(auxData_->assume));
	}
	// visit heuristics
	if (!auxData_->dom.empty()) {
		for (DomRules::const_iterator it = auxData_->dom.begin(), end = auxData_->dom.end(); it != end; ++it) {
			if (extractCondition(it->cond, cond)) {
				out.heuristic(it->atom, static_cast<DomModType>(it->type), it->bias, it->prio, Potassco::toSpan(cond));
			}
		}
	}
	// visit acyc edges
	if (!auxData_->acyc.empty()) {
		for (AcycRules::const_iterator it = auxData_->acyc.begin(), end = auxData_->acyc.end(); it != end; ++it) {
			if (extractCondition(it->cond, cond)) {
				out.acycEdge(it->node[0], it->node[1], Potassco::toSpan(cond));
			}
		}
	}
	if (theory_ && theory_->numAtoms()) {
		struct This : public Potassco::TheoryData::Visitor {
			This(const Asp::LogicProgram& p, Potassco::AbstractProgram& o, Potassco::LitVec& c) : self(&p), out(&o), cond(&c) {}
			virtual void visit(const Potassco::TheoryData& data, Potassco::Id_t termId, const Potassco::TheoryTerm& t) {
				if (!addSeen(termId, 1)) { return; }
				data.accept(t, *this);
				Potassco::print(*out, termId, t);
			}
			virtual void visit(const Potassco::TheoryData& data, Potassco::Id_t elemId, const Potassco::TheoryElement& e) {
				if (!addSeen(elemId, 2)) { return; }
				data.accept(e, *this);
				cond->clear();
				if (e.condition()) { self->extractCondition(e.condition(), *cond); }
				out->theoryElement(elemId, e.terms(), Potassco::toSpan(*cond));
			}
			virtual void visit(const Potassco::TheoryData& data, const Potassco::TheoryAtom& a) {
				data.accept(a, *this);
				Potassco::print(*out, a);
				const Atom_t id = a.atom();
				if (self->validAtom(id) && self->atomState_.isSet(id, AtomState::false_flag) && !self->inProgram(id)) {
					Potassco::WeightLit_t wl = {Potassco::lit(id), 1};
					out->rule(toHead(Potassco::toSpan<Potassco::Atom_t>()), toBody(Potassco::toSpan(&wl, 1)));
				}
			}
			bool addSeen(Potassco::Id_t id, unsigned char n) {
				if (id >= seen.size()) { seen.resize(id + 1, 0); }
				unsigned char old = seen[id];
				return (seen[id] |= n) != old;
			}
			typedef PodVector<unsigned char>::type IdSet;
			const Asp::LogicProgram*   self;
			Potassco::AbstractProgram* out;
			Potassco::LitVec*          cond;
			IdSet                      seen;
		} self(*this, out, cond);
		theory_->accept(self);
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// Program mutating functions
/////////////////////////////////////////////////////////////////////////////////////////
#define check_not_frozen() CLASP_ASSERT_CONTRACT_MSG(!frozen(), "Can't update frozen program!")
#define check_modular(x, atomId) (void)( (!!(x)) || (throw RedefinitionError((atomId), this->findName((atomId))), 0))
RedefinitionError::RedefinitionError(unsigned atomId, const char* name)
	: std::logic_error(ClaspErrorString("redefinition of atom <'%s',%u>", name && *name ? name : "_", atomId).c_str()) {
}

Atom_t LogicProgram::newAtom() {
	check_not_frozen();
	Atom_t id = static_cast<Atom_t>(atoms_.size());
	atoms_.push_back( new PrgAtom(id) );
	return id;
}

Id_t LogicProgram::newCondition(const Potassco::LitSpan& cond) {
	check_not_frozen();
	rule_.reset();
	for (Potassco::LitSpan::iterator it = Potassco::begin(cond), end = Potassco::end(cond); it != end; ++it) {
		CLASP_FAIL_IF(Potassco::atom(*it) >= bodyId, "Atom out of bounds");
		Potassco::WeightLit_t x = { *it, 1 };
		rule_.body.lits.push_back(x);
	}
	if (simplifyRule(RuleView(rule_.head, rule_.body))) {
		if (activeBody_.empty())     { return 0; }
		if (activeBody_.size() == 1) { return Potassco::id(activeBody_.begin()->lit); }
		PrgBody* b = getBodyFor(activeBody_);
		b->markFrozen();
		return static_cast<Id_t>(Clasp::Asp::bodyId | b->id());
	}
	return static_cast<Id_t>(Clasp::Asp::falseId);
}

LogicProgram& LogicProgram::addOutput(const ConstString& str, const Potassco::LitSpan& cond) {
	if (!ctx()->output.filter(str)) {
		CLASP_FAIL_IF(cond.size == 1 && Potassco::atom(cond[0]) >= bodyId, "Atom out of bounds");
		show_.push_back(ShowPair(cond.size == 1 ? Potassco::id(cond[0]) : newCondition(cond), str));
	}
	return *this;
}
LogicProgram& LogicProgram::addOutput(const ConstString& str, Id_t id) {
	if (!ctx()->output.filter(str) && id != falseId) {
		show_.push_back(ShowPair(id, str));
	}
	return *this;
}

LogicProgram& LogicProgram::addProject(const Potassco::AtomSpan& atoms) {
	check_not_frozen();
	VarVec& pro = auxData_->project;
	if (!Potassco::empty(atoms)) {
		if (!pro.empty() && pro.back() == 0) { pro.pop_back(); }
		pro.insert(pro.end(), Potassco::begin(atoms), Potassco::end(atoms));
	}
	else if (pro.empty()) {
		pro.push_back(0);
	}
	return *this;
}

TheoryData& LogicProgram::theoryData() {
	if (!theory_) { theory_ = new TheoryData(); }
	return *theory_;
}

PrgAtom* LogicProgram::setExternal(Atom_t atomId, ValueRep v) {
	PrgAtom* a = resize(atomId);
	if (a->frozen() || (isNew(a->id()) && !a->supports())) {
		if (!incData_)    { incData_ = new Incremental(); }
		if (!a->frozen()) { incData_->update.push_back(a->id()); }
		a->markFrozen(v);
		return a;
	}
	return 0; // atom is defined or from a previous step - ignore!
}

LogicProgram& LogicProgram::freeze(Atom_t atomId, ValueRep value) {
	check_not_frozen();
	CLASP_ASSERT_CONTRACT(value < value_weak_true);
	setExternal(atomId, value);
	return *this;
}

LogicProgram& LogicProgram::unfreeze(Atom_t atomId) {
	check_not_frozen();
	PrgAtom* a = setExternal(atomId, value_free);
	if (a && a->supports() == 0) {
		// add dummy edge - will be removed once we update the set of frozen atoms
		a->addSupport(PrgEdge::noEdge());
	}
	return *this;
}
bool LogicProgram::supportsSmodels() const {
	if (incData_ || theory_)        { return false; }
	if (!auxData_->dom.empty())     { return false; }
	if (!auxData_->acyc.empty())    { return false; }
	if (!auxData_->assume.empty())  { return false; }
	if (!auxData_->project.empty()) { return false; }
	for (ShowVec::const_iterator it = show_.begin(), end = show_.end(); it != end; ++it) {
		Potassco::Lit_t lit = Potassco::lit(it->first);
		if (lit <= 0 || static_cast<uint32>(lit) >= bodyId) { return false; }
	}
	return true;
}

bool LogicProgram::isExternal(Atom_t aId) const {
	if (!aId || !validAtom(aId)) { return false; }
	PrgAtom* a = getRootAtom(aId);
	return a->frozen() && (a->supports() == 0 || frozen());
}
bool LogicProgram::isFact(Atom_t aId) const {
	return validAtom(aId) && (atomState_.isFact(aId) || atomState_.isFact(getRootId(aId)));
}
bool LogicProgram::isDefined(Atom_t aId) const {
	if (!validAtom(aId) || getAtom(aId)->removed()) {
		return false;
	}
	PrgAtom* a = getAtom(aId);
	return isFact(aId) || (a->relevant() && a->supports() && !isExternal(aId));
}
bool LogicProgram::inProgram(Atom_t id) const {
	if (PrgAtom* a = (validAtom(id) ? getAtom(id) : 0)) {
		return a->relevant() && (a->supports() || a->frozen() || !isNew(id));
	}
	return false;
}
LogicProgram& LogicProgram::addAssumption(const Potassco::LitSpan& lits) {
	auxData_->assume.insert(auxData_->assume.end(), Potassco::begin(lits), Potassco::end(lits));
	return *this;
}

LogicProgram& LogicProgram::addAcycEdge(uint32 n1, uint32 n2, Id_t condId) {
	if (condId != falseId) {
		AcycArc arc = { condId, {n1, n2} };
		auxData_->acyc.push_back(arc);
	}
	upStat(RK(Acyc), 1);
	return *this;
}

LogicProgram& LogicProgram::addDomHeuristic(Atom_t atom, DomModType t, int bias, unsigned prio) {
	return addDomHeuristic(atom, t, bias, prio, Potassco::toSpan<Potassco::Lit_t>());
}
LogicProgram& LogicProgram::addDomHeuristic(Atom_t atom, DomModType type, int bias, unsigned prio, Id_t cond) {
	static_assert(sizeof(DomRule) == sizeof(uint32[3]), "Invalid DomRule size");
	if (cond != falseId) {
		auxData_->dom.push_back(DomRule());
		DomRule& x = auxData_->dom.back();
		x.atom = atom;
		x.type = type;
		x.cond = cond;
		x.bias = (int16)Range<int>(INT16_MIN, INT16_MAX).clamp(bias);
		x.prio = (uint16)Range<unsigned>(unsigned(0), unsigned(~uint16(0)) ).clamp(prio);
	}
	upStat(RK(Heuristic), 1);
	return *this;
}

LogicProgram& LogicProgram::addRule(const RuleView& rule) {
	check_not_frozen();
	if (simplifyRule(rule)) {
		upStat(rule.head.type);
		if (handleNatively(activeHead_.type, activeBody_)) { // and can be handled natively
			addRule(activeHead_, activeBody_);
		}
		else {
			upStat(activeBody_.type);
			if (activeHead_.size() <= 1 && transformNoAux(activeHead_.type, activeBody_)) {
				// rule transformation does not require aux atoms - do it now
				int oId  = statsId_;
				statsId_ = 1;
				RuleTransform tm(*this);
				upStat(activeBody_.type, -1);
				upStat(activeHead_.type, -1);
				tm.transform(RuleView(activeHead_, activeBody_), RuleTransform::strategy_select_no_aux);
				statsId_ = oId;
			}
			else {
				// make sure we have all head atoms
				for (HeadData::iterator it = activeHead_.begin(), end = activeHead_.end(); it != end; ++it) {
					resize(*it);
				}
				extended_.push_back(new Rule());
				extended_.back()->head = activeHead_;
				extended_.back()->body = activeBody_;
			}
		}
	}
	activeBody_.reset();
	return *this;
}
LogicProgram& LogicProgram::addMinimize(weight_t prio, const Potassco::WeightLitSpan& lits) {
	SingleOwnerPtr<Min> n(new Min());
	n->prio = prio;
	MinList::iterator it = std::lower_bound(minimize_.begin(), minimize_.end(), n.get(), CmpMin());
	if (it == minimize_.end() || (*it)->prio != prio) {
		n->lits.assign(Potassco::begin(lits), Potassco::end(lits));
		minimize_.insert(it, n.get());
		n.release();
		upStat(RuleStats::Minimize);
	}
	else {
		(*it)->lits.insert((*it)->lits.end(), Potassco::begin(lits), Potassco::end(lits));
	}
	return *this;
}

LogicProgram::RuleBuilder::RuleBuilder(LogicProgram& prg) : prg_(&prg) {}

LogicProgram::RuleBuilder& LogicProgram::RuleBuilder::addHead(Atom_t atomId) {
	prg_->rule_.head.add(atomId);
	return *this;
}
LogicProgram::RuleBuilder& LogicProgram::RuleBuilder::addToBody(Atom_t atomId, bool pos, weight_t w) {
	prg_->rule_.body.add(atomId, pos, w);
	return *this;
}
LogicProgram& LogicProgram::RuleBuilder::endRule() {
	return prg_->addRule(prg_->rule_.head.toView(), prg_->rule_.body.toView());
}

LogicProgram::RuleBuilder LogicProgram::startRule() {
	rule_.reset();
	return RuleBuilder(*this);
}
LogicProgram::RuleBuilder LogicProgram::startChoiceRule() {
	rule_.reset();
	rule_.head.type = Head_t::Choice;
	return RuleBuilder(*this);
}
LogicProgram::RuleBuilder LogicProgram::startWeightRule(weight_t bound) {
	rule_.reset();
	rule_.body.type  = Body_t::Sum;
	rule_.body.bound = bound;
	return RuleBuilder(*this);
}
LogicProgram::MinBuilder::MinBuilder(LogicProgram& prg) : prg_(&prg) {}
LogicProgram::MinBuilder LogicProgram::startMinimizeRule(weight_t prio) {
	rule_.reset();
	rule_.body.type  = Body_t::Sum;
	rule_.body.bound = prio;
	return MinBuilder(*this);
}
LogicProgram& LogicProgram::MinBuilder::endRule() {
	return prg_->addMinimize(prg_->rule_.body.bound, Potassco::toSpan(prg_->rule_.body.lits));
}
LogicProgram::MinBuilder& LogicProgram::MinBuilder::addToBody(Atom_t atomId, bool pos, weight_t w) {
	prg_->rule_.body.add(atomId, pos, w);
	return *this;
}
#undef check_not_frozen
/////////////////////////////////////////////////////////////////////////////////////////
// Query functions
/////////////////////////////////////////////////////////////////////////////////////////
bool LogicProgram::isFact(PrgAtom* a) const {
	uint32 eqId = getRootId(a->id());
	if (atomState_.isFact(eqId)) {
		return true;
	}
	if (a->value() == value_true) {
		for (PrgAtom::sup_iterator it = a->supps_begin(), end = a->supps_end(); it != end; ++it) {
			if (it->isBody() && it->isNormal() && getBody(it->node())->bound() == 0) {
				return true;
			}
		}
	}
	return false;
}
Literal LogicProgram::getLiteral(Id_t id) const {
	Literal out = lit_false();
	Potassco::Id_t nId = nodeId(id);
	if (isAtom(id) && validAtom(nId)) {
		out = getRootAtom(nId)->literal();
	}
	else if (isBody(id)) {
		CLASP_FAIL_IF(!validBody(nId), "Invalid condition");
		out = getBody(getEqBody(nId))->literal();
	}
	return out ^ signId(id);
}
Literal LogicProgram::getDomLiteral(Atom_t atomId) const {
	IndexMap::const_iterator it = domEqIndex_.find(atomId);
	return it == domEqIndex_.end() ? getLiteral(atomId) : posLit(it->second);
}

void LogicProgram::doGetAssumptions(LitVec& out) const {
	if (incData_) {
		for (VarVec::const_iterator it = incData_->frozen.begin(), end = incData_->frozen.end(); it != end; ++it) {
			Literal lit = getRootAtom(*it)->assumption();
			if (lit != lit_true()) { out.push_back( lit ); }
		}
	}
	for (Potassco::LitVec::const_iterator it = auxData_->assume.begin(), end = auxData_->assume.end(); it != end; ++it) {
		out.push_back(getLiteral(Potassco::id(*it)));
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// Program definition - private
/////////////////////////////////////////////////////////////////////////////////////////
void LogicProgram::addRule(const HeadData& head, const SBody& body) {
	if (head.size() <= 1 && head.type == Head_t::Disjunctive) {
		if      (head.empty()) { addIntegrity(body); return; }
		else if (body.empty()) { addFact(head.atoms, *getBodyFor(body)); return; }
	}
	PrgBody* b = getBodyFor(body);
	// only a non-false body can define atoms
	if (b->value() != value_false) {
		bool const disjunctive = head.size() > 1 && head.type == Head_t::Disjunctive;
		const EdgeType t = head.type == Head_t::Disjunctive ? PrgEdge::Normal : PrgEdge::Choice;
		uint32 headHash = 0;
		bool ignoreScc  = opts_.noSCC || b->size() == 0;
		for (HeadData::iterator it = head.begin(), end = head.end(); it != end; ++it) {
			PrgAtom* a = resize(*it);
			check_modular(isNew(*it) || a->frozen() || a->value() == value_false, *it);
			if (!disjunctive) {
				// Note: b->heads may now contain duplicates. They are removed in PrgBody::simplifyHeads.
				b->addHead(a, t);
				if (ignoreScc) { a->setIgnoreScc(ignoreScc); }
			}
			else {
				headHash += hashLit(posLit(*it));
				atomState_.addToHead(*it);
			}
		}
		if (disjunctive) {
			PrgDisj* d = getDisjFor(head.atoms, headHash);
			b->addHead(d, t);
		}
	}
}
void LogicProgram::addFact(const VarVec& head, PrgBody& trueBody) {
	for (VarVec::const_iterator it = head.begin(), end = head.end(); it != end; ++it) {
		PrgAtom* a = resize(*it);
		check_modular(isNew(*it) || a->frozen() || a->value() == value_false, *it);
		if (*it != a->id() || atomState_.isFact(*it)) { continue; }
		a->setIgnoreScc(true);
		atomState_.set(*it, AtomState::fact_flag);
		if (!a->hasDep(PrgAtom::dep_all) && !a->frozen()) {
			if (!a->assignValue(value_true) || !a->propagateValue(*this, false)) {
				setConflict();
			}
			for (PrgAtom::sup_iterator bIt = a->supps_begin(), bEnd = a->supps_end(); bIt != bEnd; ++bIt) {
				if      (bIt->isBody()) { getBody(bIt->node())->markHeadsDirty(); }
				else if (bIt->isDisj()) { getDisj(bIt->node())->markDirty(); }
			}
			atoms_[*it] = &trueAtom_g;
			delete a;
		}
		else {
			trueBody.addHead(a, PrgEdge::Normal);
			assignValue(a, value_true, PrgEdge::newEdge(trueBody, PrgEdge::Normal));
		}
	}
}
void LogicProgram::addIntegrity(const SBody& body) {
	if (body.size() != 1 || body.meta.id != varMax) {
		PrgBody* B = getBodyFor(body);
		if (!B->assignValue(value_false) || !B->propagateValue(*this, true)) {
			setConflict();
		}
	}
	else {
		PrgAtom* a = resize(Potassco::atom(*body.begin()));
		ValueRep v = Potassco::lit(*body.begin()) > 0 ? value_false : value_weak_true;
		assignValue(a, v, PrgEdge::noEdge());
	}
}

bool LogicProgram::assignValue(PrgAtom* a, ValueRep v, PrgEdge reason) {
	if (a->eq()) { a = getRootAtom(a->id()); }
	ValueRep old = a->value();
	if (old == value_weak_true && v != value_weak_true) old = value_free;
	if (!a->assignValue(v)) { setConflict(); return false; }
	if (old == value_free)  { propQ_.push_back(a->id());  }
	if (v == value_false) {
		atomState_.set(a->id(), AtomState::false_flag);
	}
	else if (v == value_true && reason.isBody() && reason.isNormal() && getBody(reason.node())->bound() == 0) {
		atomState_.set(a->id(), AtomState::fact_flag);
	}
	return true;
}
bool LogicProgram::assignValue(PrgHead* h, ValueRep v, PrgEdge reason) {
	return !h->isAtom() || assignValue(static_cast<PrgAtom*>(h), v, reason);
}

bool LogicProgram::handleNatively(Head_t ht, const BodyData& body) const {
	ExtendedRuleMode m = opts_.erMode;
	if (m == mode_native || (body.type == Body_t::Normal && ht == Head_t::Disjunctive)) {
		return true;
	}
	else if (m == mode_transform_integ || m == mode_transform_scc || m == mode_transform_nhcf) {
		return true;
	}
	else if (m == mode_transform) {
		return false;
	}
	else if (m == mode_transform_dynamic) {
		return body.type == Body_t::Normal || transformNoAux(ht, body) == false;
	}
	else if (m == mode_transform_choice) {
		return ht != Head_t::Choice;
	}
	else if (m == mode_transform_card)   {
		return body.type != Body_t::Count;
	}
	else if (m == mode_transform_weight) {
		return body.type == Body_t::Normal;
	}
	assert(false && "unhandled extended rule mode");
	return true;
}

bool LogicProgram::transformNoAux(Head_t ht, const BodyData& body) const {
	return ht == Head_t::Disjunctive && (body.bound == 1 || (body.size() <= 6 && choose(body.size(), body.bound) <= 15));
}

void LogicProgram::transformExtended() {
	uint32 a = numAtoms();
	RuleTransform tm(*this);
	for (RuleList::size_type i = 0; i != extended_.size(); ++i) {
		Rule* r = extended_[i];
		upStat(r->head.type, -1);
		upStat(r->body.type, -1);
		RuleView rv(r->head, r->body);
		if (rv.isPrimitive()) {
			tm.transform(rv);
		}
		else {
			Atom_t aux = newAtom();
			HeadView auxHead = {Head_t::Disjunctive, Potassco::toSpan(&aux, 1)};
			// aux :- body
			if (handleNatively(auxHead.type, r->body)) {
				addRule(auxHead, r->body.toView());
			}
			else {
				RuleTransform::Strategy st = transformNoAux(Head_t::Disjunctive, r->body) ? RuleTransform::strategy_select_no_aux : RuleTransform::strategy_default;
				tm.transform(RuleView(auxHead, r->body.toView()), st);
			}
			// head :- aux
			r->body.reset(Body_t::Normal).add(aux, true);
			if (handleNatively(r->head.type, r->body)) {
				addRule(r->head.toView(), r->body.toView());
			}
			else {
				tm.transform(RuleView(r->head.toView(), r->body.toView()));
			}
		}
		delete r;
	}
	extended_.clear();
	incTrAux(numAtoms() - a);
}

void LogicProgram::transformIntegrity(uint32 nAtoms, uint32 maxAux) {
	if (stats.bodies[1][Body_t::Count] == 0) { return; }
	// find all constraint rules that are integrity constraints
	BodyList integrity;
	for (uint32 i = 0, end = static_cast<uint32>(bodies_.size()); i != end; ++i) {
		PrgBody* b = bodies_[i];
		if (b->relevant() && b->type() == Body_t::Count && b->value() == value_false) {
			integrity.push_back(b);
		}
	}
	if (!integrity.empty() && (integrity.size() == 1 || (nAtoms/double(bodies_.size()) > 0.5 && integrity.size() / double(bodies_.size()) < 0.01))) {
		uint32 aux = static_cast<uint32>(atoms_.size());
		RuleTransform tr(*this);
		// transform integrity constraints
		for (BodyList::size_type i = 0; i != integrity.size(); ++i) {
			PrgBody* b = integrity[i];
			uint32 est = b->bound()*( b->sumW()-b->bound() );
			if (est > maxAux) {
				// reached limit on aux atoms - stop transformation
				break;
			}
			maxAux -= est;
			// transform rule
			setFrozen(false);
			rule_.head.reset();
			rule_.body.reset(Body_t::Count);
			rule_.body.bound = b->bound();
			for (uint32 g = 0; g != b->size(); ++g) {
				rule_.body.add(b->goal(g).var(), !b->goal(g).sign());
			}
			upStat(Head_t::Disjunctive, -1);
			upStat(Body_t::Count, -1);
			tr.transform(RuleView(rule_.head, rule_.body));
			setFrozen(true);
			// propagate integrity condition to new rules
			propagate(true);
			b->markRemoved();
		}
		// create vars for new atoms/bodies
		for (uint32 i = aux; i != atoms_.size(); ++i) {
			PrgAtom* a = atoms_[i];
			for (PrgAtom::sup_iterator it = a->supps_begin(); it != a->supps_end(); ++it) {
				PrgBody* nb = bodies_[it->node()];
				assert(nb->value() != value_false);
				nb->assignVar(*this);
			}
			a->assignVar(*this, a->supports() ? *a->supps_begin() : PrgEdge::noEdge());	
		}
		incTrAux(static_cast<uint32>(atoms_.size()) - aux);
	}
}

void LogicProgram::updateFrozenAtoms() {
	if (!incData_) { return; }
	assert(incData_->frozen.empty());
	activeHead_.reset();
 	activeBody_.reset();
	PrgBody* support   = 0;
	VarVec::iterator j = incData_->update.begin();
	for (VarVec::iterator it = j, end = incData_->update.end(); it != end; ++it) {
		Id_t id = getRootId(*it);
 		PrgAtom* a = getAtom(id);
		assert(a->frozen());
		a->resetId(id, false);
		if (a->supports() != 0) {
			a->clearFrozen();
			if (*a->supps_begin() == PrgEdge::noEdge()) {
				// remove dummy edge added in unfreeze()
				a->removeSupport(PrgEdge::noEdge());
			}
			if (!isNew(id)) {
				// keep in list so that we can later perform completion
				*j++ = id;
			}
		}
		else {
			assert(a->relevant() && a->supports() == 0);
			if (!support) { support = getTrueBody(); }
 			a->setIgnoreScc(true);
			support->addHead(a, PrgEdge::GammaChoice);
			incData_->frozen.push_back(id); // still frozen
		}
	}
	incData_->update.erase(j, incData_->update.end());
}

void LogicProgram::prepareProgram(bool checkSccs) {
	assert(!frozen());
	freezeTheory();
	uint32 nAtoms  = (uint32)atoms_.size();
	input_.hi = nAtoms;
	for (uint32 i = 0; i != RuleStats::numKeys(); ++i) {
		stats.rules[1][i] += stats.rules[0][i];
	}
	for (uint32 i = 0; i != BodyStats::numKeys(); ++i) {
		stats.bodies[1][i] += stats.bodies[0][i];
	}
	statsId_ = 1;
	transformExtended();
	updateFrozenAtoms();
	PrgAtom* suppAtom = 0;
	if (opts_.suppMod) {
		suppAtom = getAtom(newAtom());
		startChoiceRule().addHead(suppAtom->id()).endRule();
		RB r = startChoiceRule();
		for (Atom_t v = startAtom(), end = suppAtom->id(); v != end; ++v) {
			if (atoms_[v]->supports() != 0) { r.addHead(v); }
		}
		r.addToBody(suppAtom->id(), true);
		r.endRule();
	}
	setFrozen(true);
	Preprocessor p;
	if (hasConflict() || !propagate(true) || !p.preprocess(*this, opts_.iters != 0 ? Preprocessor::full_eq : Preprocessor::no_eq, opts_.iters, opts_.dfOrder != 0)) {
		setConflict();
		return;
	}
	if (suppAtom && (!assignValue(suppAtom, value_false, PrgEdge::noEdge()) || !propagate(true))) {
		setConflict();
		return;
	}
	if (opts_.erMode == mode_transform_integ || opts_.erMode == mode_transform_dynamic) {
		nAtoms -= startAtom();
		transformIntegrity(nAtoms, std::min(uint32(15000), nAtoms*2));
	}
	addMinimize();
	uint32 sccs = 0;
	if (checkSccs) {
		uint32 startScc = incData_ ? incData_->startScc : 0;
		SccChecker c(*this, auxData_->scc, startScc);
		sccs       = c.sccs();
		stats.sccs = (sccs-startScc);
		if (incData_) { incData_->startScc = c.sccs(); }
		if (!disjunctions_.empty() || (opts_.erMode == mode_transform_scc && sccs)) {
			// reset node ids changed by scc checking
			for (uint32 i = 0; i != bodies_.size(); ++i) {
				if (getBody(i)->relevant()) { getBody(i)->resetId(i, true); }
			}
			for (uint32 i = 0; i != atoms_.size(); ++i) {
				if (getAtom(i)->relevant()) { getAtom(i)->resetId(i, true); }
			}
		}
	}
	else { stats.sccs = PrgNode::noScc; }
	finalizeDisjunctions(p, sccs);
	prepareComponents();
	prepareOutputTable();
	if (theory_) {
		TFilter f = { this };
		theory_->filter(f);
	}
	stats.atoms = static_cast<uint32>(atoms_.size()) - startAtom();
	bodyIndex_.clear();
	disjIndex_.clear();
}

void LogicProgram::freezeTheory() {
	if (theory_) {
		PrgBody* supp = 0;
		for (TheoryData::atom_iterator it = theory_->currBegin(), end = theory_->end(); it != end; ++it) {
			const Potassco::TheoryAtom& a = **it;
			PrgAtom* at = 0;
			if (a.atom() && isNew(a.atom()) && !(at = resize(a.atom()))->supports()) { 
				if (!supp) { supp = getTrueBody(); }
				at->markFrozen(value_free);
				at->setIgnoreScc(true);
				supp->addHead(at, PrgEdge::GammaChoice);
			}
		}
	}
}
bool LogicProgram::TFilter::operator()(const Potassco::TheoryAtom& a) const {
	Atom_t id = a.atom();
	if (self->getLiteral(id) != lit_false() && self->getRootAtom(id)->value() != value_false) {
		self->ctx()->setFrozen(self->getLiteral(id).var(), true);
		return false;
	}
	PrgAtom* at = self->getRootAtom(id);
	return !at->frozen();
}

struct LogicProgram::DlpTr : public RuleTransform::ProgramAdapter {
	DlpTr(LogicProgram* x, EdgeType et) : self(x), type(et), scc(0) {}
	Atom_t newAtom() {
		CLASP_FAIL_IF(type == PrgEdge::Gamma, "shifting must not introduce new atoms!");
		Atom_t x   = self->newAtom();
		PrgAtom* a = self->getAtom(x);
		self->auxData_->scc.push_back(a);
		a->setScc(scc);
		a->setSeen(true);
		atoms.push_back(x);
		return x;
	}
	virtual void addRule(const HeadView& head, const BodyView& body) {
		if (!self->simplifyRule(RuleView(head, body))) { return; }
		bool gamma = type == PrgEdge::Gamma;
		PrgAtom* a = self->getAtom(*self->activeHead_.begin());
		PrgBody* B = self->assignBodyFor(self->activeBody_, type, gamma);
		if (B->value() != value_false && !B->hasHead(a, PrgEdge::Normal)) {
			B->addHead(a, type);
			self->stats.gammas += uint32(gamma);
		}
	}
	LogicProgram* self;
	EdgeType type;
	uint32   scc;
	VarVec   atoms;
};

// replace disjunctions with gamma (shifted) and delta (component-shifted) rules
void LogicProgram::finalizeDisjunctions(Preprocessor& p, uint32 numSccs) {
	if (disjunctions_.empty()) { return; }
	VarVec head; BodyList supports;
	disjIndex_.clear();
	SccMap sccMap;
	sccMap.resize(numSccs, 0);
	enum SccFlag { seen_scc = 1u, is_scc_non_hcf = 128u };
	// replace disjunctions with shifted rules and non-hcf-disjunctions
	DisjList disj; disj.swap(disjunctions_);
	setFrozen(false);
	uint32 shifted = 0;
	stats.nonHcfs  = uint32(nonHcfs_.size());
	Literal bot    = lit_false();
	DlpTr tr(this, PrgEdge::Gamma);
	RuleTransform shifter(tr);
	for (uint32 id = 0, maxId = disj.size(); id != maxId; ++id) {
		PrgDisj* d = disj[id];
		Literal dx = d->inUpper() ? d->literal() : bot;
		d->resetId(id, true); // id changed during scc checking
		PrgEdge e  = PrgEdge::newEdge(*d, PrgEdge::Choice);
		// remove from program and
		// replace with shifted rules or component-shifted disjunction
		head.clear(); supports.clear();
		for (PrgDisj::atom_iterator it = d->begin(), end = d->end(); it != end; ++it) {
			uint32  aId = *it;
			PrgAtom* at = getAtom(aId);
			at->removeSupport(e);
			if (dx == bot) { continue; }
			if (at->eq())  {
				at = getAtom(aId = getRootId(aId));
			}
			if (isFact(at)){
				dx = bot;
				continue;
			}
			if (at->inUpper()) {
				head.push_back(aId);
				if (at->scc() != PrgNode::noScc){ sccMap[at->scc()] = seen_scc; }
			}
		}
		EdgeVec temp;
		d->clearSupports(temp);
		for (EdgeVec::iterator it = temp.begin(), end = temp.end(); it != end; ++it) {
			PrgBody* b = getBody(it->node());
			if (b->relevant() && b->value() != value_false) { supports.push_back(b); }
			b->removeHead(d, PrgEdge::Normal);
		}
		d->destroy();
		// create shortcut for supports to avoid duplications during shifting
		Literal supportLit = dx != bot ? getEqAtomLit(dx, supports, p, sccMap) : dx;
		// create shifted rules and split disjunctions into non-hcf components
		for (VarVec::iterator hIt = head.begin(), hEnd = head.end(); hIt != hEnd; ++hIt) {
			uint32 scc = getAtom(*hIt)->scc();
			if (scc == PrgNode::noScc || (sccMap[scc] & seen_scc) != 0) {
				if (scc != PrgNode::noScc) { sccMap[scc] &= ~seen_scc; }
				else                       { scc = UINT32_MAX; }
				rule_.reset();
				rule_.head.add(*hIt);
				if (supportLit.var() != 0) { rule_.body.add(supportLit.var(), !supportLit.sign()); }
				else if (supportLit.sign()){ continue; }
				for (VarVec::iterator oIt = head.begin(); oIt != hEnd; ++oIt) {
					if (oIt != hIt) {
						if (getAtom(*oIt)->scc() == scc) { rule_.head.add(*oIt); }
						else                             { rule_.body.add(*oIt, false); }
					}
				}
				PrgBody* B = simplifyRule(RuleView(rule_.head, rule_.body)) ? assignBodyFor(activeBody_, PrgEdge::Normal, true) : 0;
				if (!B || B->value() == value_false) { continue; }
				if (activeHead_.size() == 1) {
					++shifted;
					B->addHead(getAtom(*activeHead_.begin()), PrgEdge::Normal);
				}
				else if (activeHead_.size() > 1) {
					PrgDisj* x = getDisjFor(activeHead_.atoms, 0);
					B->addHead(x, PrgEdge::Normal);
					x->assignVar(*this, *x->supps_begin());
					x->setInUpper(true);
					x->setSeen(true);
					if ((sccMap[scc] & is_scc_non_hcf) == 0) {
						sccMap[scc] |= is_scc_non_hcf;
						nonHcfs_.add(scc);
					}
					if (!options().noGamma) {
						shifter.transform(RuleView(activeHead_, activeBody_));
					}
					else {
						// only add support edge
						for (PrgDisj::atom_iterator a = x->begin(), end = x->end(); a != end; ++a) {
							B->addHead(getAtom(*a), PrgEdge::GammaChoice);
						}
					}
				}
			}
		}
	}
	if (!disjunctions_.empty() && nonHcfs_.config == 0) {
		nonHcfs_.config = ctx()->configuration()->config("tester");
	}
	upStat(RK(Normal), shifted);
	stats.nonHcfs = uint32(nonHcfs_.size()) - stats.nonHcfs;
	rule_.reset();
	setFrozen(true);
}

// optionally transform extended rules in sccs
void LogicProgram::prepareComponents() {
	int trRec = opts_.erMode == mode_transform_scc;
	// HACK: force transformation of extended rules in non-hcf components
	// REMOVE this once minimality check supports aggregates
	if (!disjunctions_.empty() && trRec != 1) {
		trRec = 2;
	}
	if (trRec != 0) {
		DlpTr tr(this, PrgEdge::Normal);
		RuleTransform trans(tr);
		setFrozen(false);
		EdgeVec heads;
		// find recursive aggregates
		for (uint32 bIdx = 0, bEnd = numBodies(); bIdx != bEnd; ++bIdx) {
			PrgBody* B = bodies_[bIdx];
			if (B->type() == Body_t::Normal || !B->hasVar() || B->value() == value_false) { continue; } // not aggregate or not relevant
			tr.scc = B->scc(*this);
			if (tr.scc == PrgNode::noScc || (trRec == 2 && !nonHcfs_.find(tr.scc))) { continue; } // not recursive
			// transform all rules a :- B, where scc(a) == scc(B):
			heads.clear();
			for (PrgBody::head_iterator hIt = B->heads_begin(), hEnd = B->heads_end(); hIt != hEnd; ++hIt) {
				assert(hIt->isAtom());
				if (getAtom(hIt->node())->scc() == tr.scc) { heads.push_back(*hIt); }
			}
			if (heads.empty()) { continue; }
			rule_.reset();
			rule_.head.reset(!isChoice(heads[0].type()) ? Head_t::Disjunctive : Head_t::Choice).add(heads[0].node());
			rule_.body.type  = B->type();
			rule_.body.bound = B->bound();
			for (uint32 i = 0; i != B->size(); ++i) {
				rule_.body.add(B->goal(i).var(), B->goal(i).sign() == false, B->weight(i));
			}
			if (heads.size() > 1) { // more than one head, make body eq to some new aux atom
				rule_.head.reset().add(tr.newAtom());
			}
			trans.transform(RuleView(rule_.head, rule_.body));
			rule_.body.reset().add(rule_.head.atoms[0], true);
			for (EdgeVec::const_iterator hIt = heads.begin(); hIt != heads.end(); ++hIt) {
				B->removeHead(getAtom(hIt->node()), hIt->type());
				if (rule_.head.atoms[0] != hIt->node()) {
					rule_.head.reset(!isChoice(hIt->type()) ? Head_t::Disjunctive : Head_t::Choice).add(hIt->node());
					tr.addRule(rule_.head.toView(), rule_.body.toView());
				}
			}
		}
		incTrAux(tr.atoms.size());
		while (!tr.atoms.empty()) {
			PrgAtom* ax = getAtom(tr.atoms.back());
			tr.atoms.pop_back();
			if (ax->supports()) {
				ax->setInUpper(true);
				ax->assignVar(*this, *ax->supps_begin());
			}
			else { assignValue(ax, value_false, PrgEdge::noEdge()); }
		}
		setFrozen(true);
	}
}

void LogicProgram::prepareOutputTable() {
	OutputTable& out = ctx()->output;
	// add new output predicates in program order to output table
	std::stable_sort(show_.begin(), show_.end(), compose22(std::less<Id_t>(), select1st<ShowPair>(), select1st<ShowPair>()));
	for (ShowVec::iterator it = show_.begin(), end = show_.end(); it != end; ++it) {
		Literal lit = getLiteral(it->first);
		if      (!isSentinel(lit))  { out.add(it->second, lit, it->first); }
		else if (lit == lit_true()) { out.add(it->second); }
	}
	if (!auxData_->project.empty()) {
		for (VarVec::const_iterator it = auxData_->project.begin(), end = auxData_->project.end(); it != end; ++it) {
			out.addProject(getLiteral(*it));
		}
	}
}

// add (completion) nogoods
bool LogicProgram::addConstraints() {
	ClauseCreator gc(ctx()->master());
	if (options().iters == 0) {
		gc.addDefaultFlags(ClauseCreator::clause_force_simplify);
	}
	ctx()->startAddConstraints();
	// handle initial conflict, if any
	if (!ctx()->ok() || !ctx()->addUnary(getTrueAtom()->trueLit())) {
		return false;
	}
	if (options().noGamma && !disjunctions_.empty()) {
		// add "rule" nogoods for disjunctions
		for (DisjList::const_iterator it = disjunctions_.begin(); it != disjunctions_.end(); ++it) {
			gc.start().add(~(*it)->literal());
			for (PrgDisj::atom_iterator a = (*it)->begin(); a != (*it)->end(); ++a) {
				gc.add(getAtom(*a)->literal());
			}
			if (!gc.end()) { return false; }
		}
	}
	// add bodies from this step
	for (BodyList::const_iterator it = bodies_.begin(); it != bodies_.end(); ++it) {
		if (!toConstraint((*it), *this, gc)) { return false; }
	}
	// add atoms thawed in this step
	for (VarIter it = unfreeze_begin(), end = unfreeze_end(); it != end; ++it) {
		if (!toConstraint(getAtom(*it), *this, gc)) { return false; }
	}
	// add atoms from this step
	const bool freezeAll = incData_ && ctx()->satPrepro.get() != 0;
	const uint32 hiAtom  = input_.hi;
	uint32 id = startAtom();
	for (AtomList::const_iterator it = atoms_.begin()+startAtom(), end = atoms_.end(); it != end; ++it, ++id) {
		if (!toConstraint(*it, *this, gc)) { return false; }
		if (id < hiAtom && (*it)->hasVar()){
			if (freezeAll) { ctx()->setFrozen((*it)->var(), true); }
			ctx()->setInput((*it)->var(), true);
		}
	}
	if (!auxData_->scc.empty()) {
		if (ctx()->sccGraph.get() == 0) {
			ctx()->sccGraph = new PrgDepGraph(static_cast<PrgDepGraph::NonHcfMapType>(opts_.oldMap == 0));
		}
		uint32 oldNodes = ctx()->sccGraph->nodes();
		ctx()->sccGraph->addSccs(*this, auxData_->scc, nonHcfs_);
		stats.ufsNodes  = ctx()->sccGraph->nodes()-oldNodes;
	}
	return true;
}

void LogicProgram::addDomRules() {
	if (auxData_->dom.empty()) { return; }
	VarVec domVec;
	EqVec eqVec;
	DomRules&  doms = auxData_->dom;
	Solver const& s = *ctx()->master();
	// mark any previous domain atoms so that we can decide
	// whether existing variables can be used for the atoms in doms
	if (incData_) {
		domVec.swap(incData_->doms);
		for (VarVec::const_iterator it = domVec.begin(); it != domVec.end(); ++it) {
			if (s.value(*it) == value_free) { ctx()->mark(posLit(*it)); }
		}
	}
	DomRules::iterator j;
	IndexMap::const_iterator eq;
	DomRule r;
	for (DomRules::iterator it = (j = doms.begin()), end = doms.end(); it != end; ++it) {
		Literal cond = getLiteral(it->cond);
		Literal slit = getLiteral(it->atom);
		Var     svar = slit.var();
		if (s.isFalse(cond) || s.value(svar) != value_free) { continue; }
		if (s.isTrue(cond)) { it->cond = 0; cond = lit_true(); }
		// check if atom is the root for its var
		if (!atomState_.isSet(it->atom, AtomState::dom_flag)) {
			if (!ctx()->marked(posLit(svar))) {
				// var(it->atom) is not yet used - make it->atom its root
				ctx()->mark(posLit(svar));
				atomState_.set(it->atom, AtomState::dom_flag);
				domVec.push_back(svar);
			}
			else if ((eq = domEqIndex_.find(it->atom)) != domEqIndex_.end()) {
				// var(it->atom) is used but we already created a new var for it->atom
				slit = posLit(svar = eq->second);
			}
			else {
				// var(it->atom) is used - introduce new aux var and make it eq to lit(atom)
				Eq n = { ctx()->addVar(Var_t::Atom, VarInfo::Nant), slit };
				eqVec.push_back(n);
				svar = n.var;
				slit = posLit(svar);
				domEqIndex_.insert(IndexMap::value_type(static_cast<uint32>(it->atom), svar));
			}
		}
		*j++ = (r = *it);
		if (slit.sign()) {
			if      (r.type == DomModType::Sign)  { r.bias = r.bias != 0 ? -r.bias : 0; }
			else if (r.type == DomModType::True)  { r.type = DomModType::False; }
			else if (r.type == DomModType::False) { r.type = DomModType::True; }
		}
		ctx()->heuristic.add(svar, static_cast<DomModType>(r.type), r.bias, r.prio, cond);
	}
	if (j != doms.end()) {
		upStat(RK(Heuristic), -static_cast<int>(doms.end() - j));
		doms.erase(j, doms.end());
	}
	// cleanup var flags
	for (VarVec::const_iterator it = domVec.begin(); it != domVec.end(); ++it) {
		ctx()->unmark(*it);
	}
	if (incData_) {
		incData_->doms.swap(domVec);
	}
	if (!eqVec.empty()) {
		ctx()->startAddConstraints();
		for (EqVec::const_iterator it = eqVec.begin(), end = eqVec.end(); it != end; ++it) {
			// it->var == it->lit
			ctx()->addBinary(~it->lit, posLit(it->var));
			ctx()->addBinary( it->lit, negLit(it->var));
		}
	}
}

void LogicProgram::addAcycConstraint() {
	AcycRules& acyc = auxData_->acyc;
	if (acyc.empty()) { return; }
	SharedContext& ctx = *this->ctx();
	ExtDepGraph* graph = ctx.extGraph.get();
	const Solver&    s = *ctx.master();
	if (graph) { graph->update(); }
	else       { ctx.extGraph = (graph = new ExtDepGraph()); }
	for (AcycRules::const_iterator it = acyc.begin(), end = acyc.end(); it != end; ++it) {
		Literal lit = getLiteral(it->cond);
		if (!s.isFalse(lit)) {
			graph->addEdge(lit, it->node[0], it->node[1]);
		}
		else {
			upStat(RK(Acyc), -1);
		}
	}
	if (graph->finalize(ctx) == 0) { ctx.extGraph = 0; }
}
#undef check_modular
/////////////////////////////////////////////////////////////////////////////////////////
// misc/helper functions
/////////////////////////////////////////////////////////////////////////////////////////
PrgAtom* LogicProgram::resize(Atom_t atomId) {
	while (atoms_.size() <= AtomList::size_type(atomId)) {
		newAtom();
	}
	return getRootAtom(atomId);
}

bool LogicProgram::propagate(bool backprop) {
	assert(frozen());
	bool oldB = opts_.backprop != 0;
	opts_.backprop = backprop;
	for (VarVec::size_type i = 0; i != propQ_.size(); ++i) {
		PrgAtom* a = getAtom(propQ_[i]);
		if (!a->relevant()) { continue; }
		if (!a->propagateValue(*this, backprop)) {
			setConflict();
			return false;
		}
		if (a->hasVar() && a->id() < startAtom() && !ctx()->addUnary(a->trueLit())) {
			setConflict();
			return false;
		}
	}
	opts_.backprop = oldB;
	propQ_.clear();
	return true;
}

// Simplifies the given body:
// For a normal body:
//   - removes true and duplicate literals: {T,a,b,a} -> {a, b}.
//   - checks for contradictions and false literalss: {a, not a} -> F
// For a weight body:
//   - removes assigned literals and updates the bound accordingly
//   - removes literals with weight 0    : L[a = 0, b = 2, c = 0, ...] -> L[b = 2, ...]
//   - reduces weights > bound() to bound: 2[a=1, b=3] -> 2[a=1, b=2]
//   - merges duplicate literals         : L[a=w1, b=w2, a=w3] -> L[a=w1+w3, b=w2]
//   - checks for contradiction, i.e.
//     rule body contains both p and not p and both are needed
//   - replaces weight constraint with cardinality constraint
//     if all weights are equal
//   - replaces weight/cardinality constraint with normal body
//     if all literals must be true for the body to be satisfied
bool LogicProgram::simplifyBody(const BodyView& body, BodyData& out, BodyData::Meta* mOut) {
	out.reset();
	BodyData::BodyLitVec& sBody = out.lits;
	sBody.reserve(Potassco::size(body));
	enum { BOT = -1 };
	int bt = body.type;
	weight_t bound = Body_t::hasBound(body.type) ? body.bound : static_cast<weight_t>(Potassco::size(body));
	BodyData::Meta meta;
	weight_t maxW  = 1;
	for (BodyView::iterator it = Potassco::begin(body), bEnd = Potassco::end(body); it != bEnd; ++it) {
		if (it->weight == 0) continue; // skip irrelevant lits
		CLASP_ASSERT_CONTRACT_MSG(it->weight > 0, "Positive weight expected!");
		PrgAtom* a = resize(Potassco::atom(*it));
		Literal  p = Literal(a->id(), it->lit < 0);// replace any eq atoms
		weight_t w = 1;
		if (bt == Body_t::Sum && (w = it->weight) > maxW) { maxW = w; }
		if (a->value() != value_free || !a->relevant()) {
			bool vSign = a->value() == value_false || !a->relevant();
			if (vSign != p.sign()) { // literal is false - drop body?
				if (bt == Body_t::Normal) { bt = BOT; break; }
				continue;
			}
			else if (a->value() != value_weak_true) {
				if ((bound -= w) <= 0) { break; }
				continue;
			}
		}
		if (!atomState_.inBody(p)) {  // literal not seen yet
			atomState_.addToBody(p);
			out.add(p.var(), !p.sign(), w);
			meta.pos  += !p.sign();
			meta.hash += hashLit(p);
		}
		else if (bt != Body_t::Normal) { // Merge duplicate lits
			BodyData::LitType& oldP = *out.find(p);
			weight_t oldW = oldP.weight;
			bt = Body_t::Sum;
			CLASP_ASSERT_CONTRACT_MSG((CLASP_WEIGHT_T_MAX-oldW)>= w, "Integer overflow!");
			if ((oldP.weight += w) > maxW) {
				maxW = oldP.weight;
			}
		}
		else {
			bound -= 1;
		}
		if (atomState_.inBody(~p)) {
			if (bt == Body_t::Normal) { bt = BOT; break; }
			++maxW;
		}
	}
	if (bound > 0 && bt > int(Body_t::Normal)) {
		weight_t minW  = 1;
		wsum_t sumR = (wsum_t)sBody.size();
		wsum_t sumW = (wsum_t)sBody.size();
		if (maxW > 1) {
			maxW = 1;
			minW = CLASP_WEIGHT_T_MAX;
			sumW = sumR = 0;
			for (unsigned i = 0; i != sBody.size(); ++i) {
				if (sBody[i].weight > bound) { sBody[i].weight = bound; }
				Literal  p = toLit(sBody[i].lit);
				weight_t w = sBody[i].weight;
				minW = std::min(minW, w);
				maxW = std::max(maxW, w);
				sumW += w;
				sumR += w;
				if (atomState_.inBody(~p) && p.sign()) {
					// body contains p and ~p: we can achieve at most max(weight(p), weight(~p))
					sumR -= std::min(w, out.find(~p)->weight);
				}
			}
		}
		if (sumR < bound) {
			bt = BOT;
		}
		else if ((sumW - minW) < bound) {
			bt    = Body_t::Normal;
			bound = (weight_t)sBody.size();
		}
		else if (minW == maxW) {
			bt    = Body_t::Count;
			bound = (bound+(minW-1))/minW;
		}
	}
	if (bound <= 0 || bt == BOT) {
		while (!sBody.empty()) { atomState_.clearRule(Potassco::atom(sBody.back())); sBody.pop_back(); }
		out.reset();
		if (bt == BOT) { return false; }
		bt = Body_t::Normal;
	}
	if (bt != Body_t::Sum && maxW > 1) {
		for (BodyData::BodyLitVec::iterator it = sBody.begin(), end = sBody.end(); it != end; ++it) {
			it->weight = 1;
		}
	}
	out.type  = static_cast<Body_t>(bt);
	out.bound = bound;
	if (mOut) {
		meta.id = findEqBody(out, meta.hash);
		*mOut = meta;
	}
	return true;
}

bool LogicProgram::simplifyRule(const RuleView& r, HeadData& head, BodyData& body, BodyData::Meta* meta) {
	head.reset();
	if (!simplifyBody(r.body, body, meta)) { return false; }
	uint32 taut = 0;
	const bool weights = body.type == Body_t::Sum;
	const bool choice = r.head.type == Head_t::Choice;
	wsum_t sum = !weights ? (wsum_t)body.size() : (wsum_t)-1;
	for (HeadView::iterator it = Potassco::begin(r.head), end = Potassco::end(r.head); it != end; ++it) {
		if (!atomState_.isSet(*it, AtomState::simp_mask)) {
			head.add(*it);
			atomState_.addToHead(*it);
		}
		else if (!atomState_.isSet(*it, AtomState::head_flag)) {
			weight_t wPos = atomState_.inBody(posLit(*it)), wNeg = atomState_.inBody(negLit(*it));
			if (wPos && weights) { wPos = body.find(posLit(*it))->weight; }
			if (wNeg && weights) { wNeg = body.find(negLit(*it))->weight; }
			if (sum == -1) { sum  = body.sum(); }
			if (atomState_.isFact(*it) || (sum - wPos) < body.bound) {
				taut += uint32(!choice);
			}
			else if (!atomState_.isSet(*it, AtomState::false_flag) && !((sum - wNeg) < body.bound)) {
				head.add(*it);
				atomState_.addToHead(*it);
			}
		}
	}
	for (HeadData::iterator it = head.begin(), end = head.end(); it != end; ++it) {
		atomState_.clearRule(*it);
	}
	bool ok = !taut && (!choice || !head.empty());
	if (ok) { head.type = r.head.type; }
	for (BodyData::iterator it = body.begin(), end = body.end(); it != end; ++it) {
		atomState_.clearRule(Potassco::atom(*it));
	}
	return ok;
}

// create new atom aux representing supports, i.e.
// aux == S1 v ... v Sn
Literal LogicProgram::getEqAtomLit(Literal lit, const BodyList& supports, Preprocessor& p, const SccMap& sccMap) {
	if (supports.empty() || lit == lit_false()) { return lit_false(); }
	if (supports.size() == 1 && supports[0]->size() < 2) {
		return supports[0]->size() == 0 ? lit_true() : supports[0]->goal(0);
	}
	if (p.getRootAtom(lit) != varMax)         { return posLit(p.getRootAtom(lit)); }
	incTrAux(1);
	Atom_t auxV  = newAtom();
	PrgAtom* aux = getAtom(auxV);
	uint32 scc   = PrgNode::noScc;
	aux->setLiteral(lit);
	aux->setSeen(true);
	p.setRootAtom(aux->literal(), auxV);
	for (BodyList::const_iterator sIt = supports.begin(); sIt != supports.end(); ++sIt) {
		PrgBody* b = *sIt;
		if (b->relevant() && b->value() != value_false) {
			for (uint32 g = 0; scc == PrgNode::noScc && g != b->size() && !b->goal(g).sign(); ++g) {
				uint32 aScc = getAtom(b->goal(g).var())->scc();
				if (aScc != PrgNode::noScc && (sccMap[aScc] & 1u)) { scc = aScc; }
			}
			b->addHead(aux, PrgEdge::Normal);
			if (b->value() != aux->value()) { assignValue(aux, b->value(), PrgEdge::newEdge(*b, PrgEdge::Normal)); }
			aux->setInUpper(true);
		}
	}
	if (!aux->inUpper()) {
		aux->setValue(value_false);
		return lit_false();
	}
	else if (scc != PrgNode::noScc) {
		aux->setScc(scc);
		auxData_->scc.push_back(aux);
	}
	return posLit(auxV);
}

PrgBody* LogicProgram::getBodyFor(const SBody& body, bool addDeps) {
	if (body.meta.id < bodies_.size()) {
		return getBody(body.meta.id);
	}
	// no corresponding body exists, create a new object
	body.meta.id = numBodies();
	PrgBody* b = PrgBody::create(*this, body.meta, body.toView(), addDeps);
	bodyIndex_.insert(IndexMap::value_type(body.meta.hash, body.meta.id));
	bodies_.push_back(b);
	if (b->isSupported()) {
		initialSupp_.push_back(body.meta.id);
	}
	upStat(body.type);
	return b;
}
PrgBody* LogicProgram::getTrueBody() {
	activeBody_.reset();
	activeBody_.meta.id = std::min(findEqBody(activeBody_, 0), numBodies());
	return getBodyFor(activeBody_);
}

PrgBody* LogicProgram::assignBodyFor(const SBody& body, EdgeType depEdge, bool simpStrong) {
	PrgBody* b = getBodyFor(body, depEdge != PrgEdge::Gamma);
	if (!b->hasVar() && !b->seen()) {
		uint32 eqId;
		b->markDirty();
		b->simplify(*this, simpStrong, &eqId);
		if (eqId != b->id()) {
			assert(b->id() == bodies_.size()-1);
			removeBody(b, body.meta.hash);
			bodies_.pop_back();
			if (depEdge != PrgEdge::Gamma) {
				for (uint32 i = 0; i != b->size(); ++i) {
					getAtom(b->goal(i).var())->removeDep(b->id(), !b->goal(i).sign());
				}
			}
			b->destroy();
			b = bodies_[eqId];
		}
	}
	b->setSeen(true);
	b->assignVar(*this);
	return b;
}
bool LogicProgram::equalLits(const PrgBody& b, const BodyData::BodyLitVec& lits, bool sorted) {
	BodyData::BodyLitVec::const_iterator lBeg = lits.begin(), lEnd = lits.end();
	bool ok = true;
	for (uint32 i = 0, end = b.size(); i != end && ok; ++i) {
		Potassco::WeightLit_t wl = { toInt(b.goal(i)), b.weight(i) };
		ok = !sorted ? std::find(lBeg, lEnd, wl) != lEnd : std::binary_search(lBeg, lEnd, wl);
	}
	return ok;
}
// Pre: all literals in body are marked!
uint32 LogicProgram::findEqBody(BodyData& body, uint32 hash) {
	IndexRange bodies = bodyIndex_.equal_range(hash);
	bool sorted = false;
	for (IndexIter it = bodies.first; it != bodies.second; ++it) {
		const PrgBody& b = *getBody(it->second);
		if (!checkBody(b, body.type, body.size(), body.bound)) {
			continue;
		}
		if (!b.hasWeights()) {
			if (atomState_.inBody(b.goals_begin(), b.goals_end())) {
				return b.id();
			}
		}
		else {
			if (body.size() > 10 && !sorted) {
				std::sort(body.lits.begin(), body.lits.end());
				sorted = true;
			}
			if (equalLits(b, body.lits, sorted)) { return b.id(); }
		}
	}
	return varMax;
}
uint32 LogicProgram::findEqBody(const PrgBody* b, uint32 hash) {
	IndexRange bodies = bodyIndex_.equal_range(hash);
	if (bodies.first == bodies.second)  { return varMax;  }
	uint32 eqId = varMax, n = 0;
	BodyData::BodyLitVec& lits = activeBody_.lits; lits.clear();
	for (IndexIter it = bodies.first; it != bodies.second && eqId == varMax; ++it) {
		const PrgBody& rhs = *getBody(it->second);
		if (!checkBody(rhs, b->type(), b->size(), b->bound())) { continue; }
		if      (b->size() == 0)  { eqId = rhs.id(); }
		else if (b->size() == 1)  { eqId = b->goal(0) == rhs.goal(0) && b->weight(0) == rhs.weight(0) ? rhs.id() : varMax; }
		else if (!b->hasWeights()){
			if (++n == 1) { std::for_each(b->goals_begin(), b->goals_end(), std::bind1st(std::mem_fun(&AtomState::addToBody), &atomState_)); }
			if (atomState_.inBody(rhs.goals_begin(), rhs.goals_end())) { eqId = rhs.id(); }
		}
		else {
			if (++n == 1) {
				for (uint32 i = 0, end = b->size(); i != end; ++i) {
					Potassco::WeightLit_t wl = { toInt(b->goal(i)), b->weight(i) };
					lits.push_back(wl);
				}
				if (lits.size() > 10) { std::sort(lits.begin(), lits.end()); }
			}
			if (equalLits(rhs, lits, lits.size() > 10)) { eqId = rhs.id(); }
		}
	}
	if (n && !b->hasWeights()) {
		std::for_each(b->goals_begin(), b->goals_end(), std::bind1st(std::mem_fun(&AtomState::clearBody), &atomState_));
	}
	return eqId;
}

PrgDisj* LogicProgram::getDisjFor(const VarVec& heads, uint32 headHash) {
	PrgDisj* d = 0;
	if (headHash) {
		LogicProgram::IndexRange eqRange = disjIndex_.equal_range(headHash);
		for (; eqRange.first != eqRange.second; ++eqRange.first) {
			PrgDisj& o = *disjunctions_[eqRange.first->second];
			if (o.relevant() && o.size() == heads.size() && atomState_.allMarked(o.begin(), o.end(), AtomState::head_flag)) {
				assert(o.id() == eqRange.first->second);
				d = &o;
				break;
			}
		}
		for (VarVec::const_iterator it = heads.begin(), end = heads.end(); it != end; ++it) {
			atomState_.clearRule(*it);
		}
	}
	if (!d) {
		// no corresponding disjunction exists, create a new object
		// and link it to all atoms
		++stats.disjunctions[statsId_];
		uint32 id = disjunctions_.size();
		d         = PrgDisj::create(id, heads);
		disjunctions_.push_back(d);
		PrgEdge edge = PrgEdge::newEdge(*d, PrgEdge::Choice);
		for (VarVec::const_iterator it = heads.begin(), end = heads.end(); it != end; ++it) {
			getAtom(*it)->addSupport(edge);
		}
		if (headHash) {
			disjIndex_.insert(IndexMap::value_type(headHash, d->id()));
		}
	}
	return d;
}

// body has changed - update index
uint32 LogicProgram::update(PrgBody* body, uint32 oldHash, uint32 newHash) {
	uint32 id   = removeBody(body, oldHash);
	if (body->relevant()) {
		uint32 eqId = findEqBody(body, newHash);
		if (eqId == varMax) {
			// No equivalent body found.
			// Add new entry to index
			bodyIndex_.insert(IndexMap::value_type(newHash, id));
		}
		return eqId;
	}
	return varMax;
}

// body b has changed - remove old entry from body node index
uint32 LogicProgram::removeBody(PrgBody* b, uint32 hash) {
	IndexRange ra = bodyIndex_.equal_range(hash);
	uint32 id     = b->id();
	for (; ra.first != ra.second; ++ra.first) {
		if (bodies_[ra.first->second] == b) {
			id = ra.first->second;
			bodyIndex_.erase(ra.first);
			break;
		}
	}
	return id;
}

PrgAtom* LogicProgram::mergeEqAtoms(PrgAtom* a, Id_t rootId) {
	PrgAtom* root = getAtom(rootId = getRootId(rootId));
	ValueRep mv   = getMergeValue(a, root);	
	assert(!a->eq() && !root->eq() && !a->frozen());
	if (a->ignoreScc()) { root->setIgnoreScc(true); }
	if (mv != a->value()    && !assignValue(a, mv, PrgEdge::noEdge()))   { return 0; }
	if (mv != root->value() && !assignValue(root, mv, PrgEdge::noEdge())){ return 0; }
	a->setEq(rootId);
	incEqs(Var_t::Atom);
	return root;
}

// returns whether posSize(root) <= posSize(body)
bool LogicProgram::positiveLoopSafe(PrgBody* body, PrgBody* root) const {
	uint32 i = 0, end = std::min(body->size(), root->size());
	while (i != end && body->goal(i).sign() == root->goal(i).sign()) { ++i; }
	return i == root->size() || root->goal(i).sign();	
}

PrgBody* LogicProgram::mergeEqBodies(PrgBody* b, Id_t rootId, bool hashEq, bool atomsAssigned) {
	PrgBody* root = getBody(rootId = getEqNode(bodies_, rootId));
	bool     bp   = options().backprop != 0;
	if (b == root) { return root; }
	assert(!b->eq() && !root->eq() && (hashEq || b->literal() == root->literal()));
	if (!b->simplifyHeads(*this, atomsAssigned) || (b->value() != root->value() && (!mergeValue(b, root) || !root->propagateValue(*this, bp) || !b->propagateValue(*this, bp)))) {
		setConflict();
		return 0;
	}
	if (hashEq || positiveLoopSafe(b, root)) {
		b->setLiteral(root->literal());
		if (!root->mergeHeads(*this, *b, atomsAssigned, !hashEq)) {
			setConflict();
			return 0;
		}
		incEqs(Var_t::Body);
		b->setEq(rootId);
		return root;
	}
	return b;
}

const char* LogicProgram::findName(Atom_t x) const {
	for (OutputTable::pred_iterator it = ctx()->output.pred_begin(), end = ctx()->output.pred_end(); it != end; ++it) {
		if (it->user == x) { return it->name; }
	}
	for (ShowVec::const_iterator it = show_.begin(), end = show_.end(); it != end; ++it) {
		if (it->first == x){ return it->second; }
	}
	return "";
}
VarVec& LogicProgram::getSupportedBodies(bool sorted) {
	if (sorted) {
		std::stable_sort(initialSupp_.begin(), initialSupp_.end(), LessBodySize(bodies_));
	}
	return initialSupp_;
}

Atom_t LogicProgram::falseAtom() {
	Atom_t aFalse = 0;
	for (VarVec::size_type i = 1; i < atoms_.size() && !aFalse; ++i) {
		if (atoms_[i]->value() == value_false || atomState_.isSet(i, AtomState::false_flag)) {
			aFalse = i;
		}
	}
	if (!aFalse) {
		bool s = frozen();
		setFrozen(false);
		aFalse = newAtom();
		assignValue(getAtom(aFalse), value_false, PrgEdge::noEdge());
		setFrozen(s);
	}
	return aFalse;
}

bool LogicProgram::extractCondition(Id_t id, Potassco::LitVec& out) const {
	out.clear();
	if (id == Clasp::Asp::falseId || (frozen() && getLiteral(id) == lit_false())) { return false; }
	if (!id || isAtom(id)) {
		out.assign(id != 0, Potassco::lit(id));
		return true;
	}
	Id_t bId = nodeId(id);
	CLASP_FAIL_IF(!validBody(bId), "Invalid literal");
	const PrgBody* B = getBody(getEqBody(bId));
	out.reserve(B->size());
	for (PrgBody::goal_iterator it = B->goals_begin(), end = B->goals_end(); it != end; ++it) {
		out.push_back( toInt(*it) );
	}
	return true;
}
#undef RT

} } // end namespace Asp

