// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Benjamin Kaufmann
// Copyright (C) 2013  Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#include "clingo/clingocontrol.hh"
#include <gringo/input/programbuilder.hh>
#include "clasp/solver.h"
#include <program_opts/typed_value.h>
#include <program_opts/application.h>
#include <potassco/basic_types.h>
#include "clingo.hh"

// {{{1 definition of ClaspAPIBackend

void ClaspAPIBackend::initProgram(bool) { }

void ClaspAPIBackend::endStep() { }

void ClaspAPIBackend::beginStep() { }

void ClaspAPIBackend::rule(Potassco::Head_t ht, const Potassco::AtomSpan& head, const Potassco::LitSpan& body) {
    if (auto p = prg()) { p->addRule(ht, head, body); }
}

void ClaspAPIBackend::rule(Potassco::Head_t ht, const Potassco::AtomSpan& head, Potassco::Weight_t bound, const Potassco::WeightLitSpan& body) {
    if (auto p = prg()) { p->addRule(ht, head, bound, body); }
}

void ClaspAPIBackend::minimize(Potassco::Weight_t prio, const Potassco::WeightLitSpan& lits) {
    if (auto p = prg()) { p->addMinimize(prio, lits); }
}

void ClaspAPIBackend::project(const Potassco::AtomSpan& atoms) {
    if (auto p = prg()) { p->addProject(atoms); }
}

void ClaspAPIBackend::output(Gringo::Symbol sym, Potassco::Atom_t atom) {
    std::ostringstream out;
    out << sym;
    if (atom != 0) {
        Potassco::Lit_t lit = atom;
        if (auto p = prg()) { p->addOutput(Potassco::toSpan(out.str().c_str()), Potassco::LitSpan{&lit, 1}); }
    }
    else {
        if (auto p = prg()) { p->addOutput(Potassco::toSpan(out.str().c_str()), Potassco::LitSpan{nullptr, 0}); }
    }
}

void ClaspAPIBackend::output(Gringo::Symbol sym, Potassco::LitSpan const& condition) {
    std::ostringstream out;
    out << sym;
    if (auto p = prg()) { p->addOutput(Potassco::toSpan(out.str().c_str()), condition); }
}

void ClaspAPIBackend::output(Gringo::Symbol sym, int value, Potassco::LitSpan const& condition) {
    std::ostringstream out;
    out << sym << "=" << value;
    if (auto p = prg()) { p->addOutput(Potassco::toSpan(out.str().c_str()), condition); }
}

void ClaspAPIBackend::acycEdge(int s, int t, const Potassco::LitSpan& condition) {
    if (auto p = prg()) { p->addAcycEdge(s, t, condition); }
}

void ClaspAPIBackend::heuristic(Potassco::Atom_t a, Potassco::Heuristic_t t, int bias, unsigned prio, const Potassco::LitSpan& condition) {
    if (auto p = prg()) { p->addDomHeuristic(a, t, bias, prio, condition); }
}

void ClaspAPIBackend::assume(const Potassco::LitSpan& lits) {
    if (auto p = prg()) { p->addAssumption(lits); }
}

void ClaspAPIBackend::external(Potassco::Atom_t a, Potassco::Value_t v) {
    if (auto p = prg()) {
        switch (v) {
            case Potassco::Value_t::False:   { p->freeze(a, Clasp::value_false); break; }
            case Potassco::Value_t::True:    { p->freeze(a, Clasp::value_true); break; }
            case Potassco::Value_t::Free:    { p->freeze(a, Clasp::value_free); break; }
            case Potassco::Value_t::Release: { p->unfreeze(a); break; }
        }
    }
}

void ClaspAPIBackend::theoryTerm(Potassco::Id_t, int) { }

void ClaspAPIBackend::theoryTerm(Potassco::Id_t, const Potassco::StringSpan&) { }

void ClaspAPIBackend::theoryTerm(Potassco::Id_t, int, const Potassco::IdSpan&) { }

void ClaspAPIBackend::theoryElement(Potassco::Id_t e, const Potassco::IdSpan&, const Potassco::LitSpan& cond) {
    if (auto p = prg()) {
        Potassco::TheoryElement const &elem = p->theoryData().getElement(e);
        if (elem.condition() == Potassco::TheoryData::COND_DEFERRED) { p->theoryData().setCondition(e, p->newCondition(cond)); }
    }
}

