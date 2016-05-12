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
#include "clasp/solver.h"
#include <program_opts/typed_value.h>
#include <program_opts/application.h>

// {{{1 definition of ClingoLpOutput

void ClingoLpOutput::addBody(const LitVec& body) {
    for (auto x : body) {
        prg_.addToBody((Clasp::Var)std::abs(x), x > 0);
    }
}
void ClingoLpOutput::addBody(const LitWeightVec& body) {
    for (auto x : body) {
        prg_.addToBody((Clasp::Var)std::abs(x.first), x.first > 0, x.second);
    }
}
void ClingoLpOutput::printBasicRule(unsigned head, LitVec const &body) {
    prg_.startRule().addHead(head);
    addBody(body);
    prg_.endRule();
}

void ClingoLpOutput::printChoiceRule(AtomVec const &atoms, LitVec const &body) {
    prg_.startRule(Clasp::Asp::CHOICERULE);
    for (auto x : atoms) { prg_.addHead(x); }
    addBody(body);
    prg_.endRule();
}

void ClingoLpOutput::printCardinalityRule(unsigned head, unsigned lower, LitVec const &body) {
    prg_.startRule(Clasp::Asp::CONSTRAINTRULE, lower).addHead(head);
    addBody(body);
    prg_.endRule();
}

void ClingoLpOutput::printWeightRule(unsigned head, unsigned lower, LitWeightVec const &body) {
    prg_.startRule(Clasp::Asp::WEIGHTRULE, lower).addHead(head);
    addBody(body);
    prg_.endRule();
}

void ClingoLpOutput::printMinimize(LitWeightVec const &body) {
    prg_.startRule(Clasp::Asp::OPTIMIZERULE);
    addBody(body);
    prg_.endRule();
}

void ClingoLpOutput::printDisjunctiveRule(AtomVec const &atoms, LitVec const &body) {
    prg_.startRule(Clasp::Asp::DISJUNCTIVERULE);
    for (auto x : atoms) { prg_.addHead(x); }
    addBody(body);
    prg_.endRule();
}

void ClingoLpOutput::printSymbol(unsigned atomUid, Gringo::Value v) {
    if ((v.type() == Gringo::Value::ID && !v.sign()) || v.type() == Gringo::Value::STRING) {
        prg_.setAtomName(atomUid, (*v.string()).c_str());
    }
    else {
        str_.str("");
        v.print(str_);
        prg_.setAtomName(atomUid, str_.str().c_str());
    }
}

void ClingoLpOutput::printExternal(unsigned atomUid, Gringo::TruthValue type) {
    switch (type) {
        case Gringo::TruthValue::False: { prg_.freeze(atomUid, Clasp::value_false); break; }
        case Gringo::TruthValue::True:  { prg_.freeze(atomUid, Clasp::value_true); break; }
        case Gringo::TruthValue::Open:  { prg_.freeze(atomUid, Clasp::value_free); break; }
        case Gringo::TruthValue::Free:  { prg_.unfreeze(atomUid); break; }
    }
}

bool &ClingoLpOutput::disposeMinimize() {
    return disposeMinimize_;
}

// {{{1 definition of ClingoControl

#define LOG if (verbose_) std::cerr
ClingoControl::ClingoControl(Gringo::Scripts &scripts, bool clingoMode, Clasp::ClaspFacade *clasp, Clasp::Cli::ClaspCliConfig &claspConfig, PostGroundFunc pgf, PreSolveFunc psf)
    : scripts(scripts)
    , clasp(clasp)
    , claspConfig_(claspConfig)
    , pgf_(pgf)
    , psf_(psf)
    , clingoMode_(clingoMode) { }

void ClingoControl::parse_() {
    if (!parser->empty()) {
        parser->parse();
        defs.init();
        parsed = true;
    }
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
    if (opts.text) {
        out.reset(new Output::OutputBase(std::move(outPreds), std::cout, opts.lpRewrite));
    }
    else {
        if (claspOut) { lpOut.reset(new ClingoLpOutput(*claspOut)); }
        else          { lpOut.reset(new Output::PlainLparseOutputter(std::cout)); }
        out.reset(new Output::OutputBase(std::move(outPreds), *lpOut, opts.lparseDebug));
    }
    out->keepFacts = opts.keepFacts;
    pb = gringo_make_unique<Input::NongroundProgramBuilder>(scripts, prg, *out, defs, opts.rewriteMinimize);
    parser = gringo_make_unique<Input::NonGroundParser>(*pb);
    for (auto &x : opts.defines) {
        LOG << "define: " << x << std::endl;
        parser->parseDefine(x);
    }
    for (auto x : files) {
        LOG << "file: " << x << std::endl;
        parser->pushFile(std::move(x));
    }
    if (files.empty() && addStdIn) {
        LOG << "reading from stdin" << std::endl;
        parser->pushFile("-");
    }
    parse_();
}

