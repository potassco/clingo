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

#ifndef _GRINGO_CLINGOCONTROL_HH
#define _GRINGO_CLINGOCONTROL_HH

#include <gringo/output/output.hh>
#include <gringo/input/program.hh>
#include <gringo/input/programbuilder.hh>
#include <gringo/input/nongroundparser.hh>
#include <gringo/input/groundtermparser.hh>
#include <gringo/control.hh>
#include <gringo/logger.hh>
#include <gringo/scripts.hh>
#include <clasp/logic_program.h>
#include <clasp/clasp_facade.h>
#include <clasp/clingo.h>
#include <clasp/cli/clasp_options.h>
#include <potassco/application.h>
#include <potassco/string_convert.h>
#include <mutex>

// {{{1 declaration of ClaspAPIBackend

using StringVec = std::vector<std::string>;

class ClingoControl;
class ClaspAPIBackend : public Gringo::Backend {
public:
    ClaspAPIBackend(ClingoControl& ctl) : ctl_(ctl) { }
    ClaspAPIBackend(const ClaspAPIBackend&) = delete;
    ClaspAPIBackend& operator=(const ClaspAPIBackend&) = delete;
    void initProgram(bool incremental) override;
    void beginStep() override;
    void rule(Potassco::Head_t ht, const Potassco::AtomSpan& head, const Potassco::LitSpan& body) override;
    void rule(Potassco::Head_t ht, const Potassco::AtomSpan& head, Potassco::Weight_t bound, const Potassco::WeightLitSpan& body) override;
    void minimize(Potassco::Weight_t prio, const Potassco::WeightLitSpan& lits) override;
    void project(const Potassco::AtomSpan& atoms) override;
    void output(Gringo::Symbol sym, Potassco::Atom_t atom) override;
    void output(Gringo::Symbol sym, Potassco::LitSpan const& condition) override;
    void output(Gringo::Symbol sym, int value, Potassco::LitSpan const& condition) override;
    void external(Potassco::Atom_t a, Potassco::Value_t v) override;
    void assume(const Potassco::LitSpan& lits) override;
    void heuristic(Potassco::Atom_t a, Potassco::Heuristic_t t, int bias, unsigned prio, const Potassco::LitSpan& condition) override;
    void acycEdge(int s, int t, const Potassco::LitSpan& condition) override;
    void theoryTerm(Potassco::Id_t termId, int number) override;
    void theoryTerm(Potassco::Id_t termId, const Potassco::StringSpan& name) override;
    void theoryTerm(Potassco::Id_t termId, int cId, const Potassco::IdSpan& args) override;
    void theoryElement(Potassco::Id_t elementId, const Potassco::IdSpan& terms, const Potassco::LitSpan& cond) override;
    void theoryAtom(Potassco::Id_t atomOrZero, Potassco::Id_t termId, const Potassco::IdSpan& elements) override;
    void theoryAtom(Potassco::Id_t atomOrZero, Potassco::Id_t termId, const Potassco::IdSpan& elements, Potassco::Id_t op, Potassco::Id_t rhs) override;
    void endStep() override;
    ~ClaspAPIBackend() noexcept override;
private:
    Clasp::Asp::LogicProgram *prg();
    ClingoControl& ctl_;
};

// {{{1 declaration of ClingoOptions

struct ClingoOptions {
    using Foobar = std::vector<Gringo::Sig>;
    StringVec                     defines;
    Gringo::Output::OutputOptions outputOptions;
    Gringo::Output::OutputFormat  outputFormat          = Gringo::Output::OutputFormat::INTERMEDIATE;
    bool                          verbose               = false;
    bool                          wNoOperationUndefined = false;
    bool                          wNoAtomUndef          = false;
    bool                          wNoFileIncluded       = false;
    bool                          wNoVariableUnbounded  = false;
    bool                          wNoGlobalVariable     = false;
    bool                          wNoOther              = false;
    bool                          rewriteMinimize       = false;
    bool                          keepFacts             = false;
    Foobar                        foobar;
};

