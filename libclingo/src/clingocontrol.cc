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

void ClaspAPIBackend::rule(const Potassco::HeadView& head, const Potassco::BodyView& body) {
    prg_.addRule(head, body);
}

void ClaspAPIBackend::minimize(Potassco::Weight_t prio, const Potassco::WeightLitSpan& lits) {
    prg_.addMinimize(prio, lits);
}

void ClaspAPIBackend::project(const Potassco::AtomSpan& atoms) {
    prg_.addProject(atoms);
}

void ClaspAPIBackend::output(const Potassco::StringSpan& str, const Potassco::LitSpan& condition) {
    prg_.addOutput(str, condition);
}

void ClaspAPIBackend::acycEdge(int s, int t, const Potassco::LitSpan& condition) {
    prg_.addAcycEdge(s, t, condition);
}

void ClaspAPIBackend::heuristic(Potassco::Atom_t a, Potassco::Heuristic_t t, int bias, unsigned prio, const Potassco::LitSpan& condition) {
    prg_.addDomHeuristic(a, t, bias, prio, condition);
}

void ClaspAPIBackend::assume(const Potassco::LitSpan& lits) {
    prg_.addAssumption(lits);
}

void ClaspAPIBackend::external(Potassco::Atom_t a, Potassco::Value_t v) {
    switch (v) {
        case Potassco::Value_t::False:   { prg_.freeze(a, Clasp::value_false); break; }
        case Potassco::Value_t::True:    { prg_.freeze(a, Clasp::value_true); break; }
        case Potassco::Value_t::Free:    { prg_.freeze(a, Clasp::value_free); break; }
        case Potassco::Value_t::Release: { prg_.unfreeze(a); break; }
    }
}

void ClaspAPIBackend::theoryTerm(Potassco::Id_t, int) { }

void ClaspAPIBackend::theoryTerm(Potassco::Id_t, const Potassco::StringSpan&) { }

void ClaspAPIBackend::theoryTerm(Potassco::Id_t, int, const Potassco::IdSpan&) { }

void ClaspAPIBackend::theoryElement(Potassco::Id_t e, const Potassco::IdSpan&, const Potassco::LitSpan& cond) {
    Potassco::TheoryElement const &elem = prg_.theoryData().getElement(e);
    if (elem.condition() == Potassco::TheoryData::COND_DEFERRED) { prg_.theoryData().setCondition(e, prg_.newCondition(cond)); }
}

void ClaspAPIBackend::theoryAtom(Potassco::Id_t, Potassco::Id_t, const Potassco::IdSpan&) { }