bool ClingoControl::update() {
    if (clingoMode_) {
        clasp->update(configUpdate_);
        configUpdate_ = false;
		return clasp->ok();
    }
	return true;
}

void ClingoControl::ground(Gringo::Control::GroundVec const &parts, Gringo::Any &&context) {
    if (!update()) { return; }
    if (parsed) {
        LOG << "************** parsed program **************" << std::endl << prg;
        prg.rewrite(defs);
        LOG << "************* rewritten program ************" << std::endl << prg;
        prg.check();
        if (Gringo::message_printer()->hasError()) {
            throw std::runtime_error("grounding stopped because of errors");
        }
        parsed = false;
    }
    if (!grounded) {
        if (incremental) { out->incremental(); }
        grounded = true;
    }
    if (!parts.empty()) {
        Gringo::Ground::Parameters params;
        for (auto &x : parts) { params.add(x.first, x.second); }
        auto gPrg = prg.toGround(out->domains);
        LOG << "*********** intermediate program ***********" << std::endl << gPrg << std::endl;
        LOG << "************* grounded program *************" << std::endl;
        auto exit = Gringo::onExit([this]{ scripts.context = Gringo::Any(); });
        scripts.context = std::move(context);
        gPrg.ground(params, scripts, *out, false);
    }
}