inline void enableAll(ClingoOptions& out, bool enable) {
    out.wNoAtomUndef          = !enable;
    out.wNoFileIncluded       = !enable;
    out.wNoOperationUndefined = !enable;
    out.wNoVariableUnbounded  = !enable;
    out.wNoGlobalVariable     = !enable;
    out.wNoOther              = !enable;
}

inline bool parseWarning(const std::string& str, ClingoOptions& out) {
    if (str == "none")                     { enableAll(out, false);             return true; }
    if (str == "all")                      { enableAll(out, true);              return true; }
    if (str == "no-atom-undefined")        { out.wNoAtomUndef          = true;  return true; }
    if (str ==    "atom-undefined")        { out.wNoAtomUndef          = false; return true; }
    if (str == "no-file-included")         { out.wNoFileIncluded       = true;  return true; }
    if (str ==    "file-included")         { out.wNoFileIncluded       = false; return true; }
    if (str == "no-operation-undefined")   { out.wNoOperationUndefined = true;  return true; }
    if (str ==    "operation-undefined")   { out.wNoOperationUndefined = false; return true; }
    if (str == "no-variable-unbounded")    { out.wNoVariableUnbounded  = true;  return true; }
    if (str ==    "variable-unbounded")    { out.wNoVariableUnbounded  = false; return true; }
    if (str == "no-global-variable")       { out.wNoGlobalVariable     = true;  return true; }
    if (str ==    "global-variable")       { out.wNoGlobalVariable     = false; return true; }
    if (str == "no-other")                 { out.wNoOther              = true;  return true; }
    if (str ==    "other")                 { out.wNoOther              = false; return true; }
    return false;
}

inline std::vector<std::string> split(std::string const &source, char const *delimiter = " ", bool keepEmpty = false) {
    std::vector<std::string> results;
    size_t prev = 0;
    size_t next = 0;
    while ((next = source.find_first_of(delimiter, prev)) != std::string::npos) {
        if (keepEmpty || (next - prev != 0)) { results.push_back(source.substr(prev, next - prev)); }
        prev = next + 1;
    }
    if (prev < source.size()) { results.push_back(source.substr(prev)); }
    return results;
}

static inline bool parseFoobar(const std::string& str, ClingoOptions::Foobar& foobar) {
    for (auto &x : split(str, ",")) {
        auto y = split(x, "/");
        if (y.size() != 2) { return false; }
        unsigned a;
        if (!Potassco::string_cast<unsigned>(y[1], a)) { return false; }
        bool sign = !y[0].empty() && y[0][0] == '-';
        if (sign) { y[0] = y[0].substr(1); }
        foobar.emplace_back(y[0].c_str(), a, sign);
    }
    return true;
}

// {{{1 declaration of ClingoSolveFuture

Gringo::SolveResult convert(Clasp::ClaspFacade::Result res);
#if WITH_THREADS
struct ClingoSolveFuture : Gringo::SolveFuture {
    ClingoSolveFuture(Clasp::ClaspFacade::AsyncResult const &res);
    // async
    Gringo::SolveResult get() override;
    void wait() override;
    bool wait(double timeout) override;
    void cancel() override;

    void reset(Clasp::ClaspFacade::AsyncResult res);

    Clasp::ClaspFacade::AsyncResult future;
    Gringo::SolveResult             ret = {Gringo::SolveResult::Unknown, false, false};
    bool                            done = false;
};
#endif

// {{{1 declaration of ClingoControl
class ClingoPropagateInit : public Gringo::PropagateInit {
public:
    using Lit_t = Potassco::Lit_t;
    ClingoPropagateInit(Gringo::Control &c, Clasp::ClingoPropagatorInit &p) : c_(c), p_(p) { }
    Gringo::TheoryData const &theory() const override { return c_.theory(); }
    Gringo::SymbolicAtoms &getDomain() override { return c_.getDomain(); }
    Lit_t mapLit(Lit_t lit) override;
    int threads() override;
    void addWatch(Lit_t lit) override { p_.addWatch(Clasp::decodeLit(lit)); }
private:
    Gringo::Control &c_;
    Clasp::ClingoPropagatorInit &p_;
};