void ClaspAPIBackend::theoryAtom(Potassco::Id_t, Potassco::Id_t, const Potassco::IdSpan&) { }

void ClaspAPIBackend::theoryAtom(Potassco::Id_t, Potassco::Id_t, const Potassco::IdSpan&, Potassco::Id_t, Potassco::Id_t){ }

Clasp::Asp::LogicProgram *ClaspAPIBackend::prg() {
    return ctl_.update() ? static_cast<Clasp::Asp::LogicProgram*>(ctl_.clasp_->program()) : nullptr;
}

ClaspAPIBackend::~ClaspAPIBackend() noexcept = default;

// {{{1 definition of ClingoControl

#define LOG if (verbose_) std::cerr
ClingoControl::ClingoControl(Gringo::Scripts &scripts, bool clingoMode, Clasp::ClaspFacade *clasp, Clasp::Cli::ClaspCliConfig &claspConfig, PostGroundFunc pgf, PreSolveFunc psf, Gringo::Logger::Printer printer, unsigned messageLimit)
: scripts_(scripts)
, clasp_(clasp)
, claspConfig_(claspConfig)
, pgf_(pgf)
, psf_(psf)
, logger_(printer, messageLimit)
, clingoMode_(clingoMode) { }

void ClingoControl::parse() {
    if (!parser_->empty()) {
        parser_->parse(logger_);
        defs_.init(logger_);
        parsed = true;
    }
    if (logger_.hasError()) {
        throw std::runtime_error("parsing failed");
    }
}

Potassco::Lit_t ClingoPropagateInit::mapLit(Lit_t lit) {
    const auto& prg = static_cast<Clasp::Asp::LogicProgram&>(*static_cast<ClingoControl&>(c_).clasp_->program());
    return Clasp::encodeLit(prg.getLiteral(lit));
}

int ClingoPropagateInit::threads() {
    return static_cast<ClingoControl&>(c_).clasp_->ctx.concurrency();
}

void ClingoControl::parse(const StringSeq& files, const ClingoOptions& opts, Clasp::Asp::LogicProgram* claspOut, bool addStdIn) {
    using namespace Gringo;
    logger_.enable(clingo_warning_operation_undefined, !opts.wNoOperationUndefined);
    logger_.enable(clingo_warning_atom_undefined, !opts.wNoAtomUndef);
    logger_.enable(clingo_warning_variable_unbounded, !opts.wNoVariableUnbounded);
    logger_.enable(clingo_warning_file_included, !opts.wNoFileIncluded);
    logger_.enable(clingo_warning_global_variable, !opts.wNoGlobalVariable);
    logger_.enable(clingo_warning_other, !opts.wNoOther);
    verbose_ = opts.verbose;
    Output::OutputPredicates outPreds;
    for (auto &x : opts.foobar) {
        outPreds.emplace_back(Location("<cmd>",1,1,"<cmd>", 1,1), x, false);
    }
    if (claspOut) {
        out_ = gringo_make_unique<Output::OutputBase>(claspOut->theoryData(), std::move(outPreds), gringo_make_unique<ClaspAPIBackend>(*this), opts.outputOptions);
    }
    else {
        data_ = gringo_make_unique<Potassco::TheoryData>();
        out_ = gringo_make_unique<Output::OutputBase>(*data_, std::move(outPreds), std::cout, opts.outputFormat, opts.outputOptions);
    }
    out_->keepFacts = opts.keepFacts;
    pb_ = gringo_make_unique<Input::NongroundProgramBuilder>(scripts_, prg_, *out_, defs_, opts.rewriteMinimize);
    parser_ = gringo_make_unique<Input::NonGroundParser>(*pb_);
    for (auto &x : opts.defines) {
        LOG << "define: " << x << std::endl;
        parser_->parseDefine(x, logger_);
    }
    for (auto x : files) {
        LOG << "file: " << x << std::endl;
        parser_->pushFile(std::move(x), logger_);
    }
    if (files.empty() && addStdIn) {
        LOG << "reading from stdin" << std::endl;
        parser_->pushFile("-", logger_);
    }
    parse();
}

