// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#include "clingo/clingocontrol.hh"
#include <gringo/input/programbuilder.hh>
#include "clasp/solver.h"
#include <potassco/program_opts/typed_value.h>
#include <potassco/basic_types.h>
#include "clingo.h"
#include <signal.h>
#include <clingo/script.h>
#include <clingo/incmode.hh>

namespace Gringo {

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

void ClaspAPIBackend::output(Symbol sym, Potassco::Atom_t atom) {
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

void ClaspAPIBackend::output(Symbol sym, Potassco::LitSpan const& condition) {
    std::ostringstream out;
    out << sym;
    if (auto p = prg()) { p->addOutput(Potassco::toSpan(out.str().c_str()), condition); }
}

void ClaspAPIBackend::output(Symbol sym, int value, Potassco::LitSpan const& condition) {
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
ClingoControl::ClingoControl(Scripts &scripts, bool clingoMode, Clasp::ClaspFacade *clasp, Clasp::Cli::ClaspCliConfig &claspConfig, PostGroundFunc pgf, PreSolveFunc psf, Logger::Printer printer, unsigned messageLimit)
: scripts_(scripts)
, clasp_(clasp)
, claspConfig_(claspConfig)
, pgf_(pgf)
, psf_(psf)
, logger_(printer, messageLimit)
, clingoMode_(clingoMode)
, theory_(*this) {
    clasp->ctx.output.theory = &theory_;
}

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
    return Clasp::encodeLit(prg.getLiteral(lit, Clasp::Asp::MapLit_t::Refined));
}

int ClingoPropagateInit::threads() {
    return static_cast<ClingoControl&>(c_).clasp_->ctx.concurrency();
}

void ClingoControl::parse(const StringVec& files, const ClingoOptions& opts, Clasp::Asp::LogicProgram* claspOut, bool addStdIn) {
    using namespace Gringo;
    logger_.enable(Warnings::OperationUndefined, !opts.wNoOperationUndefined);
    logger_.enable(Warnings::AtomUndefined, !opts.wNoAtomUndef);
    logger_.enable(Warnings::VariableUnbounded, !opts.wNoVariableUnbounded);
    logger_.enable(Warnings::FileIncluded, !opts.wNoFileIncluded);
    logger_.enable(Warnings::GlobalVariable, !opts.wNoGlobalVariable);
    logger_.enable(Warnings::Other, !opts.wNoOther);
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
    parser_ = gringo_make_unique<Input::NonGroundParser>(*pb_, incmode_);
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

void ClingoControl::ground(Control::GroundVec const &parts, Context *context) {
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
        Ground::Parameters params;
        for (auto &x : parts) { params.add(x.first, SymVec(x.second)); }
        auto gPrg = prg_.toGround(out_->data, logger_);
        LOG << "*********** intermediate program ***********" << std::endl << gPrg << std::endl;
        LOG << "************* grounded program *************" << std::endl;
        auto exit = onExit([this]{ scripts_.resetContext(); });
        if (context) { scripts_.setContext(*context); }
        gPrg.ground(params, scripts_, *out_, false, logger_);
    }
}

void ClingoControl::main(IClingoApp &app, StringVec const &files, const ClingoOptions& opts, Clasp::Asp::LogicProgram* out) {
    incremental_ = true;
    if (app.has_main()) {
        parse({}, opts, out, false);
        clasp_->enableProgramUpdates();
        app.main(*this, files);
    }
    else {
        parse(files, opts, out);
        if (scripts_.callable("main")) {
            clasp_->enableProgramUpdates();
            scripts_.main(*this);
        }
        else if (incmode_) {
            clasp_->enableProgramUpdates();
            incmode(*this);
        }
        else {
            incremental_ = false;
            claspConfig_.releaseOptions();
            Control::GroundVec parts;
            parts.emplace_back("base", SymVec{});
            ground(parts, nullptr);
            solve({nullptr, 0}, 0, nullptr)->get();
        }
    }
}
bool ClingoControl::onModel(Clasp::Model const &m) {
    if (eventHandler_) {
        std::lock_guard<decltype(propLock_)> lock(propLock_);
        ClingoModel model(*this, &m);
        return eventHandler_->on_model(model);
    }
    return true;
}
void ClingoControl::onFinish(Clasp::ClaspFacade::Result ret) {
    if (eventHandler_) {
        eventHandler_->on_finish(convert(ret));
        eventHandler_ = nullptr;
    }
}
Symbol ClingoControl::getConst(std::string const &name) {
    auto ret = defs_.defs().find(name.c_str());
    if (ret != defs_.defs().end()) {
        bool undefined = false;
        Symbol val = std::get<2>(ret->second)->eval(undefined, logger_);
        if (!undefined) { return val; }
    }
    return Symbol();
}
void ClingoControl::add(std::string const &name, Gringo::StringVec const &params, std::string const &part) {
    Location loc("<block>", 1, 1, "<block>", 1, 1);
    Input::IdVec idVec;
    for (auto &x : params) { idVec.emplace_back(loc, x); }
    parser_->pushBlock(name, std::move(idVec), part, logger_);
    parse();
}
void ClingoControl::load(std::string const &filename) {
    parser_->pushFile(std::string(filename), logger_);
    parse();
}
bool ClingoControl::hasSubKey(unsigned key, char const *name) {
    unsigned subkey = claspConfig_.getKey(key, name);
    return subkey != Clasp::Cli::ClaspCliConfig::KEY_INVALID;
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
ConfigProxy &ClingoControl::getConf() {
    return *this;
}
USolveFuture ClingoControl::solve(Assumptions ass, clingo_solve_mode_bitset_t mode, USolveEventHandler cb) {
    prepare(ass);
    if (clingoMode_) {
        static_assert(clingo_solve_mode_yield == static_cast<clingo_solve_mode_bitset_t>(Clasp::SolveMode_t::Yield), "");
        static_assert(clingo_solve_mode_async == static_cast<clingo_solve_mode_bitset_t>(Clasp::SolveMode_t::Async), "");
        eventHandler_ = std::move(cb);
        return gringo_make_unique<ClingoSolveFuture>(*this, static_cast<Clasp::SolveMode_t>(mode));
    }
    else {
        return gringo_make_unique<DefaultSolveFuture>(std::move(cb));
    }
}
void ClingoControl::interrupt() {
    clasp_->interrupt(65);
}
bool ClingoControl::blocked() {
    return clasp_->solving();
}
void ClingoControl::prepare(Assumptions ass) {
    eventHandler_ = nullptr;
    // finalize the program
    if (update()) {
        // pass assumptions to the backend
        out_->assume(ass);
        out_->endStep(true, logger_);
    }
    grounded = false;
    if (clingoMode_) {
        Clasp::ProgramBuilder *prg = clasp_->program();
        postGround(*prg);
        if (!propagators_.empty()) {
            clasp_->program()->endProgram();
            for (auto&& pp : propagators_) {
                ClingoPropagateInit i(*this, *pp);
                static_cast<Propagator*>(pp->propagator())->init(i);
            }
            propLock_.init(clasp_->ctx.concurrency());
        }
        prePrepare(*clasp_);
        clasp_->prepare(enableEnumAssupmption_ ? Clasp::ClaspFacade::enum_volatile : Clasp::ClaspFacade::enum_static);
        preSolve(*clasp_);
    }
    out_->reset(data_ || (clasp_ && clasp_->program()));
}

void *ClingoControl::claspFacade() {
    return clasp_;
}

ClingoControl::TheoryOutput::TheoryOutput(ClingoControl& ctl) : ctl_(ctl) { }

const char* ClingoControl::TheoryOutput::first(const Clasp::Model& m) {
    last_.clear();
    SymVec symbols_;
    index_ = 1;
    for (auto& prop : ctl_.props_) {
        std::lock_guard<decltype(propLock_)> lock(ctl_.propLock_);
        prop->extend_model(m.sId, false, symbols_);
    }
    std::stringstream ss;
    for (auto& s : symbols_) {
        ss.str("");
        s.print(ss);
        last_.emplace_back(ss.str());
    }
    return last_.size() ? last_.front().c_str() : nullptr;
}

const char* ClingoControl::TheoryOutput::next() {
    return last_.size() > index_ ? last_[index_++].c_str() : nullptr;
}

void ClingoControl::registerPropagator(std::unique_ptr<Propagator> p, bool sequential) {
    propagators_.emplace_back(gringo_make_unique<Clasp::ClingoPropagatorInit>(*p, propLock_.add(sequential)));
    claspConfig_.addConfigurator(propagators_.back().get(), Clasp::Ownership_t::Retain);
    static_cast<Clasp::Asp::LogicProgram*>(clasp_->program())->enableDistinctTrue();
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

void ClingoControl::assignExternal(Potassco::Atom_t ext, Potassco::Value_t val) {
    if (update()) { out_->backend_()->external(ext, val); }
}

bool ClingoControl::isConflicting() noexcept {
    return !clasp_->ok();
}

Potassco::AbstractStatistics *ClingoControl::statistics() {
    return clasp_->getStats();
}

void ClingoControl::addStatisticsCallback(clingo_set_user_statistics cb, void* data) {
    clasp_->addStatisticsCallback(reinterpret_cast<void(*)(Clasp::ClaspStatistics*, void*)>(cb), data);
}

void ClingoControl::useEnumAssumption(bool enable) {
    enableEnumAssupmption_ = enable;
}
bool ClingoControl::useEnumAssumption() {
    return enableEnumAssupmption_;
}

SymbolicAtoms &ClingoControl::getDomain() {
    return *this;
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

bool skipDomain(Sig sig)                                          { return sig.name().startsWith("#"); }

SymbolicAtomIter init(Output::OutputBase &out, uint32_t domainOffset, bool advance) {
    SymbolicAtomOffset it(domainOffset, advance, 0, false);
    for (auto domIt = out.predDoms().begin() + domainOffset, domIe = out.predDoms().end(); domIt != domIe; ++domIt, ++it.data.domain_offset) {
        auto &dom = **domIt;
        if (!skipDomain(dom) && dom.size() > 0) { return it.repr; }
        if (!it.data.domain_advance) { break; }
    }
    it.data.domain_offset = out.predDoms().size();
    return it.repr;
}

void advance(Output::OutputBase &out, SymbolicAtomIter &it) {
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

Output::PredicateAtom &domainElem(Output::PredDomMap &map, SymbolicAtomIter it) {
    auto &off = toOffset(it).data;
    return (*map[off.domain_offset])[off.atom_offset];
}

} // namespace

Symbol ClingoControl::atom(SymbolicAtomIter it) const {
    return domainElem(out_->predDoms(), it);
}

Potassco::Lit_t ClingoControl::literal(SymbolicAtomIter it) const {
    auto &elem = domainElem(out_->predDoms(), it);
    return elem.hasUid() ? elem.uid() : 0;
}

bool ClingoControl::fact(SymbolicAtomIter it) const {
    return domainElem(out_->predDoms(), it).fact();
}

bool ClingoControl::external(SymbolicAtomIter it) const {
    auto &elem = domainElem(out_->predDoms(), it);
    return elem.hasUid() && elem.isExternal() && (!clingoMode_ || static_cast<Clasp::Asp::LogicProgram*>(clasp_->program())->isExternal(elem.uid()));
}

SymbolicAtomIter ClingoControl::next(SymbolicAtomIter it) {
    advance(*out_, it);
    return it;
}

bool ClingoControl::valid(SymbolicAtomIter it) const {
    auto &off = toOffset(it).data;
    return off.domain_offset < out_->predDoms().size() && off.atom_offset < out_->predDoms()[off.domain_offset]->size();
}

std::vector<Sig> ClingoControl::signatures() const {
    std::vector<Sig> ret;
    for (auto &dom : out_->predDoms()) {
        if (!skipDomain(*dom)) { ret.emplace_back(*dom); }
    }
    return ret;
}

SymbolicAtomIter ClingoControl::begin(Sig sig) const {
    return init(*out_, out_->predDoms().offset(out_->predDoms().find(sig)), false);
}

SymbolicAtomIter ClingoControl::begin() const {
    return init(*out_, 0, true);
}

SymbolicAtomIter ClingoControl::end() const {
    return SymbolicAtomOffset{out_->predDoms().size(), false, 0, false}.repr;
}

bool ClingoControl::eq(SymbolicAtomIter it, SymbolicAtomIter jt) const {
    return toOffset(it) == toOffset(jt);
}

SymbolicAtomIter ClingoControl::lookup(Symbol atom) const {
    if (atom.hasSig()) {
        auto it = out_->predDoms().find(atom.sig());
        if (it != out_->predDoms().end()) {
            auto jt = (*it)->find(atom);
            if (jt != (*it)->end()) {
                return SymbolicAtomOffset(out_->predDoms().offset(it), true, numeric_cast<uint32_t>(jt - (*it)->begin()), true).repr;
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

bool ClingoControl::beginAddBackend() {
    backend_ = out_->backend(logger());
    return backend_ != nullptr;
}

Id_t ClingoControl::addAtom(Symbol sym) {
    return out_->addAtom(sym);
}

void ClingoControl::endAddBackend() {
    out_->flush();
}

Potassco::Atom_t ClingoControl::addProgramAtom() { return out_->data.newAtom(); }

ClingoControl::~ClingoControl() noexcept = default;

// {{{1 definition of ClingoSolveFuture

SolveResult convert(Clasp::ClaspFacade::Result res) {
    if (res.interrupted() && res.signal != 0 && res.signal != 65) {
        throw std::runtime_error("solving stopped by signal");
    }
    SolveResult::Satisfiabily sat = SolveResult::Satisfiable;
    switch (res) {
        case Clasp::ClaspFacade::Result::SAT:     { sat = SolveResult::Satisfiable; break; }
        case Clasp::ClaspFacade::Result::UNSAT:   { sat = SolveResult::Unsatisfiable; break; }
        case Clasp::ClaspFacade::Result::UNKNOWN: { sat = SolveResult::Unknown; break; }
    }
    return {sat, res.exhausted(), res.interrupted()};
}

ClingoSolveFuture::ClingoSolveFuture(ClingoControl &ctl, Clasp::SolveMode_t mode)
: model_(ctl)
, handle_(model_.context().clasp_->solve(mode)) {
}
SolveResult ClingoSolveFuture::get() {
    return convert(handle_.get());
}
Model const *ClingoSolveFuture::model() {
    if (auto m = handle_.model()) {
        model_.reset(*m);
        return &model_;
    }
    else { return nullptr; }
}
bool ClingoSolveFuture::wait(double timeout) {
    if (timeout == 0)      { return handle_.ready(); }
    else if (timeout < 0)  { return handle_.wait(), true; }
    else                   { return handle_.waitFor(timeout); }
}
void ClingoSolveFuture::cancel() {
    handle_.cancel();
}
void ClingoSolveFuture::resume() {
    handle_.resume();
}

// {{{1 definition of ClingoLib

ClingoLib::ClingoLib(Scripts &scripts, int argc, char const * const *argv, Logger::Printer printer, unsigned messageLimit)
        : ClingoControl(scripts, true, &clasp_, claspConfig_, nullptr, nullptr, printer, messageLimit) {
    using namespace Potassco::ProgramOptions;
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

void ClingoLib::initOptions(Potassco::ProgramOptions::OptionContext& root) {
    using namespace Potassco::ProgramOptions;
    grOpts_.defines.clear();
    grOpts_.verbose = false;
    OptionGroup gringo("Gringo Options");
    gringo.addOptions()
        ("verbose,V"                , flag(grOpts_.verbose = false), "Enable verbose output")
        ("const,c"                  , storeTo(grOpts_.defines, parseConst)->composing()->arg("<id>=<term>"), "Replace term occurrences of <id> with <term>")
        ("output-debug", storeTo(grOpts_.outputOptions.debug = Output::OutputDebug::NONE, values<Output::OutputDebug>()
          ("none", Output::OutputDebug::NONE)
          ("text", Output::OutputDebug::TEXT)
          ("translate", Output::OutputDebug::TRANSLATE)
          ("all", Output::OutputDebug::ALL)), "Print debug information during output:\n"
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
    Clasp::ClaspFacade::StepReady const *r = Clasp::event_cast<Clasp::ClaspFacade::StepReady>(ev);
    if (r) { onFinish(r->summary->result); }
    const Clasp::LogEvent* log = Clasp::event_cast<Clasp::LogEvent>(ev);
    if (log && log->isWarning()) { logger_.print(Warnings::Other, log->msg); }
}
bool ClingoLib::parsePositional(const std::string& t, std::string& out) {
    int num;
    if (Potassco::string_cast(t, num)) {
        out = "number";
        return true;
    }
    return false;
}
ClingoLib::~ClingoLib() {
    clasp_.shutdown();
}

// }}}1

} // namespace Gringo