class ClingoPropagatorLock : public Clasp::ClingoPropagatorLock {
public:
    ClingoPropagatorLock() : seq_(0) {}
    void lock()   override { if (mut_) mut_->lock(); }
    void unlock() override { if (mut_) mut_->unlock(); }
    ClingoPropagatorLock* add(bool seq) {
        if (seq) { ++seq_; return this; }
        return 0;
    }
    void init(unsigned nThreads) {
        if      (nThreads < 2 || !seq_) { mut_ = nullptr; }
        else if (!mut_.get())           { mut_ = Gringo::gringo_make_unique<std::mutex>(); }
    }
private:
    using OptLock = std::unique_ptr<std::mutex>;
    OptLock  mut_;
    unsigned seq_;
};

class ClingoSolveIter;
class ClingoControl : public clingo_control, private Gringo::ConfigProxy, private Gringo::SymbolicAtoms {
public:
    using StringVec        = std::vector<std::string>;
    using ExternalVec      = std::vector<std::pair<Gringo::Symbol, Potassco::Value_t>>;
    using PostGroundFunc   = std::function<bool (Clasp::ProgramBuilder &)>;
    using PreSolveFunc     = std::function<bool (Clasp::ClaspFacade &)>;
    enum class ConfigUpdate { KEEP, REPLACE };

    ClingoControl(Gringo::Scripts &scripts, bool clingoMode, Clasp::ClaspFacade *clasp, Clasp::Cli::ClaspCliConfig &claspConfig, PostGroundFunc pgf, PreSolveFunc psf, Gringo::Logger::Printer printer, unsigned messageLimit);
    ~ClingoControl() noexcept override;
    void prepare(Assumptions &&ass, Gringo::Control::ModelHandler mh, Gringo::Control::FinishHandler fh);
    void commitExternals();
    void parse();
    void parse(const StringVec& files, const ClingoOptions& opts, Clasp::Asp::LogicProgram* out, bool addStdIn = true);
    void main();
    bool onModel(Clasp::Model const &m);
    void onFinish(Clasp::ClaspFacade::Result ret);
    bool update();

    virtual void postGround(Clasp::ProgramBuilder& prg) { if (pgf_) { pgf_(prg); } }
    virtual void prePrepare(Clasp::ClaspFacade& ) { }
    virtual void preSolve(Clasp::ClaspFacade& clasp) { if (psf_) { psf_(clasp);} }
    virtual void postSolve(Clasp::ClaspFacade& ) { }
    virtual void addToModel(Clasp::Model const&, bool /*complement*/, Gringo::SymVec& ) { }

    // {{{2 SymbolicAtoms interface

    Gringo::SymbolicAtomIter begin(Gringo::Sig sig) const override;
    Gringo::SymbolicAtomIter begin() const override;
    Gringo::SymbolicAtomIter end() const override;
    bool eq(Gringo::SymbolicAtomIter it, Gringo::SymbolicAtomIter jt) const override;
    Gringo::SymbolicAtomIter lookup(Gringo::Symbol atom) const override;
    size_t length() const override;
    std::vector<Gringo::Sig> signatures() const override;
    Gringo::Symbol atom(Gringo::SymbolicAtomIter it) const override;
    Potassco::Lit_t literal(Gringo::SymbolicAtomIter it) const override;
    bool fact(Gringo::SymbolicAtomIter it) const override;
    bool external(Gringo::SymbolicAtomIter it) const override;
    Gringo::SymbolicAtomIter next(Gringo::SymbolicAtomIter it) override;
    bool valid(Gringo::SymbolicAtomIter it) const override;

    // {{{2 ConfigProxy interface