bool ClingoControl::update() {
    if (clingoMode_) {
        clasp_->update(configUpdate_);
        configUpdate_ = false;
        if (!clasp_->ok()) { return false; }
    }
    if (!grounded) {
        if (!initialized_) {
            out_->init(incremental_);
            initialized_ = true;
        }
        out_->beginStep();
        grounded = true;
    }
    return true;
}

void ClingoControl::ground(Gringo::Control::GroundVec const &parts, Gringo::Context *context) {
    if (!update()) { return; }
    if (parsed) {
        LOG << "************** parsed program **************" << std::endl << prg_;
        prg_.rewrite(defs_, logger_);
        LOG << "************* rewritten program ************" << std::endl << prg_;
        prg_.check(logger_);
        if (logger_.hasError()) {
            throw std::runtime_error("grounding stopped because of errors");
        }
        parsed = false;
    }
    if (!parts.empty()) {
        Gringo::Ground::Parameters params;
        for (auto &x : parts) { params.add(x.first, Gringo::SymVec(x.second)); }
        auto gPrg = prg_.toGround(out_->data, logger_);
        LOG << "*********** intermediate program ***********" << std::endl << gPrg << std::endl;
        LOG << "************* grounded program *************" << std::endl;
        auto exit = Gringo::onExit([this]{ scripts_.context = nullptr; });
        scripts_.context = context;
        gPrg.ground(params, scripts_, *out_, false, logger_);
    }
}

