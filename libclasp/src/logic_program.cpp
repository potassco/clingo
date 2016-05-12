//
// Copyright (c) 2013 Benjamin Kaufmann
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
#include <stdexcept>
#include <sstream>
#include <climits>
namespace Clasp { namespace Asp {
/////////////////////////////////////////////////////////////////////////////////////////
// class LpStats
/////////////////////////////////////////////////////////////////////////////////////////
#define LP_STATS( APPLY )                                        \
	APPLY("bodies"              , bodies)                        \
	APPLY("atoms"               , atoms)                         \
	APPLY("atoms_aux"           , auxAtoms)                      \
	APPLY("sccs"                , sccs)                          \
	APPLY("sccs_non_hcf"        , nonHcfs)                       \
	APPLY("gammas"              , gammas)                        \
	APPLY("ufs_nodes"           , ufsNodes)                      \
	APPLY("rules"               , rules())                       \
	APPLY("rules_basic"         , rules(BASICRULE).first)        \
	APPLY("rules_choice"        , rules(CHOICERULE).first)       \
	APPLY("rules_constraint"    , rules(CONSTRAINTRULE).first)   \
	APPLY("rules_weight"        , rules(WEIGHTRULE).first)       \
	APPLY("rules_disjunctive"   , rules(DISJUNCTIVERULE).first)  \
	APPLY("rules_optimize"      , rules(OPTIMIZERULE).first)     \
	APPLY("rules_tr_basic"      , rules(BASICRULE).second)       \
	APPLY("rules_tr_choice"     , rules(CHOICERULE).second)      \
	APPLY("rules_tr_constraint" , rules(CONSTRAINTRULE).second)  \
	APPLY("rules_tr_weight"     , rules(WEIGHTRULE).second)      \
	APPLY("rules_tr_disjunctive", rules(DISJUNCTIVERULE).second) \
	APPLY("rules_tr_optimize"   , rules(OPTIMIZERULE).second)    \
	APPLY("eqs"                 , eqs())                         \
	APPLY("eqs_atom"            , eqs(Var_t::atom_var))          \
	APPLY("eqs_body"            , eqs(Var_t::body_var))          \
	APPLY("eqs_other"           , eqs(Var_t::atom_body_var))

void LpStats::reset() {
	std::memset(this, 0, sizeof(LpStats));
}
uint32 LpStats::rules() const {
	uint32 sum = 0;
	for (uint32 i = 0; i != NUM_RULE_TYPES; ++i) { sum += rules_[i].second; }
	return sum;
}
void LpStats::accu(const LpStats& o) {
	bodies   += o.bodies;
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
	for (int i = 0; i != sizeof(eqs_)/sizeof(eqs_[0]); ++i) {
		eqs_[i] += o.eqs_[i];
	}
	for (int i = 0; i != sizeof(rules_)/sizeof(rules_[0]); ++i) {
		rules_[i].first  += o.rules_[i].first;
		rules_[i].second += o.rules_[i].second;
	}
}

double LpStats::operator[](const char* key) const {
#define MAP_IF(x, A) if (std::strcmp(key, x) == 0)  return double( (A) );
	LP_STATS(MAP_IF)
#undef MAP_IF
	return -1.0;
}
const char* LpStats::keys(const char* path) {
	if (!path || !*path) {
#define KEY(x, y) x "\0"
		return LP_STATS(KEY);
#undef KEY
	}
	return 0;
}
#undef LP_STATS
/////////////////////////////////////////////////////////////////////////////////////////
// class LogicProgram
/////////////////////////////////////////////////////////////////////////////////////////
namespace {
struct LessBodySize {
	LessBodySize(const BodyList& bl) : bodies_(&bl) {}
	bool operator()(Var b1, Var b2 ) const {
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
LogicProgram::LogicProgram() : nonHcfCfg_(0), minimize_(0), incData_(0), startAux_(0) { accu = 0; }
LogicProgram::~LogicProgram() { dispose(true); }
LogicProgram::Incremental::Incremental() : startAtom(1), startScc(0) {}
void LogicProgram::dispose(bool force) {
	// remove rules
	std::for_each( bodies_.begin(), bodies_.end(), DestroyObject() );
	std::for_each( disjunctions_.begin(), disjunctions_.end(), DestroyObject() );
	AtomList().swap(sccAtoms_);
	BodyList().swap(bodies_);
	DisjList().swap(disjunctions_);
	bodyIndex_.clear();
	disjIndex_.clear();
	for (MinimizeRule* r = minimize_, *t; r; ) {
		t = r;
		r = r->next_;
		delete t;
	}
	minimize_ = 0;
	for (RuleList::size_type i = 0; i != extended_.size(); ++i) {
		delete extended_[i];
	}
	extended_.clear();
	VarVec().swap(initialSupp_);
	rule_.clear();
	if (force || !incData_) {
		std::for_each( atoms_.begin(), atoms_.end(), DeleteObject() );
		AtomList().swap(atoms_);
		delete incData_;
		VarVec().swap(propQ_);
		ruleState_.clearAll();
		stats.reset();
		startAux_ = atoms_.size();
	}
	else {
		// make sure that we have a reference to any minimize constraint
		getMinimizeConstraint();
	}
	activeHead_.clear();
	activeBody_.reset();
}
bool LogicProgram::doStartProgram() {
	dispose(true);
	// atom 0 is always false
	atoms_.push_back( new PrgAtom(0, false) );
	assignValue(getAtom(0), value_false);
	getFalseAtom()->setLiteral(negLit(0));
	nonHcfCfg_= 0;
	incData_  = 0;
	ctx()->symbolTable().clear();
	ctx()->symbolTable().startInit(SymbolTable::map_indirect);
	return true;
}
void LogicProgram::setOptions(const AspOptions& opts) {
	opts_ = opts;
	if (opts.suppMod) {
		if (!incData_ || incData_->startScc == 0) {
			if (opts_.iters && ctx()) { ctx()->report(warning(Event::subsystem_prepare, "'supp-models' implies 'eq=0'.")); }
			opts_.supportedModels();
		}
		else {
			if (ctx()) { ctx()->report(warning(Event::subsystem_facade, "'supp-models' ignored for non-tight programs.")); }
			opts_.suppMod = 0;
			opts_.noSCC   = 0;
		}
	}
}
bool LogicProgram::doParse(StreamSource& prg) { return DefaultLparseParser(*this).parse(prg); }
bool LogicProgram::doUpdateProgram() {
	if (!incData_) { incData_ = new Incremental(); ctx()->symbolTable().incremental(true); }
	if (!frozen()) { return true; }
	// delete bodies/disjunctions...
	dispose(false);
	setFrozen(false);
	incData_->update.clear();
	incData_->frozen.swap(incData_->update);
	ctx()->symbolTable().startInit(SymbolTable::map_indirect);
	// reset prop queue and add supported atoms from previous steps 
	// {ai | ai in P}.
	PrgBody* support = startAux_ > 1 ? getBodyFor(activeBody_) : 0;
	propQ_.assign(1, getFalseId());
	for (Var i = 1, end = startAux_; i != end; ++i) {
		if (getEqAtom(i) >= startAux_) {
			// atom is equivalent to some aux atom - make i the new root
			uint32 r = getEqAtom(i);
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
			if (v != value_free) { assignValue(a, v); }
			if (!a->frozen() && a->value() != value_false) {
				a->setIgnoreScc(true);
				support->addHead(a, PrgEdge::CHOICE_EDGE);
			}
		}
		else if (a->removed() || (!a->eq() && a->value() == value_false)) {
			a->setEq(getFalseId());
		}
	}
	// delete any introduced aux atoms
	// this is safe because aux atoms are never part of the input program
	// it is necessary in order to free their ids, i.e. the id of an aux atom
	// from step I might be needed for a program atom in step I+1
	std::for_each(atoms_.begin()+startAux_, atoms_.end(), DeleteObject());
	atoms_.erase(atoms_.begin()+startAux_, atoms_.end());
	incData_->startAtom = startAux_ = (uint32)atoms_.size();
	stats.reset();
	return true;
}
bool LogicProgram::doEndProgram() {
	if (!frozen() && ctx()->ok()) {
		prepareProgram(!opts_.noSCC);
		addConstraints();
		if (accu) { accu->accu(stats); }
	}
	return ctx()->ok();
}

bool LogicProgram::clone(SharedContext& oCtx, bool shareSymbols) {
	assert(frozen());
	if (&oCtx == ctx()) {
		return true;
	}
	oCtx.cloneVars(*ctx(), shareSymbols ? SharedContext::init_share_symbols : SharedContext::init_copy_symbols);
	SharedContext* t = ctx();
	setCtx(&oCtx);
	bool r = addConstraints();
	setCtx(t);
	return r;
}

void LogicProgram::addMinimize() {
	CLASP_ASSERT_CONTRACT(frozen());
	if (hasMinimize()) {
		if (opts_.iters != 0) { simplifyMinimize(); }
		WeightLitVec lits;
		for (MinimizeRule* r = minimize_; r; r = r->next_) {
			for (WeightLitVec::iterator it = r->lits_.begin(); it != r->lits_.end(); ++it) {
				PrgAtom* h = resize(it->first.var()); // checks for eq
				lits.push_back(WeightLiteral(it->first.sign() ? ~h->literal() : h->literal(), it->second));
			}
			addMinRule(lits);
			lits.clear();
		}
	}
}
static std::ostream& operator<<(std::ostream& out, const BodyInfo& body) {
	if (body.type() == BodyInfo::SUM_BODY && body.bound() != -1) { out << body.bound() << " "; }
	out << body.size() << " ";
	out << (body.size() - body.posSize()) << " ";
	if (body.type() == BodyInfo::COUNT_BODY) { out << body.bound() << " "; }
	for (WeightLitVec::const_iterator it = body.lits.begin(), end = body.lits.end(); it != end; ++it) {
		out << it->first.var() << " ";
	}
	if (body.type() == BodyInfo::SUM_BODY) {
		for (WeightLitVec::const_iterator it = body.lits.begin(), end = body.lits.end(); it != end; ++it) {
			out << it->second << " ";
		}
	}
	return out;
}

void LogicProgram::write(std::ostream& os) {
	const char* const delimiter = "0";
	// first write all minimize rules - revert order!
	PodVector<MinimizeRule*>::type mr;
	for (MinimizeRule* r = minimize_; r; r = r->next_) {
		mr.push_back(r);
	}
	for (PodVector<MinimizeRule*>::type::reverse_iterator rit = mr.rbegin(); rit != mr.rend(); ++rit) {
		transform(**rit, activeBody_);
		os << OPTIMIZERULE << " " << 0 << " " << activeBody_ << "\n";
	}
	uint32 falseAtom = 0;
	bool  oldFreeze  = frozen();
	setFrozen(false);
	// write all bodies together with their heads
	for (BodyList::iterator bIt = bodies_.begin(); bIt != bodies_.end(); ++bIt) {
		PrgBody* b = *bIt;
		if (b->relevant() && (b->hasVar() || b->value() == value_false) && transform(*b, activeBody_)) {
			if (b->value() == value_false) {
				// write integrity constraint
				if (falseAtom == 0 && (falseAtom = findLpFalseAtom()) == 0) {
					setCompute(falseAtom = newAtom(), false);
				}
				os << activeBody_.ruleType() << " " << falseAtom << " " << activeBody_ << "\n";
			}
			else {
				activeHead_.clear(); uint32 nDisj = 0;
				for (PrgBody::head_iterator hIt = b->heads_begin(); hIt != b->heads_end(); ++hIt) {
					if (!getHead(*hIt)->hasVar() || hIt->isGamma()) { continue; }
					if (hIt->isAtom()) {
						if (hIt->isNormal()) { 
							os << activeBody_.ruleType() << " " << hIt->node() << " " << activeBody_ << "\n"; 
							if (activeBody_.type() != BodyInfo::NORMAL_BODY && getHead(*hIt)->literal() == b->literal()) {
								activeBody_.reset();
								Literal lit = posLit(hIt->node());
								activeBody_.init(BodyInfo::NORMAL_BODY, 1, hashLit(lit), 1);
								activeBody_.lits.push_back(WeightLiteral(lit, 1));
							}
						}
						else if (hIt->isChoice()) { activeHead_.push_back(hIt->node()); }
					}
					else if (hIt->isDisj()) { ++nDisj; }
				}
				if (!activeHead_.empty()) {
					os << CHOICERULE << " " << activeHead_.size() << " ";
					std::copy(activeHead_.begin(), activeHead_.end(), std::ostream_iterator<uint32>(os, " "));
					assert(activeBody_.type() == BodyInfo::NORMAL_BODY);
					os << activeBody_ << "\n";
				}
				if (nDisj) {
					assert(activeBody_.type() == BodyInfo::NORMAL_BODY);
					for (PrgBody::head_iterator hIt = b->heads_begin(); hIt != b->heads_end(); ++hIt) {
						if (hIt->isDisj()) {
							PrgDisj* d = getDisj(hIt->node());
							os << DISJUNCTIVERULE << " " << d->size() << " ";
							for (PrgDisj::atom_iterator a = d->begin(), aEnd = d->end(); a != aEnd; ++a) {
								os << a->node() << " ";
							}
							os << activeBody_ << "\n";
							if (--nDisj == 0) break;
						}
					}
				}
			}
		}
	}
	// write eq-atoms, symbol-table and compute statement
	std::stringstream bp, bm, symTab;
	Literal comp;
	SymbolTable::const_iterator sym = ctx()->symbolTable().begin();
	for (AtomList::size_type i = 1; i < atoms_.size(); ++i) {
		// write the equivalent atoms
		if (atoms_[i]->eq()) {
			PrgAtom* eq = getAtom(getEqAtom(Var(i)));
			if (eq->inUpper() && eq->value() != value_false) {
				os << "1 " << i << " 1 0 " << getEqAtom(Var(i)) << " \n";
			}
		}
		if ( (i == falseAtom || atoms_[i]->inUpper()) && atoms_[i]->value() != value_free ) {
			std::stringstream& str = atoms_[i]->value() == value_false ? bm : bp;
			str << i << "\n";
		}
		if (sym != ctx()->symbolTable().end() && Var(i) == sym->first) {
			if (sym->second.lit != negLit(sentVar) && !sym->second.name.empty()) {
				symTab << i << " " << sym->second.name.c_str() << "\n";
			}
			++sym;
		}
	}
	os << delimiter << "\n";
	os << symTab.str();
	os << delimiter << "\n";
	os << "B+\n" << bp.str() << "0\n"
		 << "B-\n" << bm.str() << "0\n1\n";
	setFrozen(oldFreeze);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Program mutating functions
/////////////////////////////////////////////////////////////////////////////////////////
#define check_not_frozen() CLASP_ASSERT_CONTRACT_MSG(!frozen(), "Can't update frozen program!")
#define check_modular(x, atomId) (void)( (!!(x)) || (throw RedefinitionError((atomId), this->getAtomName((atomId))), 0))
RedefinitionError::RedefinitionError(unsigned atomId, const char* name) : std::logic_error(clasp_format_error("Program not modular: Redefinition of atom <%u,'%s'>", atomId, name)) {
}

Var LogicProgram::newAtom() {
	check_not_frozen();
	Var id = static_cast<Var>(atoms_.size());
	atoms_.push_back( new PrgAtom(id) );
	return id;
}

LogicProgram& LogicProgram::setAtomName(Var atomId, const char* name) {
	check_not_frozen();
	check_modular(isNew(atomId), atomId);
	resize(atomId);
	ctx()->symbolTable().addUnique(atomId, name);
	return *this;
}

LogicProgram& LogicProgram::setCompute(Var atomId, bool pos) {
	resize(atomId);
	ValueRep v = pos ? value_weak_true : value_false;
	PrgAtom* a = atoms_[atomId];
	assert(!a->hasVar() || a->frozen());
	assignValue(a, v);
	return *this;
}

PrgAtom* LogicProgram::setExternal(Var atomId, ValueRep v) {
	PrgAtom* a = resize(atomId);
	if (a->frozen() || (isNew(atomId) && !a->supports())) {
		if (!incData_)    { incData_ = new Incremental(); }
		if (!a->frozen()) { incData_->update.push_back(a->id()); }
		a->markFrozen(v);
		return a;
	}
	return 0; // atom is defined or from a previous step - ignore!
}

LogicProgram& LogicProgram::freeze(Var atomId, ValueRep value) {
	check_not_frozen();
	CLASP_ASSERT_CONTRACT(value < value_weak_true);
	setExternal(atomId, value);
	return *this;
}

LogicProgram& LogicProgram::unfreeze(Var atomId) {
	check_not_frozen();
	PrgAtom* a = setExternal(atomId, value_free);
	if (a && a->supports() == 0) {
		// add dummy edge - will be removed once we update the set of frozen atoms
		a->addSupport(PrgEdge::noEdge());
	}
	return *this;
}

bool LogicProgram::isExternal(Var aId) const {
	if (!aId || aId > numAtoms()) { return false; }
	PrgAtom* a = getAtom(getEqAtom(aId));
	return a->frozen() && (a->supports() == 0 || frozen());
}

LogicProgram& LogicProgram::addRule(const Rule& r) {
	check_not_frozen();
	// simplify rule
	RuleType t = simplifyRule(r, activeHead_, activeBody_);
	if (t != ENDRULE) { // rule is relevant
		upRules(t, 1);
		if (handleNatively(t, activeBody_)) { // and can be handled natively
			addRuleImpl(t, activeHead_, activeBody_);
		}
		else {
			bool  aux  = transformNoAux(t, activeBody_) == false;
			Rule* temp = new Rule();
			temp->setType(t);
			temp->setBound(activeBody_.bound());
			temp->heads.swap(activeHead_);
			temp->body.swap(activeBody_.lits);
			if (aux) {
				// Since rule transformation needs aux atoms, we must
				// defer the transformation until all rules were added
				// because only then we can safely assign new unique consecutive atom ids.
				extended_.push_back(temp);
			}
			else {
				RuleTransform rt;
				incTr(t, rt.transformNoAux(*this, *temp));
				delete temp;
			}
		}
	}
	activeBody_.reset();
	return *this;
}
#undef check_not_frozen
/////////////////////////////////////////////////////////////////////////////////////////
// Query functions
/////////////////////////////////////////////////////////////////////////////////////////
Literal LogicProgram::getLiteral(Var atomId) const {
	CLASP_ASSERT_CONTRACT_MSG(atomId < atoms_.size(), "Atom out of bounds!");
	return getAtom(getEqAtom(atomId))->literal();
}
void LogicProgram::doGetAssumptions(LitVec& out) const {
	if (incData_) {
		for (VarVec::const_iterator it = incData_->frozen.begin(), end = incData_->frozen.end(); it != end; ++it) {
			Literal lit = getAtom(getEqAtom(*it))->assumption();
			if (lit != posLit(0)) { out.push_back( lit ); }
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// Program definition - private
/////////////////////////////////////////////////////////////////////////////////////////
void LogicProgram::addRuleImpl(RuleType r, const VarVec& heads, BodyInfo& body) {
	if (r != OPTIMIZERULE) {
		assert(!heads.empty() && (r != DISJUNCTIVERULE || heads.size() > 1));
		PrgBody* b = getBodyFor(body);
		// only a non-false body can define atoms
		if (b->value() != value_false) {
			EdgeType t      = r != CHOICERULE ? PrgEdge::NORMAL_EDGE : PrgEdge::CHOICE_EDGE;
			uint32 headHash = 0;
			bool ignoreScc  = opts_.noSCC || b->size() == 0;
			for (VarVec::const_iterator it = heads.begin(), end = heads.end(); it != end; ++it) {
				PrgAtom* a = resize(*it);
				check_modular(isNew(*it) || a->frozen() || a->value() == value_false, *it);
				if (r != DISJUNCTIVERULE) {
					// Note: b->heads may now contain duplicates. They are removed in PrgBody::simplifyHeads.
					b->addHead(a, t);
					if (ignoreScc) { a->setIgnoreScc(ignoreScc); }
				}
				else {
					headHash += hashId(*it);
					ruleState_.addToHead(*it);
				}
			}
			if (r == DISJUNCTIVERULE) {
				assert(headHash != 0);
				PrgDisj* d = getDisjFor(heads, headHash);
				b->addHead(d, t);
			}
		}	
	}
	else {
		CLASP_ASSERT_CONTRACT(heads.empty());
		LogicProgram::MinimizeRule* mr = new LogicProgram::MinimizeRule;
		mr->lits_ = body.lits;
		mr->next_ = minimize_;
		minimize_ = mr;
	}
}

bool LogicProgram::assignValue(PrgAtom* a, ValueRep v) {
	if (a->eq())            { a = getAtom(getEqAtom(a->id())); }
	ValueRep old = a->value();
	if (old == value_weak_true && v != value_weak_true) old = value_free;
	if (!a->assignValue(v)) { setConflict(); return false; }
	if (old == value_free)  { propQ_.push_back(a->id()); }
	return true;
}
bool LogicProgram::assignValue(PrgHead* h, ValueRep v) {
	return !h->isAtom() || assignValue(static_cast<PrgAtom*>(h), v);
}

bool LogicProgram::handleNatively(RuleType r, const BodyInfo& body) const {
	ExtendedRuleMode m = opts_.erMode;
	if (r == BASICRULE || r == OPTIMIZERULE || m == mode_native) {
		return true;
	}
	else if (m == mode_transform_integ || m == mode_transform_scc || m == mode_transform_nhcf) {
		return true;
	}
	else if (m == mode_transform) {
		return r == DISJUNCTIVERULE;
	}
	else if (m == mode_transform_dynamic) {
		return (r != CONSTRAINTRULE && r != WEIGHTRULE)
			|| transformNoAux(r, body) == false;
	}
	else if (m == mode_transform_choice) {
		return r != CHOICERULE;
	}
	else if (m == mode_transform_card)   {
		return r != CONSTRAINTRULE;
	}
	else if (m == mode_transform_weight) {
		return r != CONSTRAINTRULE && r != WEIGHTRULE;
	}
	assert(false && "unhandled extended rule mode");
	return true;
}

bool LogicProgram::transformNoAux(RuleType r, const BodyInfo& body) const {
	return r != CHOICERULE && (body.bound() == 1 || (body.size() <= 6 && choose(body.size(), body.bound()) <= 15));
}

void LogicProgram::transformExtended() {
	// remember starting position of aux atoms so
	// that we can remove them on next incremental step
	uint32 a  = numAtoms();
	RuleTransform tm;
	for (RuleList::size_type i = 0; i != extended_.size(); ++i) {
		incTr(extended_[i]->type(), tm.transform(*this, *extended_[i]));
		delete extended_[i];
	}
	extended_.clear();
	incTrAux(numAtoms() - a);
}

void LogicProgram::transformIntegrity(uint32 nAtoms, uint32 maxAux) {
	if (stats.rules(CONSTRAINTRULE).second == 0) { return; }
	// find all constraint rules that are integrity constraints
	BodyList integrity;
	for (uint32 i = 0, end = static_cast<uint32>(bodies_.size()); i != end; ++i) {
		PrgBody* b = bodies_[i];
		if (b->relevant() && b->type() == BodyInfo::COUNT_BODY && b->value() == value_false) {
			integrity.push_back(b);
		}
	}
	if (!integrity.empty() && (integrity.size() == 1 || (nAtoms/double(bodies_.size()) > 0.5 && integrity.size() / double(bodies_.size()) < 0.01))) {
		uint32 aux = static_cast<uint32>(atoms_.size());
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
			Rule* r;
			extended_.push_back(r = new Rule());
			r->setType(CONSTRAINTRULE);
			r->setBound(b->bound());
			r->addHead(getFalseId());
			for (uint32 g = 0; g != b->size(); ++g) {
				r->addToBody(b->goal(g).var(), !b->goal(g).sign());
			}
			setFrozen(false);
			transformExtended();
			setFrozen(true);
			// propagate integrity condition to new rules
			propQ_.push_back(getFalseId());
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
	}
}

// replace equivalent atoms in minimize rules
void LogicProgram::simplifyMinimize() {
	assert(hasMinimize());
	for (LogicProgram::MinimizeRule* r = minimize_; r; r = r->next_) {
		for (WeightLitVec::iterator it = r->lits_.begin(); it != r->lits_.end(); ++it) {
			it->first = Literal(getEqAtom(it->first.var()), it->first.sign());
		}
	}
}

void LogicProgram::updateFrozenAtoms() {
	if (!incData_) { return; }
	assert(incData_->frozen.empty());
	activeHead_.clear();
 	activeBody_.reset();
	PrgBody* support   = 0;
	VarVec::iterator j = incData_->update.begin();
	for (VarVec::iterator it = j, end = incData_->update.end(); it != end; ++it) {
		Var id     = getEqAtom(*it);
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
			if (!support) { support = getBodyFor(activeBody_); }
 			a->setIgnoreScc(true);
 			support->addHead(a, PrgEdge::CHOICE_EDGE);
			incData_->frozen.push_back(id); // still frozen
		}
	}
	incData_->update.erase(j, incData_->update.end());
}

void LogicProgram::prepareProgram(bool checkSccs) {
	assert(!frozen());
	startAux_ = (uint32)atoms_.size();
	transformExtended();
	stats.bodies= numBodies();
	if (opts_.normalize) { /* normalize(); */ assert(false);  }
	updateFrozenAtoms();
	setFrozen(true);
	Preprocessor p;
	if (hasConflict() || !propagate(true) || !p.preprocess(*this, opts_.iters != 0 && !opts_.suppMod ? Preprocessor::full_eq : Preprocessor::no_eq, opts_.iters, opts_.dfOrder)) {
		setConflict();
		return;
	}
	if (opts_.erMode == mode_transform_integ || opts_.erMode == mode_transform_dynamic) {
		uint32 nAtoms = startAux_ - startAtom();
		transformIntegrity(nAtoms, std::min(uint32(15000), nAtoms*2));
	}
	addMinimize();
	uint32 sccs = 0;
	if (checkSccs) {
		uint32 startScc = incData_ ? incData_->startScc : 0;
		SccChecker c(*this, sccAtoms_, startScc);
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
	stats.atoms = static_cast<uint32>(atoms_.size()) - startAtom();
	bodyIndex_.clear();
	disjIndex_.clear();
}

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
	uint32 shifted = 0, added = 0;
	stats.nonHcfs  = uint32(nonHcfs_.size());
	Literal bot    = negLit(0);
	for (uint32 id = 0, maxId = disj.size(); id != maxId; ++id) {
		PrgDisj* d = disj[id];
		Literal dx = d->inUpper() ? d->literal() : bot;
		PrgEdge e  = PrgEdge::newEdge(id, PrgEdge::CHOICE_EDGE, PrgEdge::DISJ_NODE);
		d->resetId(id, true); // id changed during scc checking
		// remove from program and 
		// replace with shifted rules or component-shifted disjunction
		head.clear(); supports.clear();
		for (PrgDisj::atom_iterator it = d->begin(), end = d->end(); it != end; ++it) {
			uint32  aId = it->node();
			PrgAtom* at = getAtom(aId);
			at->removeSupport(e);
			if (dx == bot) { continue; }
			if (at->eq())  { 
				at = getAtom(aId = getEqAtom(aId));
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
			b->removeHead(d, PrgEdge::NORMAL_EDGE);
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
				rule_.clear(); rule_.setType(DISJUNCTIVERULE);
				rule_.addHead(*hIt);
				if (supportLit.var() != 0) { rule_.addToBody(supportLit.var(), !supportLit.sign()); }
				else if (supportLit.sign()){ continue; }
				for (VarVec::iterator oIt = head.begin(); oIt != hEnd; ++oIt) {
					if (oIt != hIt) {
						if (getAtom(*oIt)->scc() == scc) { rule_.addHead(*oIt); }
						else                             { rule_.addToBody(*oIt, false); }
					}
				}
				RuleType t = simplifyRule(rule_, activeHead_, activeBody_);
				PrgBody* B = t != ENDRULE ? assignBodyFor(activeBody_, PrgEdge::NORMAL_EDGE, true) : 0;
				if (!B || B->value() == value_false) { continue; }
				if (t == BASICRULE) {
					++shifted;
					B->addHead(getAtom(activeHead_[0]), PrgEdge::NORMAL_EDGE);
				}
				else if (t == DISJUNCTIVERULE) {
					PrgDisj* x = getDisjFor(activeHead_, 0);
					B->addHead(x, PrgEdge::NORMAL_EDGE);
					x->assignVar(*this, *x->supps_begin());
					x->setInUpper(true);
					x->setSeen(true);
					++added;
					if ((sccMap[scc] & is_scc_non_hcf) == 0) {
						sccMap[scc] |= is_scc_non_hcf;
						nonHcfs_.add(scc);
					}
					if (!options().noGamma) {
						rule_.setType(BASICRULE);
						for (uint32 i = 1; i != rule_.heads.size(); ++i) { rule_.addToBody(rule_.heads[i], false); }
						rule_.heads.resize(1);
						WeightLitVec::iterator bIt = rule_.body.end();
						for (uint32 i = x->size();;) {
							t = simplifyRule(rule_, activeHead_, activeBody_);
							B = t != ENDRULE ? assignBodyFor(activeBody_, PrgEdge::GAMMA_EDGE, true) : 0;
							if (B && B->value() != value_false && !B->hasHead(getAtom(activeHead_[0]), PrgEdge::NORMAL_EDGE)) {
								B->addHead(getAtom(activeHead_[0]), PrgEdge::GAMMA_EDGE);
								++stats.gammas;
							}
							if (--i == 0) { break; }
							Var h          = rule_.heads[0];
							rule_.heads[0] = (--bIt)->first.var();
							*bIt           = WeightLiteral(negLit(h), 1);
						}
					}
					else {
						// only add support edge
						for (PrgDisj::atom_iterator a = x->begin(), end = x->end(); a != end; ++a) {
							B->addHead(getAtom(a->node()), PrgEdge::GAMMA_CHOICE_EDGE);
						}
					}
				}
			}
		}
	}
	stats.rules(DISJUNCTIVERULE).second = added;
	stats.rules(BASICRULE).second      += shifted;
	stats.nonHcfs  = uint32(nonHcfs_.size()) - stats.nonHcfs;
	setFrozen(true);
}

// optionally transform extended rules in sccs
void LogicProgram::prepareComponents() {
	int trRec  = opts_.erMode == mode_transform_scc;
	// HACK: force transformation of extended rules in non-hcf components
	// REMOVE this once minimality check supports aggregates
	if (!disjunctions_.empty() && trRec != 1) {
		trRec    = 2;
	}
	if (trRec != 0) {
		BodyList ext;
		EdgeVec  heads;
		for (BodyList::const_iterator it = bodies_.begin(), end = bodies_.end(); it != end; ++it) {
			if ((*it)->type() != BodyInfo::NORMAL_BODY && (*it)->hasVar() && (*it)->value() != value_false) {
				uint32 scc = (*it)->scc(*this);
				if (scc != PrgNode::noScc && (trRec == 1 || nonHcfs_.find(scc))) {
					ext.push_back(*it);
				}
			}
		}
		if (ext.empty()) { return; }
		struct Tr : public RuleTransform::ProgramAdapter {
			Tr(LogicProgram* x) : self(x), scc(0) {}
			Var newAtom() {
				Var x      = self->newAtom();
				PrgAtom* a = self->getAtom(x);
				self->sccAtoms_.push_back(a);
				a->setScc(scc);
				a->setSeen(true);
				atoms.push_back(x);
				return x;
			}
			void addRule(Rule& nr) {
				if (self->simplifyRule(nr, self->activeHead_, self->activeBody_) != ENDRULE) {
					PrgBody* B = self->assignBodyFor(self->activeBody_, PrgEdge::NORMAL_EDGE, false);
					if (B->value() != value_false) {
						B->addHead(self->getAtom(self->activeHead_[0]), PrgEdge::NORMAL_EDGE);
					}
				}
			}
			LogicProgram* self;
			uint32 scc;
			VarVec atoms;
		} tr(this);
		RuleTransform trans;
		setFrozen(false);
		for (BodyList::const_iterator it = ext.begin(), end = ext.end(); it != end; ++it) {
			uint32 scc = (*it)->scc(*this);
			rule_.clear();
			rule_.setType((*it)->type() == BodyInfo::COUNT_BODY ? CONSTRAINTRULE : WEIGHTRULE);
			rule_.setBound((*it)->bound());
			tr.scc = scc;
			for (uint32 i = 0; i != (*it)->size(); ++i) {
				rule_.addToBody((*it)->goal(i).var(), (*it)->goal(i).sign() == false, (*it)->weight(i));
			}
			heads.assign((*it)->heads_begin(), (*it)->heads_end());
			for (EdgeVec::const_iterator hIt = heads.begin(); hIt != heads.end(); ++hIt) {
				assert(hIt->isAtom());
				if (getAtom(hIt->node())->scc() == scc) {
					(*it)->removeHead(getAtom(hIt->node()), hIt->type());
					rule_.heads.assign(1, hIt->node());
					if (simplifyRule(rule_, activeHead_, activeBody_) != ENDRULE) {
						trans.transform(tr, rule_);	
					}
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
			else { assignValue(ax, value_false); }
		}
		setFrozen(true);
	}
}

// add (completion) nogoods
bool LogicProgram::addConstraints() {
	ClauseCreator gc(ctx()->master());
	if (options().iters == 0) {
		gc.addDefaultFlags(ClauseCreator::clause_force_simplify);
	}
	ctx()->startAddConstraints();
	ctx()->symbolTable().endInit();
	CLASP_ASSERT_CONTRACT(ctx()->symbolTable().curBegin() == ctx()->symbolTable().end() || startAtom() <= ctx()->symbolTable().curBegin()->first);
	// handle initial conflict, if any
	if (!ctx()->ok() || !ctx()->addUnary(getFalseAtom()->trueLit())) {
		return false;
	}
	if (options().noGamma && !disjunctions_.empty()) {
		// add "rule" nogoods for disjunctions
		for (DisjList::const_iterator it = disjunctions_.begin(); it != disjunctions_.end(); ++it) {
			gc.start().add(~(*it)->literal());
			for (PrgDisj::atom_iterator a = (*it)->begin(); a != (*it)->end(); ++a) {
				gc.add(getAtom(a->node())->literal());
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
	// add atoms from this step and finalize symbol table
	typedef SymbolTable::const_iterator symbol_iterator;
	const bool freezeAll   = incData_ && ctx()->satPrepro.get() != 0;
	const bool freezeShown = options().freezeShown;
	symbol_iterator sym    = ctx()->symbolTable().lower_bound(ctx()->symbolTable().curBegin(), startAtom());
	symbol_iterator symEnd = ctx()->symbolTable().end();
	Var             atomId = startAtom();
	for (AtomList::const_iterator it = atoms_.begin()+atomId, end = atoms_.end(); it != end; ++it, ++atomId) {
		if (!toConstraint(*it, *this, gc)) { return false; }
		if (freezeAll && (*it)->hasVar())  { ctx()->setFrozen((*it)->var(), true); }
		if (sym != symEnd && atomId == sym->first) {
			sym->second.lit = atoms_[getEqAtom(atomId)]->literal();
			if (freezeShown) { ctx()->setFrozen(sym->second.lit.var(), true); }
			++sym;
		}
	}
	if (!sccAtoms_.empty()) {
		if (ctx()->sccGraph.get() == 0) {
			ctx()->sccGraph = new SharedDependencyGraph(nonHcfCfg_);
		}
		uint32 oldNodes = ctx()->sccGraph->nodes();
		ctx()->sccGraph->addSccs(*this, sccAtoms_, nonHcfs_);
		stats.ufsNodes  = ctx()->sccGraph->nodes()-oldNodes;
		sccAtoms_.clear();
	}
	return true;
}
#undef check_modular
/////////////////////////////////////////////////////////////////////////////////////////
// misc/helper functions
/////////////////////////////////////////////////////////////////////////////////////////
PrgAtom* LogicProgram::resize(Var atomId) {
	while (atoms_.size() <= AtomList::size_type(atomId)) {
		newAtom();
	}
	return atoms_[getEqAtom(atomId)];
}

bool LogicProgram::propagate(bool backprop) {
	assert(frozen());
	bool oldB = opts_.backprop;
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

// Simplifies the rule's body:
//   - removes duplicate literals: {a,b,a} -> {a, b}.
//   - checks for contradictions : {a, not a}
//   - removes literals with weight 0    : LB [a = 0, b = 2, c = 0, ...] -> LB [b = 2, ...]
//   - reduces weights > bound() to bound:  2 [a=1, b=3] -> 2 [a=1, b=2]
//   - merges duplicate literals         : LB [a=w1, b=w2, a=w3] -> LB [a=w1+w3, b=w2]
//   - checks for contradiction, i.e.
//     rule body contains both p and not p and both are needed
//   - replaces weight constraint with cardinality constraint 
//     if all body weights are equal
//   - replaces weight/cardinality constraint with normal body 
//     if sumW - minW < bound()
RuleType LogicProgram::simplifyBody(const Rule& r, BodyInfo& info) {
	info.reset();
	WeightLitVec& sBody= info.lits;
	if (r.bodyHasBound() && r.bound() <= 0) {
		return BASICRULE;
	}
	sBody.reserve(r.body.size());
	RuleType resType = r.type();
	weight_t w       = 0;
	weight_t BOUND   = r.bodyHasBound() ? r.bound() : std::numeric_limits<weight_t>::max();
	weight_t bound   = r.bodyHasBound() ? r.bound() : static_cast<weight_t>(r.body.size());
	uint32   pos     = 0;
	uint32   hash    = 0;
	bool     dirty   = r.bodyHasWeights();
	Literal lit;
	for (WeightLitVec::const_iterator it = r.body.begin(), bEnd = r.body.end(); it != bEnd; ++it) {
		if (it->second == 0) continue; // skip irrelevant lits
		CLASP_ASSERT_CONTRACT_MSG(it->second>0, "Positive weight expected!");
		PrgAtom* a = resize(it->first.var());
		lit        = Literal(a->id(), it->first.sign());// replace any eq atoms
		w          = std::min(it->second, BOUND);       // reduce weights to bound
		if (a->value() != value_free || !a->relevant()) {
			bool vSign = a->value() == value_false || !a->relevant();
			if (vSign != lit.sign()) {
				// literal is false - drop rule?
				if (r.bodyIsSet()) { resType = ENDRULE; break; }
				continue;
			}
			else if (a->value() != value_weak_true && r.type() != OPTIMIZERULE) {
				// literal is true - drop from rule
				if ((bound -= w) <= 0) {
					while (!sBody.empty()) { ruleState_.clear(sBody.back().first.var()); sBody.pop_back(); }
					pos = hash = 0;
					break;
				}
				continue;
			}
		}
		if (!ruleState_.inBody(lit)) {  // literal not seen yet
			ruleState_.addToBody(lit);    // add to simplified body
			sBody.push_back(WeightLiteral(lit, w));
			pos += !lit.sign();
			hash+= hashLit(lit);
		}
		else if (!r.bodyIsSet()) {      // Merge duplicate lits
			WeightLiteral& oldLit = info[info.findLit(lit)];
			weight_t       oldW   = oldLit.second;
			CLASP_ASSERT_CONTRACT_MSG((INT_MAX-oldW)>= w, "Integer overflow!");
			w             = std::min(oldW + w, BOUND); // remember new weight
			oldLit.second = w;
			dirty         = true;
			if (resType == CONSTRAINTRULE) {
				resType = WEIGHTRULE;
			}
		}
		else {
			bound -= 1;
		}
		dirty |= ruleState_.inBody(~lit);
	}
	weight_t minW  = 1;
	weight_t maxW  = 1;
	wsum_t realSum = (wsum_t)sBody.size();
	wsum_t sumW    = (wsum_t)sBody.size();
	if (dirty) {
		minW    = std::numeric_limits<weight_t>::max();
		realSum = sumW = 0;
		for (WeightLitVec::size_type i = 0; i != sBody.size(); ++i) {
			lit = sBody[i].first; w = sBody[i].second;
			minW = std::min(minW, w);
			maxW = std::max(maxW, w);
			sumW+= w;
			if      (!ruleState_.inBody(~lit)) { realSum += w; }
			else if (r.bodyIsSet())            { resType = ENDRULE; break; }
			else if (lit.sign())               { 
				// body contains lit and ~lit: we can achieve at most max(weight(lit), weight(~lit))
				realSum += std::max(w, info[info.findLit(~lit)].second);
			}
		}
		if (resType == OPTIMIZERULE) { bound = 0; }
	}
	if (resType != ENDRULE && r.bodyHasBound()) {
		if      (bound <= 0)            { resType = BASICRULE; bound = 0; }
		else if (realSum < bound)       { resType = ENDRULE;   }
		else if ((sumW - minW) < bound) { resType = BASICRULE;      bound = (weight_t)sBody.size(); }
		else if (minW == maxW)          { resType = CONSTRAINTRULE; bound = (bound+(minW-1))/minW;  }
	}
	if (hasWeights(r.type()) && !hasWeights(resType) && maxW > 1 && resType != ENDRULE) {
		for (WeightLitVec::iterator it = sBody.begin(), end = sBody.end(); it != end; ++it) {
			it->second = 1;
		}
	}
	info.init(resType, bound, hash, pos);
	return resType;
}

RuleType LogicProgram::simplifyRule(const Rule& r, VarVec& head, BodyInfo& body) {
	RuleType type = simplifyBody(r, body);
	head.clear();
	if (type != ENDRULE && type != OPTIMIZERULE) {
		bool blocked = false, taut = false;
		weight_t sum = -1;
		for (VarVec::const_iterator it = r.heads.begin(), end = r.heads.end(); it != end; ++it) {
			if (!ruleState_.isSet(*it, RuleState::any_flag)) {
				head.push_back(*it);
				ruleState_.addToHead(*it);
			}
			else if (!ruleState_.isSet(*it, RuleState::head_flag)) {
				weight_t wPos = ruleState_.inBody(posLit(*it)) ? body.weight(posLit(*it)) : 0;
				weight_t wNeg = ruleState_.inBody(negLit(*it)) ? body.weight(negLit(*it)) : 0;
				if (sum == -1) sum = body.sum();
				if ((sum - wPos) < body.bound()) {
					taut    = (type != CHOICERULE);
				}
				else if ((sum - wNeg) < body.bound()) {
					blocked = (type != CHOICERULE);
				}
				else {
					head.push_back(*it);
					ruleState_.addToHead(*it);
				}
			}
		}
		for (VarVec::const_iterator it = head.begin(), end = head.end(); it != end; ++it) {
			ruleState_.clear(*it);
		}
		if (blocked && type != DISJUNCTIVERULE) {
			head.clear();
			head.push_back(0);
		}
		else if (taut && (type == DISJUNCTIVERULE || head.empty())) {
			head.clear();
			type = ENDRULE;
		}
		else if (type == DISJUNCTIVERULE && head.size() == 1) {
			type = BASICRULE;
		}
		else if (head.empty()) {
			type = ENDRULE;
		}
	}
	for (WeightLitVec::size_type i = 0; i != body.size(); ++i) {
		ruleState_.clear(body[i].first.var());
	}
	return type;
}

// create new atom aux representing supports, i.e.
// aux == S1 v ... v Sn
Literal LogicProgram::getEqAtomLit(Literal lit, const BodyList& supports, Preprocessor& p, const SccMap& sccMap) {
	if (supports.empty() || lit == negLit(0)) { return negLit(0); }
	if (supports.size() == 1 && supports[0]->size() < 2) { 
		return supports[0]->size() == 0 ? posLit(0) : supports[0]->goal(0); 
	}
	if (p.getRootAtom(lit) != varMax)         { return posLit(p.getRootAtom(lit)); }
	incTrAux(1);
	Var auxV     = newAtom();
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
			b->addHead(aux, PrgEdge::NORMAL_EDGE);
			if (b->value() != aux->value()) { assignValue(aux, b->value()); }
			aux->setInUpper(true);
		}
	}
	if (!aux->inUpper()) {
		aux->setValue(value_false);
		return negLit(0);
	}
	else if (scc != PrgNode::noScc) {
		aux->setScc(scc);
		sccAtoms_.push_back(aux);
	}
	return posLit(auxV);
}

PrgBody* LogicProgram::getBodyFor(BodyInfo& body, bool addDeps) {
	uint32 bodyId = equalBody(bodyIndex_.equal_range(body.hash), body);
	if (bodyId != varMax) {
		return getBody(bodyId);
	}
	// no corresponding body exists, create a new object
	bodyId        = (uint32)bodies_.size();
	PrgBody* b    = PrgBody::create(*this, bodyId, body, addDeps);
	bodyIndex_.insert(IndexMap::value_type(body.hash, bodyId));
	bodies_.push_back(b);
	if (b->isSupported()) {
		initialSupp_.push_back(bodyId);
	}
	return b;
}

PrgBody* LogicProgram::assignBodyFor(BodyInfo& body, EdgeType depEdge, bool simpStrong) {
	PrgBody* b = getBodyFor(body, depEdge != PrgEdge::GAMMA_EDGE);
	if (!b->hasVar() && !b->seen()) {
		uint32 eqId;
		b->markDirty();
		b->simplify(*this, simpStrong, &eqId);
		if (eqId != b->id()) {
			assert(b->id() == bodies_.size()-1);
			removeBody(b, body.hash);
			bodies_.pop_back();
			if (depEdge != PrgEdge::GAMMA_EDGE) {
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

uint32 LogicProgram::equalBody(const IndexRange& range, BodyInfo& body) const {
	bool sorted = false;
	for (IndexIter it = range.first; it != range.second; ++it) {
		PrgBody& o = *bodies_[it->second];
		if (o.type() == body.type() && o.size() == body.size() && o.bound() == body.bound() && (body.posSize() == 0u || o.goal(body.posSize()-1).sign() == false)) {
			// bodies are structurally equivalent - check if they contain the same literals
			if ((o.relevant() || (o.eq() && getBody(o.id())->relevant())) && o.eqLits(body.lits, sorted)) {
				assert(o.id() == it->second || o.eq());
				return o.id();
			}
		}
	}
	return varMax;
}
uint32 LogicProgram::findEqBody(PrgBody* b, uint32 hash) {
	LogicProgram::IndexRange eqRange = bodyIndex_.equal_range(hash);
	// check for existing body
	if (eqRange.first != eqRange.second) {
		activeBody_.reset();
		WeightLitVec& lits = activeBody_.lits;
		uint32 p = 0;
		for (uint32 i = 0, end = b->size(); i != end; ++i) { 
			lits.push_back(WeightLiteral(b->goal(i), b->weight(i))); 
			p += !lits.back().first.sign(); 
		}
		activeBody_.init(b->type(), b->bound(), hash, p);
		return equalBody(eqRange, activeBody_);
	}
	return varMax;
}

PrgDisj* LogicProgram::getDisjFor(const VarVec& heads, uint32 headHash) {
	PrgDisj* d = 0;
	if (headHash) {
		LogicProgram::IndexRange eqRange = disjIndex_.equal_range(headHash);
		for (; eqRange.first != eqRange.second; ++eqRange.first) {
			PrgDisj& o = *disjunctions_[eqRange.first->second];
			if (o.relevant() && o.size() == heads.size() && ruleState_.allMarked(o.begin(), o.end(), RuleState::head_flag)) {
				assert(o.id() == eqRange.first->second);
				d = &o;
				break;
			}
		}
		for (VarVec::const_iterator it = heads.begin(), end = heads.end(); it != end; ++it) {
			ruleState_.clear(*it);
		}
	}
	if (!d) {
		// no corresponding disjunction exists, create a new object
		// and link it to all atoms
		uint32 id    = disjunctions_.size();
		d            = PrgDisj::create(id, heads);
		disjunctions_.push_back(d);
		PrgEdge edge = PrgEdge::newEdge(id, PrgEdge::CHOICE_EDGE, PrgEdge::DISJ_NODE);
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

PrgAtom* LogicProgram::mergeEqAtoms(PrgAtom* a, Var rootId) {
	rootId        = getEqAtom(rootId);
	PrgAtom* root = getAtom(rootId);
	ValueRep mv   = getMergeValue(a, root);	
	assert(!a->eq() && !root->eq() && !a->frozen());
	if (a->ignoreScc()) { root->setIgnoreScc(true); }
	if (mv != a->value()    && !assignValue(a, mv))   { return 0; }
	if (mv != root->value() && !assignValue(root, mv)){ return 0; }
	a->setEq(rootId);
	incEqs(Var_t::atom_var);
	return root;
}

// returns whether posSize(root) <= posSize(body)
bool LogicProgram::positiveLoopSafe(PrgBody* body, PrgBody* root) const {
	uint32 i = 0, end = std::min(body->size(), root->size());
	while (i != end && body->goal(i).sign() == root->goal(i).sign()) { ++i; }
	return i == root->size() || root->goal(i).sign();	
}

PrgBody* LogicProgram::mergeEqBodies(PrgBody* b, Var rootId, bool hashEq, bool atomsAssigned) {
	rootId        = getEqNode(bodies_, rootId);
	PrgBody* root = getBody(rootId);
	bool     bp   = options().backprop;
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
		incEqs(Var_t::body_var);
		b->setEq(rootId);
		return root;
	}
	return b;
}

uint32 LogicProgram::findLpFalseAtom() const {
	for (VarVec::size_type i = 1; i < atoms_.size(); ++i) {
		if (!atoms_[i]->eq() && atoms_[i]->value() == value_false) {
			return i;
		}
	}
	return 0;
}

const char* LogicProgram::getAtomName(Var id) const {
	if (const SymbolTable::symbol_type* x = ctx()->symbolTable().find(id)) {
		return x->name.c_str();
	}
	return "";
}
VarVec& LogicProgram::getSupportedBodies(bool sorted) {
	if (sorted) {
		std::stable_sort(initialSupp_.begin(), initialSupp_.end(), LessBodySize(bodies_));
	}
	return initialSupp_;
}

bool LogicProgram::transform(const PrgBody& body, BodyInfo& out) const {
	out.reset();
	out.lits.reserve(body.size());
	uint32 p = 0, end = body.size();
	while (p != end && !body.goal(p).sign()) { ++p; }
	uint32 R[2][2] = { {p, end}, {0, p} };
	weight_t sw = 0, st = 0;
	for (uint32 range = 0; range != 2; ++range) {
		for (uint32 x = R[range][0]; x != R[range][1]; ++x) {
			WeightLiteral wl(body.goal(x), body.weight(x));
			if (getAtom(wl.first.var())->hasVar()) {
				sw += wl.second;
				out.lits.push_back(wl);
			}
			else if (wl.first.sign()) {
				st += wl.second;
			}
		}
	}
	out.init(body.type(), std::max(body.bound() - st, weight_t(0)), 0, p);
	return sw >= out.bound();
}

void LogicProgram::transform(const MinimizeRule& body, BodyInfo& out) const {
	out.reset();
	uint32 pos = 0;
	for (WeightLitVec::const_iterator it = body.lits_.begin(), end = body.lits_.end(); it != end; ++it) {
		if (it->first.sign() && getAtom(it->first.var())->hasVar()) {
			out.lits.push_back(*it);
		}
	}
	for (WeightLitVec::const_iterator it = body.lits_.begin(), end = body.lits_.end(); it != end; ++it) {
		if (!it->first.sign() && getAtom(it->first.var())->hasVar()) {
			out.lits.push_back(*it);
			++pos;
		}
	}
	out.init(BodyInfo::SUM_BODY, -1, 0, pos);
}

} } // end namespace Asp