    bool hasSubKey(unsigned key, char const *name, unsigned* subKey = nullptr) override;
    unsigned getSubKey(unsigned key, char const *name) override;
    unsigned getArrKey(unsigned key, unsigned idx) override;
    void getKeyInfo(unsigned key, int* nSubkeys = 0, int* arrLen = 0, const char** help = 0, int* nValues = 0) const override;
    const char* getSubKeyName(unsigned key, unsigned idx) const override;
    bool getKeyValue(unsigned key, std::string &value) override;
    void setKeyValue(unsigned key, const char *val) override;
    unsigned getRootKey() override;

    // {{{2 Control interface

    Gringo::SymbolicAtoms &getDomain() override;
    void ground(Gringo::Control::GroundVec const &vec, Gringo::Context *ctx) override;
    void add(std::string const &name, Gringo::FWStringVec const &params, std::string const &part) override;
    void load(std::string const &filename) override;
    Gringo::SolveResult solve(ModelHandler h, Assumptions &&ass) override;
    bool blocked() override;
    std::string str();
    void assignExternal(Gringo::Symbol ext, Potassco::Value_t) override;
    Gringo::Symbol getConst(std::string const &name) override;
    Potassco::AbstractStatistics *statistics() override;
    Gringo::ConfigProxy &getConf() override;
    void useEnumAssumption(bool enable) override;
    bool useEnumAssumption() override;
    void cleanupDomains() override;
    Gringo::SolveIter *solveIter(Assumptions &&ass) override;
    Gringo::SolveFuture *solveAsync(ModelHandler mh, FinishHandler fh, Assumptions &&ass) override;
    Gringo::TheoryData const &theory() const override { return out_->data.theoryInterface(); }
    void registerPropagator(Gringo::UProp p, bool sequential) override;
    void interrupt() override;
    void *claspFacade() override;
    Gringo::Backend *backend() override;
    Potassco::Atom_t addProgramAtom() override;
    Gringo::Logger &logger() override { return logger_; }
    void beginAdd() override { parse(); }
    void add(clingo_ast_statement_t const &stm) override { Gringo::Input::parseStatement(*pb_, logger_, stm); }
    void endAdd() override { defs_.init(logger_); parsed = true; }
    void registerObserver(Gringo::UBackend obs, bool replace) override {
        if (replace) { clingoMode_ = false; }
        out_->registerObserver(std::move(obs), replace);
    }

    // }}}2

    std::unique_ptr<Gringo::Output::OutputBase>               out_;
    Gringo::Scripts                                          &scripts_;
    Gringo::Input::Program                                    prg_;
    Gringo::Defines                                           defs_;
    std::unique_ptr<Gringo::Input::NongroundProgramBuilder>   pb_;
    std::unique_ptr<Gringo::Input::NonGroundParser>           parser_;
    ModelHandler                                              modelHandler_;
    FinishHandler                                             finishHandler_;
    Clasp::ClaspFacade                                       *clasp_ = nullptr;
    Clasp::Cli::ClaspCliConfig                               &claspConfig_;
    PostGroundFunc                                            pgf_;
    PreSolveFunc                                              psf_;
    std::unique_ptr<Potassco::TheoryData>                     data_;
    std::vector<Gringo::UProp>                                props_;
    std::vector<std::unique_ptr<Clasp::ClingoPropagatorInit>> propagators_;
    ClingoPropagatorLock                                      propLock_;
    Gringo::Logger                                            logger_;
#if WITH_THREADS
    std::unique_ptr<ClingoSolveFuture> solveFuture_;
#endif
    std::unique_ptr<ClingoSolveIter>   solveIter_;
    bool enableEnumAssupmption_ = true;
    bool clingoMode_;
    bool verbose_               = false;
    bool parsed                 = false;
    bool grounded               = false;
    bool incremental_           = true;
    bool configUpdate_          = false;
    bool initialized_           = false;
};

// {{{1 declaration of ClingoModel

