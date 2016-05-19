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

// {{{1 definition of ClaspAPIBackend

void ClaspAPIBackend::init(bool) {

}

void ClaspAPIBackend::endStep() {

}

void ClaspAPIBackend::beginStep() {
}

void ClaspAPIBackend::addBody(const LitVec& body) {
    for (auto x : body) {
        body_.add((Clasp::Var)std::abs(x), x > 0);
    }
}

void ClaspAPIBackend::addBody(const LitWeightVec& body) {
    for (auto x : body) {
        body_.add((Clasp::Var)std::abs(x.lit), x.lit > 0, x.weight);
    }
}

void ClaspAPIBackend::printHead(bool choice, AtomVec const &atoms) {
    head_.reset(choice ? Clasp::Asp::Head_t::Choice : Clasp::Asp::Head_t::Disjunctive);
    for (auto x : atoms) { head_.add(x); }
}

void ClaspAPIBackend::printNormalBody(LitVec const &body) {
    body_.reset(Potassco::Body_t::Normal);
    addBody(body);
    prg_.addRule(head_.toView(), body_.toView());
}

void ClaspAPIBackend::printWeightBody(Potassco::Weight_t lower, LitWeightVec const &body) {
    body_.reset(Potassco::Body_t::Sum);
    body_.bound = lower;
    addBody(body);
    prg_.addRule(head_.toView(), body_.toView());
}

void ClaspAPIBackend::printMinimize(int priority, LitWeightVec const &body) {
    body_.reset(Potassco::Body_t::Sum);
    prg_.addMinimize(priority, Potassco::toSpan(body));
}

void ClaspAPIBackend::printProject(AtomVec const &lits) {
    prg_.addProject(Potassco::toSpan(lits));
}

void ClaspAPIBackend::printOutput(char const *symbol, LitVec const &body) {
    prg_.addOutput(symbol, Potassco::toSpan(body));
}

void ClaspAPIBackend::printEdge(unsigned u, unsigned v, LitVec const &body) {
    prg_.addAcycEdge(u, v, Potassco::toSpan(body));
}

void ClaspAPIBackend::printHeuristic(Potassco::Heuristic_t modifier, Potassco::Atom_t atom, int value, unsigned priority, LitVec const &body) {
    prg_.addDomHeuristic(atom, modifier, value, priority, Potassco::toSpan(body));
}

void ClaspAPIBackend::printAssume(LitVec const &lits) {
    prg_.addAssumption(Potassco::toSpan(lits));
}

void ClaspAPIBackend::printExternal(Potassco::Atom_t atomUid, Potassco::Value_t type) {
    switch (type) {
        case Potassco::Value_t::False:   { prg_.freeze(atomUid, Clasp::value_false); break; }
        case Potassco::Value_t::True:    { prg_.freeze(atomUid, Clasp::value_true); break; }
        case Potassco::Value_t::Free:    { prg_.freeze(atomUid, Clasp::value_free); break; }
        case Potassco::Value_t::Release: { prg_.unfreeze(atomUid); break; }
    }
}

void ClaspAPIBackend::printTheoryAtom(Potassco::TheoryAtom const &atom, GetCond getCond) {
    for (auto&& e : atom.elements()) {
        Potassco::TheoryElement const &elem = data_.getElement(e);
        if (elem.condition() == Potassco::TheoryData::COND_DEFERRED) {
            Potassco::LitVec cond;
            data_.setCondition(e, prg_.newCondition(Potassco::toSpan(getCond(e))));
        }
    }
}

ClaspAPIBackend::~ClaspAPIBackend() noexcept = default;

// {{{1 definition of ClingoControl

#define LOG if (verbose_) std::cerr
ClingoControl::ClingoControl(Gringo::Scripts &scripts, bool clingoMode, Clasp::ClaspFacade *clasp, Clasp::Cli::ClaspCliConfig &claspConfig, PostGroundFunc pgf, PreSolveFunc psf)
    : scripts_(scripts)
    , clasp_(clasp)
    , claspConfig_(claspConfig)
    , pgf_(pgf)
    , psf_(psf)
    , clingoMode_(clingoMode) { }

void ClingoControl::parse() {
    if (!parser_->empty()) {
        parser_->parse();
        defs_.init();
        parsed = true;
    }
    if (Gringo::message_printer()->hasError()) {
        throw std::runtime_error("parsing failed");
    }
}