void ClingoControl::main() {
    if (scripts_.callable("main")) {
        incremental_ = true;
        clasp_->enableProgramUpdates();
        scripts_.main(*this);
    }
    else {
        incremental_ = false;
        claspConfig_.releaseOptions();
        Gringo::Control::GroundVec parts;
        parts.emplace_back("base", Gringo::SymVec{});
        ground(parts, nullptr);
        solve(nullptr, {});
    }
}
bool ClingoControl::onModel(Clasp::Model const &m) {
    if (!modelHandler_) { return true; }
    std::lock_guard<decltype(propLock_)> lock(propLock_);
    return modelHandler_(ClingoModel(*this, &m));
}
void ClingoControl::onFinish(Clasp::ClaspFacade::Result ret) {
    if (finishHandler_) {
        finishHandler_(convert(ret));
        finishHandler_ = nullptr;
    }
    modelHandler_ = nullptr;
}
Gringo::Symbol ClingoControl::getConst(std::string const &name) {
    auto ret = defs_.defs().find(name.c_str());
    if (ret != defs_.defs().end()) {
        bool undefined = false;
        Gringo::Symbol val = std::get<2>(ret->second)->eval(undefined, logger_);
        if (!undefined) { return val; }
    }
    return Gringo::Symbol();
}
void ClingoControl::add(std::string const &name, Gringo::FWStringVec const &params, std::string const &part) {
    Gringo::Location loc("<block>", 1, 1, "<block>", 1, 1);
    Gringo::Input::IdVec idVec;
    for (auto &x : params) { idVec.emplace_back(loc, x); }
    parser_->pushBlock(name, std::move(idVec), part, logger_);
    parse();
}
void ClingoControl::load(std::string const &filename) {
    parser_->pushFile(std::string(filename), logger_);
    parse();
}
bool ClingoControl::hasSubKey(unsigned key, char const *name, unsigned* subKey) {
    *subKey = claspConfig_.getKey(key, name);
    return *subKey != Clasp::Cli::ClaspCliConfig::KEY_INVALID;
}
unsigned ClingoControl::getSubKey(unsigned key, char const *name) {
    unsigned ret = claspConfig_.getKey(key, name);
    if (ret == Clasp::Cli::ClaspCliConfig::KEY_INVALID) {
        throw std::runtime_error("invalid key");
    }
    return ret;
}
unsigned ClingoControl::getArrKey(unsigned key, unsigned idx) {
    unsigned ret = claspConfig_.getArrKey(key, idx);
    if (ret == Clasp::Cli::ClaspCliConfig::KEY_INVALID) {
        throw std::runtime_error("invalid key");
    }
    return ret;
}
void ClingoControl::getKeyInfo(unsigned key, int* nSubkeys, int* arrLen, const char** help, int* nValues) const {
    if (claspConfig_.getKeyInfo(key, nSubkeys, arrLen, help, nValues) < 0) {
        throw std::runtime_error("could not get key info");
    }
}
const char* ClingoControl::getSubKeyName(unsigned key, unsigned idx) const {
    char const *ret = claspConfig_.getSubkey(key, idx);
    if (!ret) {
        throw std::runtime_error("could not get subkey");
    }
    return ret;
}
bool ClingoControl::getKeyValue(unsigned key, std::string &value) {
    int ret = claspConfig_.getValue(key, value);
    if (ret < -1) {
        throw std::runtime_error("could not get option value");
    }
    return ret >= 0;
}
void ClingoControl::setKeyValue(unsigned key, const char *val) {
    configUpdate_ = true;
    if (claspConfig_.setValue(key, val) <= 0) {
        throw std::runtime_error("could not set option value");
    }
}
unsigned ClingoControl::getRootKey() {
    return Clasp::Cli::ClaspCliConfig::KEY_ROOT;
}
Gringo::ConfigProxy &ClingoControl::getConf() {
    return *this;
}
Gringo::SolveIter *ClingoControl::solveIter(Assumptions &&ass) {
    prepare(nullptr, nullptr);
    auto a = toClaspAssumptions(std::move(ass));
    solveIter_ = Gringo::gringo_make_unique<ClingoSolveIter>(*this, a);
    return solveIter_.get();
}
Gringo::SolveFuture *ClingoControl::solveAsync(ModelHandler mh, FinishHandler fh, Assumptions &&ass) {
    if (!clingoMode_) { throw std::runtime_error("solveAsync is not supported in gringo gringo mode"); }
#if WITH_THREADS
    prepare(mh, fh);
    auto a = toClaspAssumptions(std::move(ass));
    solveFuture_ = Gringo::gringo_make_unique<ClingoSolveFuture>(clasp_->solveAsync(nullptr, a));
    return solveFuture_.get();
#else
    (void)mh;
    (void)fh;
    (void)ass;
    throw std::runtime_error("solveAsync requires clingo to be build with thread support");
#endif
}
void ClingoControl::interrupt() {
    clasp_->interrupt(30);
}
bool ClingoControl::blocked() {
    return clasp_->solving();
}
void ClingoControl::prepare(Gringo::Control::ModelHandler mh, Gringo::Control::FinishHandler fh) {
    if (update()) { out_->endStep(true, logger_); }
    grounded = false;
    if (clingoMode_) {
#if WITH_THREADS
        solveFuture_   = nullptr;
#endif
        solveIter_     = nullptr;
        finishHandler_ = fh;
        modelHandler_  = mh;
        Clasp::ProgramBuilder *prg = clasp_->program();
        postGround(*prg);
        if (!propagators_.empty()) {
            clasp_->program()->endProgram();
            for (auto&& pp : propagators_) {
                ClingoPropagateInit i(*this, *pp);
                static_cast<Gringo::Propagator*>(pp->propagator())->init(i);
            }
            propLock_.init(clasp_->ctx.concurrency());
        }
        prePrepare(*clasp_);
        clasp_->prepare(enableEnumAssupmption_ ? Clasp::ClaspFacade::enum_volatile : Clasp::ClaspFacade::enum_static);
        preSolve(*clasp_);
    }
    if (data_) { data_->reset(); }
    out_->reset();
}

Clasp::LitVec ClingoControl::toClaspAssumptions(Gringo::Control::Assumptions &&ass) const {
    Clasp::LitVec outAss;
    if (!clingoMode_ || !clasp_->program()) { return outAss; }
    const Clasp::Asp::LogicProgram* prg = static_cast<const Clasp::Asp::LogicProgram*>(clasp_->program());
    for (auto &x : ass) {
        auto atm = out_->find(x.first);
        if (atm.second && atm.first->hasUid()) {
            Clasp::Literal lit = prg->getLiteral(atm.first->uid());
            outAss.push_back(x.second ? lit : ~lit);
        }
        else if (x.second) {
            outAss.push_back(Clasp::lit_false());
            break;
        }
    }
    return outAss;
}

Gringo::SolveResult ClingoControl::solve(ModelHandler h, Assumptions &&ass) {
    prepare(h, nullptr);
    auto ret = clingoMode_ ? convert(clasp_->solve(nullptr, toClaspAssumptions(std::move(ass)))) : Gringo::SolveResult(Gringo::SolveResult::Unknown, false, false);
    postSolve(*clasp_);
    return ret;
}