class ClingoModel : public Gringo::Model {
public:
    ClingoModel(ClingoControl &ctl, Clasp::Model const *model = nullptr)
    : ctl_(ctl), model_(model) { }
    void reset(Clasp::Model const &m) { model_ = &m; }
    bool contains(Gringo::Symbol atom) const override {
        auto atm = out().find(atom);
        return atm.second && atm.first->hasUid() && model_->isTrue(lp().getLiteral(atm.first->uid()));
    }
    Gringo::SymSpan atoms(unsigned atomset) const override {
        atms_ = out().atoms(atomset, [this](unsigned uid) { return model_->isTrue(lp().getLiteral(uid)); });
        if (atomset & clingo_show_type_extra){
            ctl_.addToModel(*model_, (atomset & clingo_show_type_complement) != 0, atms_);
        }
        return Potassco::toSpan(atms_);
    }
    Gringo::Int64Vec optimization() const override {
        return model_->costs ? Gringo::Int64Vec(model_->costs->begin(), model_->costs->end()) : Gringo::Int64Vec();
    }
    void addClause(LitVec const &lits) const override {
        Clasp::LitVec claspLits;
        for (auto &x : lits) {
            auto atom = out().find(x.first);
            if (atom.second && atom.first->hasUid()) {
                Clasp::Literal lit = lp().getLiteral(atom.first->uid());
                claspLits.push_back(x.second ? lit : ~lit);
            }
            else if (!x.second) { return; }
        }
        claspLits.push_back(~ctx().stepLiteral());
        model_->ctx->commitClause(claspLits);
    }
    Gringo::ModelType type() const override {
        if (model_->type & Clasp::Model::Brave) { return Gringo::ModelType::BraveConsequences; }
        if (model_->type & Clasp::Model::Cautious) { return Gringo::ModelType::CautiousConsequences; }
        return Gringo::ModelType::StableModel;
    }
    uint64_t number() const override { return model_->num; }
    Potassco::Id_t threadId() const override { return model_->sId; }
    bool optimality_proven() const override { return model_->opt; }
private:
    Clasp::Asp::LogicProgram const &lp() const    { return *static_cast<Clasp::Asp::LogicProgram*>(ctl_.clasp_->program()); };
    Gringo::Output::OutputBase const &out() const { return *ctl_.out_; };
    Clasp::SharedContext const &ctx() const       { return ctl_.clasp_->ctx; };
    ClingoControl          &ctl_;
    Clasp::Model const     *model_;
    mutable Gringo::SymVec  atms_;
};

// {{{1 declaration of ClingoSolveIter

class ClingoSolveIter : public Gringo::SolveIter {
public:
    ClingoSolveIter(ClingoControl &ctl);

    Gringo::Model const *next() override;
    void close() override;
    Gringo::SolveResult get() override;

private:
    Clasp::ClaspFacade::ModelGenerator future_;
    ClingoModel                        model_;
};

// {{{1 declaration of ClingoLib

class ClingoLib : public Clasp::EventHandler, public ClingoControl {
public:
    ClingoLib(Gringo::Scripts &scripts, int argc, char const * const *argv, Gringo::Logger::Printer printer, unsigned messageLimit);
    ~ClingoLib() override;
protected:
    void initOptions(Potassco::ProgramOptions::OptionContext& root);
    static bool parsePositional(const std::string& s, std::string& out);
    // -------------------------------------------------------------------------------------------
    // Event handler
    void onEvent(const Clasp::Event& ev) override;
    bool onModel(const Clasp::Solver& s, const Clasp::Model& m) override;
private:
    ClingoLib(const ClingoLib&);
    ClingoLib& operator=(const ClingoLib&);
    ClingoOptions                       grOpts_;
    Clasp::Cli::ClaspCliConfig          claspConfig_;
    Clasp::ClaspFacade                  clasp_;
};

// {{{1 declaration of DefaultGringoModule

struct DefaultGringoModule : Gringo::GringoModule {
    DefaultGringoModule();
    Gringo::Control *newControl(int argc, char const * const *argv, Gringo::Logger::Printer printer, unsigned messageLimit) override;
    Gringo::Symbol parseValue(std::string const &str, Gringo::Logger::Printer printer, unsigned messageLimit) override;
    Gringo::Input::GroundTermParser parser;
    Gringo::Scripts scripts;
};

// }}}1

#endif // _GRINGO_CLINGOCONTROL_HH