void ClaspAPIBackend::theoryAtom(Potassco::Id_t, Potassco::Id_t, const Potassco::IdSpan&, Potassco::Id_t, Potassco::Id_t){ }

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
, clingoStatsNg_(clingoStats_)
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
    return Clasp::encodeLit(Clasp::Asp::solverLiteral(prg, lit));
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
        out_ = gringo_make_unique<Output::OutputBase>(claspOut->theoryData(), std::move(outPreds), gringo_make_unique<ClaspAPIBackend>(*claspOut), opts.outputDebug);
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
        return clasp_->ok();
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
    if (!grounded) {
        out_->beginStep();
        grounded = true;
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
    clasp_->interrupt(SIGUSR1);
}
bool ClingoControl::blocked() {
    return clasp_->solving();
}
void ClingoControl::prepare(Gringo::Control::ModelHandler mh, Gringo::Control::FinishHandler fh) {
    grounded = false;
    if (update()) { out_->endStep(true, logger_); }
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
            propLock_.init(clasp_->ctx.concurrency());
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
ClingoStatistics *ClingoControl::getStats() {
    clingoStats_.clasp = clasp_;
    return &clingoStats_;
}

Gringo::StatisticsNG *ClingoControl::statistics() {
    clingoStats_.clasp = clasp_;
    return &clingoStatsNg_;
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

bool skipDomain(Gringo::Sig sig)                                           { return sig.name().startsWith("#"); }
void setAtomOffset(Gringo::SymbolicAtomIter &it, uint32_t offset)         { it.atom_offset = (offset << 1) | (it.atom_offset & 1); }
void setDomainOffset(Gringo::SymbolicAtomIter &it, uint32_t offset)       { it.domain_offset = (offset << 1) | (it.domain_offset & 1); }
uint32_t atomOffset(Gringo::SymbolicAtomIter it)                          { return it.atom_offset >> 1; }
uint32_t domainOffset(Gringo::SymbolicAtomIter it)                        { return it.domain_offset >> 1; }
bool advanceDomain(Gringo::SymbolicAtomIter &it)                          { return it.domain_offset & 1; }
void advanceDomainOffset(Gringo::SymbolicAtomIter &it)                    { it.domain_offset += 2; }
void advanceAtomOffset(Gringo::SymbolicAtomIter &it)                      { it.atom_offset += 2; }
Gringo::SymbolicAtomIter init(uint32_t domainOffset, uint32_t atomOffset) { return { (domainOffset << 1) | 1, (atomOffset << 1) | 1}; }

Gringo::SymbolicAtomIter init(Gringo::Output::OutputBase &out, uint32_t domainOffset, bool advance) {
    Gringo::SymbolicAtomIter it{ (domainOffset << 1) | uint32_t(advance), 0 };
    for (auto domIt = out.predDoms().begin() + domainOffset, domIe = out.predDoms().end(); domIt != domIe; ++domIt, advanceDomainOffset(it)) {
        auto &dom = **domIt;
        if (!skipDomain(dom) && dom.size() > 0) { return it; }
        if (!advanceDomain(it)) { break; }
    }
    setDomainOffset(it, out.predDoms().size());
    return it;
}

void advance(Gringo::Output::OutputBase &out, Gringo::SymbolicAtomIter &it) {
    auto domIt  = out.predDoms().begin() + domainOffset(it);
    auto domIe  = out.predDoms().end();
    auto elemIt = (*domIt)->begin() + atomOffset(it);
    auto elemIe = (*domIt)->end();
    ++elemIt; advanceAtomOffset(it);
    while (elemIt == elemIe) {
        setAtomOffset(it, 0);
        if (!advanceDomain(it)) {
            setDomainOffset(it, out.predDoms().size());
            return;
        }
        ++domIt; advanceDomainOffset(it);
        if (domIt == domIe) { return; }
        if (!skipDomain(**domIt)) {
            elemIt = (*domIt)->begin();
            elemIe = (*domIt)->end();
        }
    }
}

} // namespace

Gringo::Symbol ClingoControl::atom(Gringo::SymbolicAtomIter it) const {
    return (*out_->predDoms()[domainOffset(it)])[atomOffset(it)];
}

Potassco::Lit_t ClingoControl::literal(Gringo::SymbolicAtomIter it) const {
    auto &elem = (*out_->predDoms()[domainOffset(it)])[atomOffset(it)];
    return elem.hasUid() ? elem.uid() : 0;
}

bool ClingoControl::fact(Gringo::SymbolicAtomIter it) const {
    return (*out_->predDoms()[domainOffset(it)])[atomOffset(it)].fact();
}

bool ClingoControl::external(Gringo::SymbolicAtomIter it) const {
    auto &elem = (*out_->predDoms()[domainOffset(it)])[atomOffset(it)];
    return elem.hasUid() && elem.isExternal() && static_cast<Clasp::Asp::LogicProgram*>(clasp_->program())->isExternal(elem.uid());
}

Gringo::SymbolicAtomIter ClingoControl::next(Gringo::SymbolicAtomIter it) {
    advance(*out_, it);
    return it;
}

bool ClingoControl::valid(Gringo::SymbolicAtomIter it) const {
    auto off = domainOffset(it);
    return off < out_->predDoms().size() && atomOffset(it) < out_->predDoms()[off]->size();
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
    return {out_->predDoms().size() << 1 , 0};
}

bool ClingoControl::eq(Gringo::SymbolicAtomIter it, Gringo::SymbolicAtomIter jt) const {
    return domainOffset(it) == domainOffset(jt) && atomOffset(it) == atomOffset(jt);
}

Gringo::SymbolicAtomIter ClingoControl::lookup(Gringo::Symbol atom) const {
    if (atom.hasSig()) {
        auto it = out_->predDoms().find(atom.sig());
        if (it != out_->predDoms().end()) {
            auto jt = (*it)->find(atom);
            if (jt != (*it)->end()) {
                return init(out_->predDoms().offset(it), jt - (*it)->begin());
            }
        }
    }
    return init(out_->predDoms().size(), 0);
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

void ClingoControl::parse(char const *program, std::function<void(clingo_ast const &)> cb) {
    Gringo::Input::ASTBuilder builder(cb);
    Gringo::Input::NonGroundParser parser(builder);
    parser.pushStream("<string>", Gringo::gringo_make_unique<std::istringstream>(program), logger_);
    parser.parse(logger_);
    // TODO: implement better error handling
    if (logger_.hasError()) {
        throw std::runtime_error("TODO: syntax errors here should not be fatal");
    }
}

void ClingoControl::add(std::function<void (std::function<void (clingo_ast const &)>)> cb) {
    Gringo::Input::ASTParser p(scripts_, prg_, *out_, defs_);
    cb([&p](clingo_ast_t const &ast) {
        p.parse(ast);
    });
    defs_.init(logger_);
    if (logger_.hasError()) {
        throw std::runtime_error("parsing failed");
    }
}

Gringo::Backend *ClingoControl::backend() { return out_->backend(); }
Potassco::Atom_t ClingoControl::addProgramAtom() { return out_->data.newAtom(); }

ClingoControl::~ClingoControl() noexcept = default;

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

Potassco::Id_t ClingoStatisticsNG::root() const {
    auto ret = keys_.push(Gringo::gringo_make_unique<Key>(""));
    return keys_.offset(ret.first);
}

Potassco::Id_t ClingoStatisticsNG::add(std::string const &path, char const *name) const {
    auto p = path;
    if (!p.empty()) { p+= '.'; }
    p+= name;
    auto ret = keys_.push(Gringo::gringo_make_unique<Key>(std::move(p)));
    return keys_.offset(ret.first);
}

size_t ClingoStatisticsNG::size(Potassco::Id_t key) const {
    if (type(key) != Array) { throw std::runtime_error("not an array"); }
    auto &k = *keys_.at(key);
    std::string path = k.path + ".__len";
    size_t len = (size_t)(double)stats_.getStat(path.c_str());
    return len;
}

size_t ClingoStatisticsNG::subkeys(Potassco::Id_t key) const {
    if (type(key) != Map) { throw std::runtime_error("not a map"); }
    auto &k = *keys_.at(key);
    char const *keys = stats_.getKeys(k.path.c_str());
    size_t numKeys = 0;
    for (char const *it = keys; *it; it+= strlen(it) + 1, ++numKeys) { }
    return numKeys;
}

char const *ClingoStatisticsNG::subkey(Potassco::Id_t key, size_t index) const {
    if (type(key) != Map) { throw std::runtime_error("not a map"); }
    auto &k = *keys_.at(key);
    char const *keys = stats_.getKeys(k.path.c_str());
    for (char const *it = keys; *it; it+= strlen(it) + 1, --index) {
        if (index == 0) {
            size_t n = strlen(it);
            if (n > 0 && it[n-1] == '.') { --n; }
            return k.keys.emplace(it, it + n).first->c_str();
        }
    }
    throw std::runtime_error("key not found");
}

Potassco::Id_t ClingoStatisticsNG::lookup(Potassco::Id_t key, char const *name) const {
    if (type(key) != Map) { throw std::runtime_error("not a map"); }
    auto &k = *keys_.at(key);
    return add(k.path, name);
}

Potassco::Id_t ClingoStatisticsNG::at(Potassco::Id_t key, size_t index) const {
    if (type(key) != Array) { throw std::runtime_error("not an array"); }
    auto &k = *keys_.at(key);
    return add(k.path, std::to_string(index).c_str());
}

ClingoStatisticsNG::Type ClingoStatisticsNG::type(Potassco::Id_t key) const {
    auto &k = *keys_.at(key);
    auto ret = stats_.getStat(k.path.c_str());
    switch (ret.error()) {
        case Gringo::Statistics::error_none:               { return Leaf; }
        case Gringo::Statistics::error_ambiguous_quantity: {
            char const *keys = stats_.getKeys(k.path.c_str());
            return strcmp(keys, "__len") == 0 ? Array : Map;
        }
        case Gringo::Statistics::error_not_available:      { throw std::runtime_error("not available"); }
        case Gringo::Statistics::error_unknown_quantity:   { throw std::runtime_error("unknown quantity"); }
    }
    throw std::logic_error("cannot happen");
    return Leaf;
}

double ClingoStatisticsNG::value(Potassco::Id_t key) const {
    auto &k = *keys_.at(key);
    auto ret = stats_.getStat(k.path.c_str());
    switch (ret.error()) {
        case Gringo::Statistics::error_none:               { return ret; }
        case Gringo::Statistics::error_ambiguous_quantity: { throw std::runtime_error("not a leaf"); }
        case Gringo::Statistics::error_not_available:      { throw std::runtime_error("not available"); }
        case Gringo::Statistics::error_unknown_quantity:   { throw std::runtime_error("unknown quantity"); }
    }
    throw std::logic_error("cannot happen");
    return 0;
}

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
#endif

// {{{1 definition of ClingoLib

ClingoLib::ClingoLib(Gringo::Scripts &scripts, int argc, char const * const *argv, Gringo::Logger::Printer printer, unsigned messageLimit)
        : ClingoControl(scripts, true, &clasp_, claspConfig_, nullptr, nullptr, printer, messageLimit) {
    using namespace ProgramOptions;
    OptionContext allOpts("<pyclingo>");
    initOptions(allOpts);
    ParsedValues values = parseCommandArray(argv, argc, allOpts, false, parsePositional);
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

extern "C" clingo_error_t clingo_module_new(clingo_module_t **mod) {
    GRINGO_CLINGO_TRY { *mod = new DefaultGringoModule(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" void clingo_module_free(clingo_module_t *mod) {
    delete mod;
}

Clingo::Module::Module()
: module_(nullptr) {
    Gringo::handleCError(clingo_module_new(&module_));
}

Clingo::Control Clingo::Module::make_control(StringSpan args, Logger &logger, unsigned message_limit) {
    clingo_control_t *ctl;
    Gringo::handleCError(clingo_control_new(module_, args.begin(), args.size(), [](clingo_warning_t code, char const *msg, void *data) {
        try { (*static_cast<Logger*>(data))(static_cast<WarningCode>(code), msg); }
        catch (...) { }
    }, &logger, message_limit, &ctl));
    return ctl;
}

Clingo::Control Clingo::Module::make_control(StringSpan args) {
    clingo_control_t *ctl;
    Gringo::handleCError(clingo_control_new(module_, args.begin(), args.size(), [](clingo_warning_t code, char const *msg, void *data) {
        try { (*static_cast<Logger*>(data))(static_cast<WarningCode>(code), msg); }
        catch (...) { }
    }, nullptr, 20, &ctl));
    return ctl;
}

Clingo::Module::~Module() {
    if (module_) { clingo_module_free(module_); }
}

// }}}1