void *ClingoControl::claspFacade() {
    return clasp_;
}

void ClingoControl::registerPropagator(std::unique_ptr<Gringo::Propagator> p, bool sequential) {
    propagators_.emplace_back(Gringo::gringo_make_unique<Clasp::ClingoPropagatorInit>(*p, propLock_.add(sequential)));
    claspConfig_.addConfigurator(propagators_.back().get(), Clasp::Ownership_t::Retain);
    props_.emplace_back(std::move(p));
}

void ClingoControl::cleanupDomains() {
    out_->endStep(false, logger_);
    if (clingoMode_) {
        Clasp::Asp::LogicProgram &prg = static_cast<Clasp::Asp::LogicProgram&>(*clasp_->program());
        prg.endProgram();
        Clasp::Solver &solver = *clasp_->ctx.master();
        auto assignment = [&prg, &solver](unsigned uid) {
            Clasp::Literal lit = prg.getLiteral(uid);
            Potassco::Value_t               truth = Potassco::Value_t::Free;
            if (solver.isTrue(lit))       { truth = Potassco::Value_t::True; }
            else if (solver.isFalse(lit)) { truth = Potassco::Value_t::False; }
            return std::make_pair(prg.isExternal(uid), truth);
        };
        auto stats = out_->simplify(assignment);
        LOG << stats.first << " atom" << (stats.first == 1 ? "" : "s") << " became facts" << std::endl;
        LOG << stats.second << " atom" << (stats.second == 1 ? "" : "s") << " deleted" << std::endl;
    }
}

std::string ClingoControl::str() {
    return "[object:IncrementalControl]";
}

void ClingoControl::assignExternal(Gringo::Symbol ext, Potassco::Value_t val) {
    if (update()) {
        auto atm = out_->find(ext);
        if (atm.second && atm.first->hasUid()) {
            Gringo::Id_t offset = atm.first - atm.second->begin();
            Gringo::Output::External external(Gringo::Output::LiteralId{Gringo::NAF::POS, Gringo::Output::AtomType::Predicate, offset, atm.second->domainOffset()}, val);
            out_->output(external);
        }
    }
}

Potassco::AbstractStatistics *ClingoControl::statistics() {
    return clasp_->getStats();
}

void ClingoControl::useEnumAssumption(bool enable) {
    enableEnumAssupmption_ = enable;
}
bool ClingoControl::useEnumAssumption() {
    return enableEnumAssupmption_;
}

Gringo::SymbolicAtoms &ClingoControl::getDomain() {
    if (clingoMode_) { return *this; }
    else {
        throw std::runtime_error("domain introspection only supported in clingo mode");
    }
}

namespace {

union SymbolicAtomOffset {
    SymbolicAtomOffset(clingo_symbolic_atom_iterator_t repr)
    : repr(repr) { }
    SymbolicAtomOffset(uint32_t domain_offset, bool domain_advance, uint32_t atom_offset, bool atom_advance)
    : data{domain_offset, domain_advance, atom_offset, atom_advance} { }
    clingo_symbolic_atom_iterator_t repr;
    struct {
        clingo_symbolic_atom_iterator_t domain_offset : 31;
        clingo_symbolic_atom_iterator_t domain_advance : 1;
        clingo_symbolic_atom_iterator_t atom_offset : 31;
        clingo_symbolic_atom_iterator_t atom_advance : 1;
    } data;
};

bool operator==(SymbolicAtomOffset const &a, SymbolicAtomOffset const &b) {
	return a.data.domain_offset == b.data.domain_offset && a.data.atom_offset == b.data.atom_offset;
}

SymbolicAtomOffset &toOffset(clingo_symbolic_atom_iterator_t &it) {
    return reinterpret_cast<SymbolicAtomOffset &>(it);
}

bool skipDomain(Gringo::Sig sig)                                          { return sig.name().startsWith("#"); }

Gringo::SymbolicAtomIter init(Gringo::Output::OutputBase &out, uint32_t domainOffset, bool advance) {
    SymbolicAtomOffset it(domainOffset, advance, 0, false);
    for (auto domIt = out.predDoms().begin() + domainOffset, domIe = out.predDoms().end(); domIt != domIe; ++domIt, ++it.data.domain_offset) {
        auto &dom = **domIt;
        if (!skipDomain(dom) && dom.size() > 0) { return it.repr; }
        if (!it.data.domain_advance) { break; }
    }
    it.data.domain_offset = out.predDoms().size();
    return it.repr;
}

void advance(Gringo::Output::OutputBase &out, Gringo::SymbolicAtomIter &it) {
    auto &off = toOffset(it).data;
    auto domIt  = out.predDoms().begin() + off.domain_offset;
    auto domIe  = out.predDoms().end();
    auto elemIt = (*domIt)->begin() + off.atom_offset;
    auto elemIe = (*domIt)->end();
    ++elemIt; ++off.atom_offset;
    while (elemIt == elemIe) {
        off.atom_offset = 0;
        if (!off.domain_advance) {
            off.domain_offset = out.predDoms().size();
            return;
        }
        ++domIt; ++off.domain_offset;
        if (domIt == domIe) { return; }
        if (!skipDomain(**domIt)) {
            elemIt = (*domIt)->begin();
            elemIe = (*domIt)->end();
        }
    }
}

Gringo::Output::PredicateAtom &domainElem(Gringo::Output::PredDomMap &map, Gringo::SymbolicAtomIter it) {
    auto &off = toOffset(it).data;
    return (*map[off.domain_offset])[off.atom_offset];
}

} // namespace