Potassco::Lit_t ClingoPropagateInit::mapLit(Lit_t lit) {
    const auto& prg = static_cast<Clasp::Asp::LogicProgram&>(*static_cast<ClingoControl&>(c_).clasp_->program());
    return Clasp::encodeLit(Clasp::Asp::solverLiteral(prg, lit));
}

int ClingoPropagateInit::threads() {
    return static_cast<ClingoControl&>(c_).clasp_->config()->numSolver();
}

void ClingoControl::parse(const StringSeq& files, const ClingoOptions& opts, Clasp::Asp::LogicProgram* claspOut, bool addStdIn) {
    using namespace Gringo;
    if (opts.wNoOperationUndefined) { message_printer()->disable(W_OPERATION_UNDEFINED); }
    if (opts.wNoAtomUndef)          { message_printer()->disable(W_ATOM_UNDEFINED); }
    if (opts.wNoVariableUnbounded)  { message_printer()->disable(W_VARIABLE_UNBOUNDED); }
    if (opts.wNoFileIncluded)       { message_printer()->disable(W_FILE_INCLUDED); }
    if (opts.wNoGlobalVariable)     { message_printer()->disable(W_GLOBAL_VARIABLE); }
    verbose_ = opts.verbose;
    Output::OutputPredicates outPreds;
    for (auto &x : opts.foobar) {
        outPreds.emplace_back(Location("<cmd>",1,1,"<cmd>", 1,1), x, false);
    }
    if (claspOut) {
        auto create = [&](Output::TheoryData &data) -> UBackend { return gringo_make_unique<ClaspAPIBackend>(const_cast<Potassco::TheoryData&>(data.data()), *claspOut); };
        out_ = gringo_make_unique<Output::OutputBase>(create, claspOut->theoryData(), std::move(outPreds), opts.outputDebug);
    }
    else {
        data_ = gringo_make_unique<Potassco::TheoryData>();
        out_ = gringo_make_unique<Output::OutputBase>(*data_, std::move(outPreds), std::cout, opts.outputFormat, opts.outputDebug);
    }
    out_->keepFacts = opts.keepFacts;
    pb_ = gringo_make_unique<Input::NongroundProgramBuilder>(scripts_, prg_, *out_, defs_, opts.rewriteMinimize);
    parser_ = gringo_make_unique<Input::NonGroundParser>(*pb_);
    for (auto &x : opts.defines) {
        LOG << "define: " << x << std::endl;
        parser_->parseDefine(x);
    }
    for (auto x : files) {
        LOG << "file: " << x << std::endl;
        parser_->pushFile(std::move(x));
    }
    if (files.empty() && addStdIn) {
        LOG << "reading from stdin" << std::endl;
        parser_->pushFile("-");
    }
    parse();
}

bool ClingoControl::update() {
    if (clingoMode_) {
        clasp_->update(configUpdate_);
        configUpdate_ = false;
        return clasp_->ok();
    }
    return true;
}

void ClingoControl::ground(Gringo::Control::GroundVec const &parts, Gringo::Context *context) {
    if (!update()) { return; }
    if (parsed) {
        LOG << "************** parsed program **************" << std::endl << prg_;
        prg_.rewrite(defs_);
        LOG << "************* rewritten program ************" << std::endl << prg_;
        prg_.check();
        if (Gringo::message_printer()->hasError()) {
            throw std::runtime_error("grounding stopped because of errors");
        }
        parsed = false;
    }
    if (!grounded) {
        out_->beginStep();
        grounded = true;
    }
    if (!parts.empty()) {
        Gringo::Ground::Parameters params;
        for (auto &x : parts) { params.add(x.first, Gringo::SymVec(x.second)); }
        auto gPrg = prg_.toGround(out_->data);
        LOG << "*********** intermediate program ***********" << std::endl << gPrg << std::endl;
        LOG << "************* grounded program *************" << std::endl;
        auto exit = Gringo::onExit([this]{ scripts_.context = nullptr; });
        scripts_.context = context;
        gPrg.ground(params, scripts_, *out_, false);
    }
}