void ClingoControl::main() {
    if (scripts.callable("main")) {
        incremental = true;
        clasp->enableProgramUpdates();
        scripts.main(*this);
    }
    else {
        claspConfig_.releaseOptions();
        Gringo::Control::GroundVec parts;
        parts.emplace_back("base", Gringo::FWValVec{});
        ground(parts, Gringo::Any());
        prepareSolve({});
        solve(nullptr);
    }
}
bool ClingoControl::onModel(Clasp::Model const &m) {
    return !modelHandler || modelHandler(ClingoModel(static_cast<Clasp::Asp::LogicProgram&>(*clasp->program()), *out, clasp->ctx, &m));
}
void ClingoControl::onFinish(Clasp::ClaspFacade::Result ret) {
    if (finishHandler) {
        finishHandler(convert(ret), ret.interrupted());
        finishHandler = nullptr;
    }
}
Gringo::Value ClingoControl::getConst(std::string const &name) {
    auto ret = defs.defs().find(name);
    if (ret != defs.defs().end()) {
        bool undefined = false;
        Gringo::Value val = std::get<2>(ret->second)->eval(undefined);
        if (!undefined) { return val; }
    }
    return Gringo::Value();
}
void ClingoControl::add(std::string const &name, Gringo::FWStringVec const &params, std::string const &part) {
    Gringo::Location loc("<block>", 1, 1, "<block>", 1, 1);
    Gringo::Input::IdVec idVec;
    for (auto &x : params) { idVec.emplace_back(loc, x); }
    parser->pushBlock(name, std::move(idVec), part);
    parse_();
}
void ClingoControl::load(std::string const &filename) {
    parser->pushFile(std::string(filename));
    parse_();
}
bool ClingoControl::hasSubKey(unsigned key, char const *name, unsigned* subKey) {
    *subKey = claspConfig_.getKey(key, name);
    return *subKey != Clasp::Cli::ClaspCliConfig::INVALID_KEY;
}
unsigned ClingoControl::getSubKey(unsigned key, char const *name) {
    unsigned ret = claspConfig_.getKey(key, name);
    if (ret == Clasp::Cli::ClaspCliConfig::INVALID_KEY) {
        throw std::runtime_error("invalid key");
    }
    return ret;
}
unsigned ClingoControl::getArrKey(unsigned key, unsigned idx) {
    unsigned ret = claspConfig_.getArrKey(key, idx);
    if (ret == Clasp::Cli::ClaspCliConfig::INVALID_KEY) {
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
void ClingoControl::prepareSolve(Assumptions &&ass) {
    if (!grounded) {
        if (incremental) { out->incremental(); }
    }
    grounded = false;
    if (update()) { out->finish(); }
    ass_ = toLparseAssumptions(std::move(ass));
}
Gringo::SolveIter *ClingoControl::solveIter() {
    if (!clingoMode_) { throw std::runtime_error("solveIter is not supported in gringo gringo mode"); }
#if WITH_THREADS
    prepare_(nullptr, nullptr);
    solveIter_ = Gringo::gringo_make_unique<ClingoSolveIter>(clasp->startSolveAsync(), static_cast<Clasp::Asp::LogicProgram&>(*clasp->program()), *out, clasp->ctx);
    return solveIter_.get();
#else
    throw std::runtime_error("solveIter requires clingo to be build with thread support");
#endif
}
Gringo::SolveFuture *ClingoControl::solveAsync(ModelHandler mh, FinishHandler fh) {
    if (!clingoMode_) { throw std::runtime_error("solveAsync is not supported in gringo gringo mode"); }
#if WITH_THREADS
    prepare_(mh, fh);
    solveFuture_ = Gringo::gringo_make_unique<ClingoSolveFuture>(clasp->solveAsync(nullptr));
    return solveFuture_.get();
#else
    (void)mh;
    (void)fh;
    throw std::runtime_error("solveAsync requires clingo to be build with thread support");
#endif
}
bool ClingoControl::blocked() {
    return clasp->solving();
}
void ClingoControl::prepare_(Gringo::Control::ModelHandler mh, Gringo::Control::FinishHandler fh) {
    if (clingoMode_) {
#if WITH_THREADS
        solveIter_    = nullptr;
        solveFuture_  = nullptr;
#endif
        finishHandler = fh;
        modelHandler  = mh;
        Clasp::ProgramBuilder *prg = clasp->program();
        if (lpOut && lpOut->disposeMinimize()) { prg->disposeMinimizeConstraint(); }
        if (pgf_) { pgf_(*prg); }
        clasp->prepare(enableEnumAssupmption_ ? Clasp::ClaspFacade::enum_volatile : Clasp::ClaspFacade::enum_static);
        if (psf_) {  psf_(*clasp);}
        clasp->assume(toClaspAssumptions(std::move(ass_)));
    }
}

std::vector<int> ClingoControl::toLparseAssumptions(Gringo::Control::Assumptions &&ass) const {
    std::vector<int> outAss;
	if (!clingoMode_ || !clasp->program()) { return outAss; }
    for (auto &x : ass) {
        auto atm = out->find2(x.first);
        if (atm && atm->second.hasUid()) {
            int uid = atm->second.uid();
            outAss.push_back(x.second ? uid : -uid);
        }
        else if (x.second) {
            outAss.push_back(1);
            break;
        }
    }
    return outAss;

}
Clasp::LitVec ClingoControl::toClaspAssumptions(std::vector<int> &&ass) const {
    Clasp::LitVec outAss;
	const Clasp::Asp::LogicProgram* prg = static_cast<const Clasp::Asp::LogicProgram*>(clasp->program());
    for (auto &x : ass) {
        Clasp::Literal lit = prg->getLiteral(std::abs(x));
        outAss.push_back(x > 0 ? lit : ~lit);
    }
    ass.clear();
    return outAss;
}

Gringo::SolveResult ClingoControl::solve(ModelHandler h) {
    prepare_(h, nullptr);
    return clingoMode_ ? convert(clasp->solve(nullptr)) : Gringo::SolveResult::UNKNOWN;
}

void ClingoControl::cleanupDomains() {
    prepareSolve({});
    prepare_(nullptr, nullptr);
    if (clingoMode_) {
        Clasp::Asp::LogicProgram &prg = static_cast<Clasp::Asp::LogicProgram&>(*clasp->program());
        Clasp::Solver &solver = *clasp->ctx.master();
        auto assignment = [&prg, &solver](unsigned uid) {
            Clasp::Literal lit = prg.getLiteral(uid);
            Gringo::TruthValue              truth = Gringo::TruthValue::Open;
            if (solver.isTrue(lit))       { truth = Gringo::TruthValue::True; }
            else if (solver.isFalse(lit)) { truth = Gringo::TruthValue::False; }
            return std::make_pair(prg.isExternal(uid), truth);
        };
        auto stats = out->simplify(assignment);
        LOG << stats.first << " atom" << (stats.first == 1 ? "" : "s") << " became facts" << std::endl;
        LOG << stats.second << " atom" << (stats.second == 1 ? "" : "s") << " deleted" << std::endl;
    }
}

std::string ClingoControl::str() {
    return "[object:IncrementalControl]";
}

void ClingoControl::assignExternal(Gringo::Value ext, Gringo::TruthValue val) {
    if (update()) {
        Gringo::PredicateDomain::element_type *atm = out->find2(ext);
        if (atm && atm->second.hasUid()) {
            out->assignExternal(*atm, val);
        }
    }
}
ClingoStatistics *ClingoControl::getStats() {
    clingoStats.clasp = clasp;
    return &clingoStats;
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

static bool skipDomain(Gringo::FWSignature sig) {
    return (strncmp((*(*sig).name()).c_str(), "#", 1) == 0);
}

struct ClingoDomainElement : Gringo::DomainProxy::Element {
    using ElemIt = Gringo::PredicateDomain::element_vec::iterator;
    using DomIt = Gringo::PredDomMap::iterator;
    ClingoDomainElement(Gringo::Output::OutputBase &out, Clasp::Asp::LogicProgram &prg, DomIt const &domIt, ElemIt const &elemIt, bool advanceDom = true)
    : out(out)
    , prg(prg)
    , domIt(domIt)
    , elemIt(elemIt)
    , advanceDom(advanceDom) {
        assert(domIt != out.domains.end() && elemIt != domIt->second.exports.end());
    }
    Gringo::Value atom() const {
        return elemIt->get().first;
    }
    bool fact() const {
        return elemIt->get().second.fact(false);
    }
    bool external() const {
        return
            elemIt->get().second.hasUid() &&
            elemIt->get().second.isExternal() &&
            prg.isExternal(elemIt->get().second.uid());
    }
    Gringo::DomainProxy::ElementPtr next() {
        auto domIe  = out.domains.end();
        auto domIt  = this->domIt;
        auto elemIt = this->elemIt;
        if (domIt != domIe) {
            assert(elemIt != domIt->second.exports.end());
            ++elemIt;
            if (elemIt != domIt->second.exports.end()) {
                return Gringo::gringo_make_unique<ClingoDomainElement>(out, prg, domIt, elemIt, advanceDom);
            }
            if (advanceDom) {
                for (++domIt; domIt != domIe; ++domIt) {
                    if (!skipDomain(domIt->first) && !domIt->second.exports.exports.empty()) {
                        elemIt = domIt->second.exports.begin();
                        return Gringo::gringo_make_unique<ClingoDomainElement>(out, prg, domIt, elemIt, advanceDom);
                    }
                }
            }
        }
        return nullptr;
    }
    bool valid() const {
        return domIt != out.domains.end();
    }
    Gringo::Output::OutputBase &out;
    Clasp::Asp::LogicProgram &prg;
    DomIt domIt;
    ElemIt elemIt;
    bool advanceDom;
};

} // namespace

std::vector<Gringo::FWSignature> ClingoControl::signatures() const {
    std::vector<Gringo::FWSignature> ret;
    for (auto &dom : out->domains) {
        if (!skipDomain(dom.first)) {
            ret.emplace_back(dom.first);
        }
    }
    return ret;
}

Gringo::DomainProxy::ElementPtr ClingoControl::iter(Gringo::Signature const &sig) const {
    auto it = out->domains.find(sig);
    if (it != out->domains.end()) {
        auto jt = it->second.exports.begin();
        if (jt != it->second.exports.end()) {
            return Gringo::gringo_make_unique<ClingoDomainElement>(*out, static_cast<Clasp::Asp::LogicProgram&>(*clasp->program()), it, jt, false);
        }
    }
    return nullptr;
}

Gringo::DomainProxy::ElementPtr ClingoControl::iter() const {
    for (auto it = out->domains.begin(), ie = out->domains.end(); it != ie; ++it) {
        if (!skipDomain(it->first) && !it->second.exports.exports.empty()) {
            return Gringo::gringo_make_unique<ClingoDomainElement>(*out, static_cast<Clasp::Asp::LogicProgram&>(*clasp->program()), it, it->second.exports.begin());
        }
    }
    return nullptr;
}

Gringo::DomainProxy::ElementPtr ClingoControl::lookup(Gringo::Value const &atom) const {
    if (atom.hasSig()) {
        auto it = out->domains.find(atom.sig());
        if (it != out->domains.end()) {
            auto jt = it->second.domain.find(atom);
            if (jt != it->second.domain.end()) {
                auto kt = it->second.exports.begin() + jt->second.generation();
                return Gringo::gringo_make_unique<ClingoDomainElement>(*out, static_cast<Clasp::Asp::LogicProgram&>(*clasp->program()), it, kt);
            }
        }
    }
    return nullptr;
}

size_t ClingoControl::length() const {
    size_t ret = 0;
    for (auto &dom : out->domains) {
        if (!skipDomain(dom.first)) {
            ret += dom.second.exports.size();
        }
    }
    return ret;
}

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

#if WITH_THREADS
ClingoSolveIter::ClingoSolveIter(Clasp::ClaspFacade::AsyncResult const &future, Clasp::Asp::LogicProgram const &lp, Gringo::Output::OutputBase const &out, Clasp::SharedContext const &ctx)
    : future(future)
    , model(lp, out, ctx) { }
Gringo::Model const *ClingoSolveIter::next() {
    if (model.model)  { future.next(); }
    if (future.end()) { return nullptr; }
    model.reset(future.model());
    return &model;
}
void ClingoSolveIter::close() {
    if (!future.end()) { future.cancel(); }
}
Gringo::SolveResult ClingoSolveIter::get() {
    return convert(future.get());
}
ClingoSolveIter::~ClingoSolveIter() = default;
#endif

// {{{1 definition of ClingoSolveFuture

Gringo::SolveResult convert(Clasp::ClaspFacade::Result res) {
    switch (res) {
        case Clasp::ClaspFacade::Result::SAT:     { return Gringo::SolveResult::SAT; }
        case Clasp::ClaspFacade::Result::UNSAT:   { return Gringo::SolveResult::UNSAT; }
        case Clasp::ClaspFacade::Result::UNKNOWN: { return Gringo::SolveResult::UNKNOWN; }
    }
    return Gringo::SolveResult::UNKNOWN;
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
    claspConfig_.finalize(parsed, Clasp::Problem_t::ASP, true);
    clasp_.ctx.setEventHandler(this);
    Clasp::Asp::LogicProgram* lp = &clasp_.startAsp(claspConfig_, true);
    incremental = true;
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
    grOpts_.text = false;
    gringo.addOptions()
        ("verbose,V"                , flag(grOpts_.verbose = false), "Enable verbose output")
        ("const,c"                  , storeTo(grOpts_.defines, parseConst)->composing()->arg("<id>=<term>"), "Replace term occurences of <id> with <term>")
        ("lparse-rewrite"           , flag(grOpts_.lpRewrite = false), "Use together with --text to inspect lparse rewriting")
        ("lparse-debug"             , storeTo(grOpts_.lparseDebug = Gringo::Output::LparseDebug::NONE, values<Gringo::Output::LparseDebug>()
          ("none"  , Gringo::Output::LparseDebug::NONE)
          ("plain" , Gringo::Output::LparseDebug::PLAIN)
          ("lparse", Gringo::Output::LparseDebug::LPARSE)
          ("all"   , Gringo::Output::LparseDebug::ALL)), "Debug information during lparse rewriting:\n"
         "      none  : no additional info\n"
         "      plain : print rules as in plain output (prefix %%)\n"
         "      lparse: print rules as in lparse output (prefix %%%%)\n"
         "      all   : combines plain and lparse\n")
        ("warn,W"                   , storeTo(grOpts_, parseWarning)->arg("<warn>")->composing(), "Enable/disable warnings:\n"
         "      [no-]atom-undefined:        a :- b.\n"
         "      [no-]file-included:         #include \"a.lp\". #include \"a.lp\".\n"
         "      [no-]operation-undefined:   p(1/0).\n"
         "      [no-]variable-unbounded:    $x > 10.\n"
         "      [no-]global-variable:       :- #count { X } = 1, X = 1.\n")
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
    if (r && finishHandler) { onFinish(r->summary->result); }
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
    solveIter_   = nullptr;
    solveFuture_ = nullptr;
#endif
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
Gringo::Value DefaultGringoModule::parseValue(std::string const &str) { return parser.parse(str); }

// }}}1