Gringo::Symbol ClingoControl::atom(Gringo::SymbolicAtomIter it) const {
    return domainElem(out_->predDoms(), it);
}

Potassco::Lit_t ClingoControl::literal(Gringo::SymbolicAtomIter it) const {
    auto &elem = domainElem(out_->predDoms(), it);
    return elem.hasUid() ? elem.uid() : 0;
}

bool ClingoControl::fact(Gringo::SymbolicAtomIter it) const {
    return domainElem(out_->predDoms(), it).fact();
}

bool ClingoControl::external(Gringo::SymbolicAtomIter it) const {
    auto &elem = domainElem(out_->predDoms(), it);
    return elem.hasUid() && elem.isExternal() && static_cast<Clasp::Asp::LogicProgram*>(clasp_->program())->isExternal(elem.uid());
}

Gringo::SymbolicAtomIter ClingoControl::next(Gringo::SymbolicAtomIter it) {
    advance(*out_, it);
    return it;
}

bool ClingoControl::valid(Gringo::SymbolicAtomIter it) const {
    auto &off = toOffset(it).data;
    return off.domain_offset < out_->predDoms().size() && off.atom_offset < out_->predDoms()[off.domain_offset]->size();
}

std::vector<Gringo::Sig> ClingoControl::signatures() const {
    std::vector<Gringo::Sig> ret;
    for (auto &dom : out_->predDoms()) {
        if (!skipDomain(*dom)) { ret.emplace_back(*dom); }
    }
    return ret;
}

Gringo::SymbolicAtomIter ClingoControl::begin(Gringo::Sig sig) const {
    return init(*out_, out_->predDoms().offset(out_->predDoms().find(sig)), false);
}

Gringo::SymbolicAtomIter ClingoControl::begin() const {
    return init(*out_, 0, true);
}

Gringo::SymbolicAtomIter ClingoControl::end() const {
    return SymbolicAtomOffset{out_->predDoms().size(), false, 0, false}.repr;
}

bool ClingoControl::eq(Gringo::SymbolicAtomIter it, Gringo::SymbolicAtomIter jt) const {
    return toOffset(it) == toOffset(jt);
}

Gringo::SymbolicAtomIter ClingoControl::lookup(Gringo::Symbol atom) const {
    if (atom.hasSig()) {
        auto it = out_->predDoms().find(atom.sig());
        if (it != out_->predDoms().end()) {
            auto jt = (*it)->find(atom);
            if (jt != (*it)->end()) {
                return SymbolicAtomOffset(out_->predDoms().offset(it), true, jt - (*it)->begin(), true).repr;
            }
        }
    }
    return SymbolicAtomOffset(out_->predDoms().size(), true, 0, true).repr;
}

size_t ClingoControl::length() const {
    size_t ret = 0;
    for (auto &dom : out_->predDoms()) {
        if (!skipDomain(*dom)) {
            ret += dom->size();
        }
    }
    return ret;
}

Gringo::Backend *ClingoControl::backend() { return out_->backend(); }
Potassco::Atom_t ClingoControl::addProgramAtom() { return out_->data.newAtom(); }