void ClingoControl::main() {
    if (scripts_.callable("main")) {
        out_->init(true);
        clasp_->enableProgramUpdates();
        scripts_.main(*this);
    }
    else {
        out_->init(false);
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
    return modelHandler_(ClingoModel(static_cast<Clasp::Asp::LogicProgram&>(*clasp_->program()), *out_, clasp_->ctx, &m));
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
        Gringo::Symbol val = std::get<2>(ret->second)->eval(undefined);
        if (!undefined) { return val; }
    }
    return Gringo::Symbol();
}
void ClingoControl::add(std::string const &name, Gringo::FWStringVec const &params, std::string const &part) {
    Gringo::Location loc("<block>", 1, 1, "<block>", 1, 1);
    Gringo::Input::IdVec idVec;
    for (auto &x : params) { idVec.emplace_back(loc, x); }
    parser_->pushBlock(name, std::move(idVec), part);
    parse();
}
void ClingoControl::load(std::string const &filename) {
    parser_->pushFile(std::string(filename));
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
    solveIter_ = Gringo::gringo_make_unique<ClingoSolveIter>(clasp_->startSolve(a), static_cast<Clasp::Asp::LogicProgram&>(*clasp_->program()), *out_, clasp_->ctx);
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
    clasp_->interrupt(SIGINT);
}
bool ClingoControl::blocked() {
    return clasp_->solving();
}
void ClingoControl::prepare(Gringo::Control::ModelHandler mh, Gringo::Control::FinishHandler fh) {
    grounded = false;
    if (update()) { out_->endStep(true); }
    if (clingoMode_) {
#if WITH_THREADS
        solveFuture_   = nullptr;
#endif
        solveIter_     = nullptr;
        finishHandler_ = fh;
        modelHandler_  = mh;
        Clasp::ProgramBuilder *prg = clasp_->program();
        if (pgf_) { pgf_(*prg); }
        if (!propagators_.empty()) {
            clasp_->program()->endProgram();
            for (auto&& pp : propagators_) {
                ClingoPropagateInit i(*this, *pp);
                static_cast<Gringo::Propagator*>(pp->propagator())->init(i);
            }
            propLock_.init(clasp_->config()->numSolver());
        }
        clasp_->prepare(enableEnumAssupmption_ ? Clasp::ClaspFacade::enum_volatile : Clasp::ClaspFacade::enum_static);
        if (psf_) { psf_(*clasp_);}
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
    return clingoMode_ ? convert(clasp_->solve(nullptr, toClaspAssumptions(std::move(ass)))) : Gringo::SolveResult(Gringo::SolveResult::Unknown, false, false);
}

void ClingoControl::registerPropagator(Gringo::Propagator &p, bool sequential) {
    propagators_.emplace_back(Gringo::gringo_make_unique<Clasp::ClingoPropagatorInit>(p, propLock_.add(sequential)));
    claspConfig_.addConfigurator(propagators_.back().get(), Clasp::Ownership_t::Retain);
}
void ClingoControl::cleanupDomains() {
    out_->endStep(false);
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
ClingoStatistics *ClingoControl::getStats() {
    clingoStats_.clasp = clasp_;
    return &clingoStats_;
}
void ClingoControl::useEnumAssumption(bool enable) {
    enableEnumAssupmption_ = enable;
}
bool ClingoControl::useEnumAssumption() {
    return enableEnumAssupmption_;
}

Gringo::DomainProxy &ClingoControl::getDomain() {
    if (clingoMode_) { return *this; }
    else {
        throw std::runtime_error("domain introspection only supported in clingo mode");
    }
}

namespace {

static bool skipDomain(Gringo::Sig sig) {
    return sig.name().startsWith("#");
}

struct ClingoDomainElement : Gringo::DomainProxy::Element {
    using ElemIt = Gringo::Output::PredicateDomain::Iterator;
    using DomIt = Gringo::Output::PredDomMap::Iterator;
    ClingoDomainElement(Gringo::Output::OutputBase &out, Clasp::Asp::LogicProgram &prg, DomIt const &domIt, ElemIt const &elemIt, bool advanceDom = true)
    : out(out)
    , prg(prg)
    , domIt(domIt)
    , elemIt(elemIt)
    , advanceDom(advanceDom) {
        assert(domIt != out.predDoms().end() && elemIt != (*domIt)->end());
    }
    Gringo::Symbol atom() const {
        return *elemIt;
    }
    bool fact() const {
        return elemIt->fact();
    }
    bool external() const {
        return
            elemIt->hasUid() &&
            elemIt->isExternal() &&
            prg.isExternal(elemIt->uid());
    }

    Potassco::Lit_t literal() const {
        return elemIt->hasUid() ? elemIt->uid() : 0;
    }

    static std::unique_ptr<ClingoDomainElement> init(Gringo::Output::OutputBase &out, Clasp::Asp::LogicProgram &prg, bool advanceDom, Gringo::Output::PredDomMap::Iterator domIt) {
        for (; domIt != out.predDoms().end(); ++domIt) {
            if (!skipDomain(**domIt)) {
                auto elemIt = (*domIt)->begin();
                if (elemIt != (*domIt)->end()) {
                    return Gringo::gringo_make_unique<ClingoDomainElement>(out, prg, domIt, elemIt, advanceDom);
                }
            }
            if (!advanceDom) { return nullptr; }
        }
        return nullptr;
    }

    static std::unique_ptr<ClingoDomainElement> advance(Gringo::Output::OutputBase &out, Clasp::Asp::LogicProgram &prg, bool advanceDom, Gringo::Output::PredDomMap::Iterator domIt, Gringo::Output::PredicateDomain::Iterator elemIt) {
        auto domIe  = out.predDoms().end();
        auto elemIe = (*domIt)->end();
        ++elemIt;
        while (elemIt == elemIe) {
            if (!advanceDom) { return nullptr; }
            ++domIt;
            if (domIt == domIe) { return nullptr; }
            if (!skipDomain(**domIt)) {
                elemIt = (*domIt)->begin();
                elemIe = (*domIt)->end();
            }
        }
        return Gringo::gringo_make_unique<ClingoDomainElement>(out, prg, domIt, elemIt, advanceDom);
    }

    Gringo::DomainProxy::ElementPtr next() {
        return advance(out, prg, advanceDom, domIt, elemIt);
    }
    bool valid() const {
        return domIt != out.predDoms().end();
    }
    Gringo::Output::OutputBase &out;
    Clasp::Asp::LogicProgram &prg;
    DomIt domIt;
    ElemIt elemIt;
    bool advanceDom;
};

} // namespace

std::vector<Gringo::Sig> ClingoControl::signatures() const {
    std::vector<Gringo::Sig> ret;
    for (auto &dom : out_->predDoms()) {
        if (!skipDomain(*dom)) { ret.emplace_back(*dom); }
    }
    return ret;
}

Gringo::DomainProxy::ElementPtr ClingoControl::iter(Gringo::Sig sig) const {
    return ClingoDomainElement::init(*out_, static_cast<Clasp::Asp::LogicProgram&>(*clasp_->program()), false, out_->predDoms().find(sig));
}

Gringo::DomainProxy::ElementPtr ClingoControl::iter() const {
    return ClingoDomainElement::init(*out_, static_cast<Clasp::Asp::LogicProgram&>(*clasp_->program()), true, out_->predDoms().begin());
}

Gringo::DomainProxy::ElementPtr ClingoControl::lookup(Gringo::Symbol atom) const {
    if (atom.hasSig()) {
        auto it = out_->predDoms().find(atom.sig());
        if (it != out_->predDoms().end()) {
            auto jt = (*it)->find(atom);
            if (jt != (*it)->end()) {
                return Gringo::gringo_make_unique<ClingoDomainElement>(*out_, static_cast<Clasp::Asp::LogicProgram&>(*clasp_->program()), it, jt);
            }
        }
    }
    return nullptr;
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

void ClingoControl::parse(char const *program, std::function<void(Gringo::AST const &)> cb) {
    Gringo::Input::ASTBuilder builder(cb);
    Gringo::Input::NonGroundParser parser(builder);
    parser.pushStream("<string>", Gringo::gringo_make_unique<std::istringstream>(program));
    parser.parse();
    // TODO: implement better error handling
    if (Gringo::message_printer()->hasError()) {
        throw std::runtime_error("TODO: syntax errors here should not be fatal");
    }
}

void ClingoControl::add(std::function<Gringo::AST const *()> cb) {
    Gringo::Input::ASTParser p(scripts_, prg_, *out_, defs_);
    for (Gringo::AST const *node = cb(); node; node = cb()) {
        p.parse(*node);
    }
    defs_.init();
    if (Gringo::message_printer()->hasError()) {
        throw std::runtime_error("parsing failed");
    }
}

Gringo::Backend *ClingoControl::backend() { return out_->backend(); }
Potassco::Atom_t ClingoControl::addProgramAtom() { return out_->data.newAtom(); }

// {{{1 definition of ClingoStatistics

Gringo::Statistics::Quantity ClingoStatistics::getStat(char const* key) const {
    if (!clasp) { return std::numeric_limits<double>::quiet_NaN(); }
    auto ret = clasp->getStat(key);
    switch (ret.error()) {
        case Clasp::ExpectedQuantity::error_ambiguous_quantity: { return Gringo::Statistics::error_ambiguous_quantity; }
        case Clasp::ExpectedQuantity::error_not_available:      { return Gringo::Statistics::error_not_available; }
        case Clasp::ExpectedQuantity::error_unknown_quantity:   { return Gringo::Statistics::error_unknown_quantity; }
        case Clasp::ExpectedQuantity::error_none:               { return (double)ret; }
    }
    return std::numeric_limits<double>::quiet_NaN();
}
char const *ClingoStatistics::getKeys(char const* key) const {
    if (!clasp) { return ""; }
    return clasp->getKeys(key);
}

// {{{1 definition of ClingoSolveIter

ClingoSolveIter::ClingoSolveIter(Clasp::ClaspFacade::ModelGenerator const &future, Clasp::Asp::LogicProgram const &lp, Gringo::Output::OutputBase const &out, Clasp::SharedContext const &ctx)
    : future(future)
    , model(lp, out, ctx) { }
Gringo::Model const *ClingoSolveIter::next() {
    if (future.next()) {
        model.reset(future.model());
        return &model;
    }
    else {
        return nullptr;
    }
}
void ClingoSolveIter::close() {
    future.stop();
}
Gringo::SolveResult ClingoSolveIter::get() {
    return convert(future.result());
}
ClingoSolveIter::~ClingoSolveIter() = default;

// {{{1 definition of ClingoSolveFuture

Gringo::SolveResult convert(Clasp::ClaspFacade::Result res) {
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
        bool stop = future.interrupted() == SIGINT;
        ret       = convert(future.get());
        done      = true;
        if (stop) { throw std::runtime_error("solving stopped by signal"); }
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
ClingoSolveFuture::~ClingoSolveFuture() { }
#endif

// {{{1 definition of ClingoLib

ClingoLib::ClingoLib(Gringo::Scripts &scripts, int argc, char const **argv)
        : ClingoControl(scripts, true, &clasp_, claspConfig_, nullptr, nullptr) {
    using namespace ProgramOptions;
    OptionContext allOpts("<pyclingo>");
    initOptions(allOpts);
    ParsedValues values = parseCommandLine(argc, const_cast<char**>(argv), allOpts, false, parsePositional);
    ParsedOptions parsed;
    parsed.assign(values);
    allOpts.assignDefaults(parsed);
    claspConfig_.finalize(parsed, Clasp::Problem_t::Asp, true);
    clasp_.ctx.setEventHandler(this);
    Clasp::Asp::LogicProgram* lp = &clasp_.startAsp(claspConfig_, true);
    parse({}, grOpts_, lp, false);
    out_->init(true);
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
        ("output-debug", storeTo(grOpts_.outputDebug = Gringo::Output::OutputDebug::NONE, values<Gringo::Output::OutputDebug>()
          ("none", Gringo::Output::OutputDebug::NONE)
          ("text", Gringo::Output::OutputDebug::TEXT)
          ("translate", Gringo::Output::OutputDebug::TRANSLATE)
          ("all", Gringo::Output::OutputDebug::ALL)), "Print debug information during output:\n"
         "      none     : no additional info\n"
         "      text     : print rules as plain text (prefix %%)\n"
         "      translate: print translated rules as plain text (prefix %%%%)\n"
         "      all      : combines text and translate")
        ("warn,W"                   , storeTo(grOpts_, parseWarning)->arg("<warn>")->composing(), "Enable/disable warnings:\n"
         "      [no-]atom-undefined:        a :- b.\n"
         "      [no-]file-included:         #include \"a.lp\". #include \"a.lp\".\n"
         "      [no-]operation-undefined:   p(1/0).\n"
         "      [no-]variable-unbounded:    $x > 10.\n"
         "      [no-]global-variable:       :- #count { X } = 1, X = 1.")
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
    if (log && log->isWarning()) {
        fflush(stdout);
        fprintf(stderr, "*** %-5s: (%s): %s\n", "Warn", "pyclingo", log->msg);
        fflush(stderr);
    }
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
Gringo::Control *DefaultGringoModule::newControl(int argc, char const **argv) {
    return new ClingoLib(scripts, argc, argv);
}

void DefaultGringoModule::freeControl(Gringo::Control *ctl) {
    if (ctl) { delete ctl; }
}
Gringo::Symbol DefaultGringoModule::parseValue(std::string const &str) { return parser.parse(str); }

// }}}1