ClingoControl::~ClingoControl() noexcept = default;

// {{{1 definition of ClingoSolveIter

ClingoSolveIter::ClingoSolveIter(ClingoControl &ctl, Clasp::LitVec const &ass)
: future_(ctl.clasp_->startSolve(ass))
, model_(ctl) { }

Gringo::Model const *ClingoSolveIter::next() {
    if (future_.next()) {
        model_.reset(future_.model());
        return &model_;
    }
    else { return nullptr; }
}

void ClingoSolveIter::close() {
    future_.stop();
}

Gringo::SolveResult ClingoSolveIter::get() {
    return convert(future_.result());
}

// {{{1 definition of ClingoSolveFuture

Gringo::SolveResult convert(Clasp::ClaspFacade::Result res) {
    if (res.interrupted() && res.signal == SIGINT) {
        throw std::runtime_error("solving stopped by signal");
    }
    Gringo::SolveResult::Satisfiabily sat = Gringo::SolveResult::Satisfiable;
    switch (res) {
        case Clasp::ClaspFacade::Result::SAT:     { sat = Gringo::SolveResult::Satisfiable; break; }
        case Clasp::ClaspFacade::Result::UNSAT:   { sat = Gringo::SolveResult::Unsatisfiable; break; }
        case Clasp::ClaspFacade::Result::UNKNOWN: { sat = Gringo::SolveResult::Unknown; break; }
    }
    return {sat, res.exhausted(), res.interrupted()};
}

#if WITH_THREADS
ClingoSolveFuture::ClingoSolveFuture(Clasp::ClaspFacade::AsyncResult const &res)
    : future(res) { }
Gringo::SolveResult ClingoSolveFuture::get() {
    if (!done) {
        done      = true;
        ret       = convert(future.get());
    }
    return ret;
}
void ClingoSolveFuture::wait() { get(); }
bool ClingoSolveFuture::wait(double timeout) {
    if (!done) {
        if (timeout == 0 ? !future.ready() : !future.waitFor(timeout)) { return false; }
        wait();
    }
    return true;
}
void ClingoSolveFuture::cancel() { future.cancel(); }
#endif

// {{{1 definition of ClingoLib

ClingoLib::ClingoLib(Gringo::Scripts &scripts, int argc, char const * const *argv, Gringo::Logger::Printer printer, unsigned messageLimit)
        : ClingoControl(scripts, true, &clasp_, claspConfig_, nullptr, nullptr, printer, messageLimit) {
    using namespace ProgramOptions;
    OptionContext allOpts("<libclingo>");
    initOptions(allOpts);
    ParsedValues values = parseCommandArray(argv, argc, allOpts, false, parsePositional);
    ParsedOptions parsed;
    parsed.assign(values);
    allOpts.assignDefaults(parsed);
    claspConfig_.finalize(parsed, Clasp::Problem_t::Asp, true);
    clasp_.ctx.setEventHandler(this);
    Clasp::Asp::LogicProgram* lp = &clasp_.startAsp(claspConfig_, true);
    parse({}, grOpts_, lp, false);
}


static bool parseConst(const std::string& str, std::vector<std::string>& out) {
    out.push_back(str);
    return true;
}

void ClingoLib::initOptions(ProgramOptions::OptionContext& root) {
    using namespace ProgramOptions;
    grOpts_.defines.clear();
    grOpts_.verbose = false;
    OptionGroup gringo("Gringo Options");
    gringo.addOptions()
        ("verbose,V"                , flag(grOpts_.verbose = false), "Enable verbose output")
        ("const,c"                  , storeTo(grOpts_.defines, parseConst)->composing()->arg("<id>=<term>"), "Replace term occurences of <id> with <term>")
        ("output-debug", storeTo(grOpts_.outputOptions.debug = Gringo::Output::OutputDebug::NONE, values<Gringo::Output::OutputDebug>()
          ("none", Gringo::Output::OutputDebug::NONE)
          ("text", Gringo::Output::OutputDebug::TEXT)
          ("translate", Gringo::Output::OutputDebug::TRANSLATE)
          ("all", Gringo::Output::OutputDebug::ALL)), "Print debug information during output:\n"
         "      none     : no additional info\n"
         "      text     : print rules as plain text (prefix %%)\n"
         "      translate: print translated rules as plain text (prefix %%%%)\n"
         "      all      : combines text and translate")
        ("warn,W"                   , storeTo(grOpts_, parseWarning)->arg("<warn>")->composing(), "Enable/disable warnings:\n"
         "      none:                     disable all warnings\n"
         "      all:                      enable all warnings\n"
         "      [no-]atom-undefined:      a :- b.\n"
         "      [no-]file-included:       #include \"a.lp\". #include \"a.lp\".\n"
         "      [no-]operation-undefined: p(1/0).\n"
         "      [no-]variable-unbounded:  $x > 10.\n"
         "      [no-]global-variable:     :- #count { X } = 1, X = 1.\n"
         "      [no-]other:               clasp related and uncategorized warnings")
        ("rewrite-minimize"         , flag(grOpts_.rewriteMinimize = false), "Rewrite minimize constraints into rules")
        ("keep-facts"               , flag(grOpts_.keepFacts = false), "Do not remove facts from normal rules")
        ;
    root.add(gringo);
    claspConfig_.addOptions(root);
}

bool ClingoLib::onModel(Clasp::Solver const&, Clasp::Model const& m) {
    return ClingoControl::onModel(m);
}
void ClingoLib::onEvent(Clasp::Event const& ev) {
#if WITH_THREADS
    Clasp::ClaspFacade::StepReady const *r = Clasp::event_cast<Clasp::ClaspFacade::StepReady>(ev);
    if (r && finishHandler_) { onFinish(r->summary->result); }
#endif
    const Clasp::LogEvent* log = Clasp::event_cast<Clasp::LogEvent>(ev);
    if (log && log->isWarning()) { logger_.print(clingo_warning_other, log->msg); }
}
bool ClingoLib::parsePositional(const std::string& t, std::string& out) {
    int num;
    if (bk_lib::string_cast(t, num)) {
        out = "number";
        return true;
    }
    return false;
}
ClingoLib::~ClingoLib() {
    // TODO: can be removed after bennies next update...
#if WITH_THREADS
    solveFuture_ = nullptr;
#endif
    solveIter_   = nullptr;
    clasp_.shutdown();
}

// {{{1 definition of DefaultGringoModule

DefaultGringoModule::DefaultGringoModule()
: scripts(*this) { }

Gringo::Control *DefaultGringoModule::newControl(int argc, char const * const*argv, Gringo::Logger::Printer printer, unsigned messageLimit) {
    return new ClingoLib(scripts, argc, argv, printer, messageLimit);
}

Gringo::Symbol DefaultGringoModule::parseValue(std::string const &str, Gringo::Logger::Printer printer, unsigned messageLimit) {
    Gringo::Logger logger(printer, messageLimit);
    return parser.parse(str, logger);
}

extern "C" bool clingo_control_new(char const *const * args, size_t n, clingo_logger_t *logger, void *data, unsigned message_limit, clingo_control_t **ctl) {
    GRINGO_CLINGO_TRY {
        static std::mutex mut;
        static DefaultGringoModule mod;
        std::lock_guard<std::mutex> grd(mut);
        *ctl = mod.newControl(n, args, logger ? [logger, data](clingo_warning_t code, char const *msg) { logger(code, msg, data); } : Gringo::Logger::Printer(nullptr), message_limit);
    }
    GRINGO_CLINGO_CATCH;
}

struct Clingo::Control::Impl {
    Impl(Logger logger)
    : ctl(nullptr)
    , logger(logger) { }
    Impl(clingo_control_t *ctl)
    : ctl(ctl) { }
    ~Impl() noexcept {
        if (ctl) { clingo_control_free(ctl); }
    }
    operator clingo_control_t *() { return ctl; }
    clingo_control_t *ctl;
    Logger logger;
    ModelCallback mh;
    FinishCallback fh;
};

Clingo::Control::Control(StringSpan args, Logger logger, unsigned message_limit)
: impl_(new Clingo::Control::Impl(logger))
{
    Gringo::handleCError(clingo_control_new(args.begin(), args.size(), [](clingo_warning_t code, char const *msg, void *data) {
        try { (*static_cast<Logger*>(data))(static_cast<WarningCode>(code), msg); }
        catch (...) { }
    }, &impl_->logger, message_limit, &impl_->ctl));
}

// }}}1
