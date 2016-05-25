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
#include <program_opts/application.h>
#include <program_opts/string_convert.h>
#include <mutex>

// {{{1 declaration of ClaspAPIBackend

class ClaspAPIBackend : public Gringo::Backend {
public:
    ClaspAPIBackend(Potassco::TheoryData &data, Clasp::Asp::LogicProgram& out) : data_(data), prg_(out) { }
    void init(bool incremental) override;
    void beginStep() override;
    void printTheoryAtom(Potassco::TheoryAtom const &atom, GetCond getCond) override;
    void printHead(bool choice, AtomVec const &atoms) override;
    void printNormalBody(LitVec const &body) override;
    void printWeightBody(Potassco::Weight_t lower, LitWeightVec const &body) override;
    void printProject(AtomVec const &lits) override;
    void printOutput(char const *symbol, LitVec const &body) override;
    void printEdge(unsigned u, unsigned v, LitVec const &body) override;
    void printHeuristic(Potassco::Heuristic_t modifier, Potassco::Atom_t atom, int value, unsigned priority, LitVec const &body) override;
    void printExternal(Potassco::Atom_t atom, Potassco::Value_t value) override;
    void printAssume(LitVec const &lits) override;
    void printMinimize(int priority, LitWeightVec const &body) override;
    void endStep() override;
    ~ClaspAPIBackend() noexcept;

private:
    void addBody(const LitVec& body);
    void addBody(const LitWeightVec& body);
    void updateConditions(Potassco::IdSpan const& elems, GetCond getCond);
    ClaspAPIBackend(const ClaspAPIBackend&);
    ClaspAPIBackend& operator=(const ClaspAPIBackend&);
    Potassco::TheoryData &data_;
    Clasp::Asp::LogicProgram& prg_;
    Clasp::Asp::HeadData head_;
    Clasp::Asp::BodyData body_;
    std::stringstream str_;
};

// {{{1 declaration of ClingoOptions

struct ClingoOptions {
    using Foobar = std::vector<Gringo::Sig>;
    ProgramOptions::StringSeq    defines;
    Gringo::Output::OutputDebug  outputDebug  = Gringo::Output::OutputDebug::NONE;
    Gringo::Output::OutputFormat outputFormat = Gringo::Output::OutputFormat::INTERMEDIATE;
    bool verbose               = false;
    bool wNoOperationUndefined = false;
    bool wNoAtomUndef          = false;
    bool wNoFileIncluded       = false;
    bool wNoVariableUnbounded  = false;
    bool wNoGlobalVariable     = false;
    bool rewriteMinimize       = false;
    bool keepFacts             = false;
    Foobar foobar;
};

inline bool parseWarning(const std::string& str, ClingoOptions& out) {
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
        if (!bk_lib::string_cast<unsigned>(y[1], a)) { return false; }
        bool sign = !y[0].empty() && y[0][0] == '-';
        if (sign) { y[0] = y[0].substr(1); }
        foobar.emplace_back(y[0].c_str(), a, sign);
    }
    return true;
}

// {{{1 declaration of ClingoStatistics

struct ClingoStatistics : Gringo::Statistics {
    Quantity    getStat(char const* key) const override;
    char const *getKeys(char const* key) const override;
    virtual ~ClingoStatistics() { }

    Clasp::ClaspFacade *clasp = nullptr;
};

// {{{1 declaration of ClingoModel

struct ClingoModel : Gringo::Model {
    ClingoModel(Clasp::Asp::LogicProgram const &lp, Gringo::Output::OutputBase const &out, Clasp::SharedContext const &ctx, Clasp::Model const *model = nullptr)
        : lp(lp)
        , out(out)
        , ctx(ctx)
        , model(model) { }
    void reset(Clasp::Model const &m) { model = &m; }
    virtual bool contains(Gringo::Symbol atom) const {
        auto atm = out.find(atom);
        return atm.second && atm.first->hasUid() && model->isTrue(lp.getLiteral(atm.first->uid()));
    }
    virtual Gringo::SymSpan atoms(int atomset) const {
        atms = out.atoms(atomset, [this, atomset](unsigned uid) -> bool { return bool(atomset & COMP) ^ model->isTrue(lp.getLiteral(uid)); });
        return Potassco::toSpan(atms);
    }
    virtual Gringo::Int64Vec optimization() const {
        return model->costs ? Gringo::Int64Vec(model->costs->begin(), model->costs->end()) : Gringo::Int64Vec();
    }
    virtual void addClause(LitVec const &lits) const {
        Clasp::LitVec claspLits;
        for (auto &x : lits) {
            auto atom = out.find(x.first);
            if (atom.second && atom.first->hasUid()) {
                Clasp::Literal lit = lp.getLiteral(atom.first->uid());
                claspLits.push_back(x.second ? lit : ~lit);
            }
            else if (!x.second) { return; }
        }
        claspLits.push_back(~ctx.stepLiteral());
        model->ctx->commitClause(claspLits);
    }
    virtual ~ClingoModel() { }
    Clasp::Asp::LogicProgram const   &lp;
    Gringo::Output::OutputBase const &out;
    Clasp::SharedContext const       &ctx;
    Clasp::Model const               *model;
    mutable Gringo::SymVec            atms;
};

// {{{1 declaration of ClingoSolveIter

struct ClingoSolveIter : Gringo::SolveIter {
    ClingoSolveIter(Clasp::ClaspFacade::ModelGenerator const &future, Clasp::Asp::LogicProgram const &lp, Gringo::Output::OutputBase const &out, Clasp::SharedContext const &ctx);

    Gringo::Model const *next() override;
    void close() override;
    Gringo::SolveResult get() override;

    virtual ~ClingoSolveIter();

    Clasp::ClaspFacade::ModelGenerator future;
    ClingoModel                        model;
};

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

    virtual ~ClingoSolveFuture();
    void reset(Clasp::ClaspFacade::AsyncResult res);

    Clasp::ClaspFacade::AsyncResult future;
    Gringo::SolveResult             ret = {Gringo::SolveResult::Unknown, false, false};
    bool                            done = false;
};
#endif

// {{{1 declaration of ClingoControl
class ClingoPropagateInit : public Gringo::Propagator::Init {
public:
    using Lit_t = Potassco::Lit_t;
    ClingoPropagateInit(Gringo::Control &c, Clasp::ClingoPropagatorInit &p) : c_(c), p_(p) { }
    Gringo::TheoryData const &theory() const override { return c_.theory(); }
    Gringo::DomainProxy &getDomain() override { return c_.getDomain(); }
    Lit_t mapLit(Lit_t lit) override;
    int threads() override;
    void addWatch(Lit_t lit) override { p_.addWatch(Clasp::decodeLit(lit)); }
    virtual ~ClingoPropagateInit() noexcept = default;
private:
    Gringo::Control &c_;
    Clasp::ClingoPropagatorInit &p_;
};

class ClingoPropagatorLock : public Clasp::ClingoPropagatorLock {
public:
    ClingoPropagatorLock() : seq_(0) {}
    virtual void lock()   override { if (mut_) mut_->lock(); }
    virtual void unlock() override { if (mut_) mut_->unlock(); }
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

class ClingoControl : public Gringo::Control, private Gringo::ConfigProxy, private Gringo::DomainProxy {
public:
    using StringVec        = std::vector<std::string>;
    using ExternalVec      = std::vector<std::pair<Gringo::Symbol, Potassco::Value_t>>;
    using StringSeq        = ProgramOptions::StringSeq;
    using PostGroundFunc   = std::function<bool (Clasp::ProgramBuilder &)>;
    using PreSolveFunc     = std::function<bool (Clasp::ClaspFacade &)>;
    enum class ConfigUpdate { KEEP, REPLACE };

    ClingoControl(Gringo::Scripts &scripts, bool clingoMode, Clasp::ClaspFacade *clasp, Clasp::Cli::ClaspCliConfig &claspConfig, PostGroundFunc pgf, PreSolveFunc psf, Gringo::MessagePrinter &log);
    void prepare(Gringo::Control::ModelHandler mh, Gringo::Control::FinishHandler fh);
    void commitExternals();
    void parse();
    void parse(const StringSeq& files, const ClingoOptions& opts, Clasp::Asp::LogicProgram* out, bool addStdIn = true);
    void main();
    bool onModel(Clasp::Model const &m);
    void onFinish(Clasp::ClaspFacade::Result ret);
    bool update();

    Clasp::LitVec toClaspAssumptions(Gringo::Control::Assumptions &&ass) const;

    // {{{2 DomainProxy interface

    ElementPtr iter(Gringo::Sig sig) const override;
    ElementPtr iter() const override;
    ElementPtr lookup(Gringo::Symbol atom) const override;
    size_t length() const override;
    std::vector<Gringo::Sig> signatures() const override;

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

    Gringo::DomainProxy &getDomain() override;
    void ground(Gringo::Control::GroundVec const &vec, Gringo::Context *ctx) override;
    void add(std::string const &name, Gringo::FWStringVec const &params, std::string const &part) override;
    void load(std::string const &filename) override;
    Gringo::SolveResult solve(ModelHandler h, Assumptions &&ass) override;
    bool blocked() override;
    std::string str();
    void assignExternal(Gringo::Symbol ext, Potassco::Value_t) override;
    Gringo::Symbol getConst(std::string const &name) override;
    ClingoStatistics *getStats() override;
    Gringo::ConfigProxy &getConf() override;
    void useEnumAssumption(bool enable) override;
    bool useEnumAssumption() override;
    void cleanupDomains() override;
    Gringo::SolveIter *solveIter(Assumptions &&ass) override;
    Gringo::SolveFuture *solveAsync(ModelHandler mh, FinishHandler fh, Assumptions &&ass) override;
    virtual Gringo::TheoryData const &theory() const override { return out_->data.theoryInterface(); }
    void registerPropagator(Gringo::Propagator &p, bool sequential) override;
    void parse(char const *program, std::function<void(clingo_ast const &)> cb) override;
    void add(std::function<void (std::function<void (clingo_ast const &)>)> cb) override;
    void interrupt() override;
    Gringo::Backend *backend() override;
    Potassco::Atom_t addProgramAtom() override;
    Gringo::MessagePrinter &logger() override { return logger_; }

    // }}}2

    std::unique_ptr<Gringo::Output::OutputBase>               out_;
    Gringo::Scripts                                          &scripts_;
    Gringo::Input::Program                                    prg_;
    Gringo::Defines                                           defs_;
    std::unique_ptr<Gringo::Input::NongroundProgramBuilder>   pb_;
    std::unique_ptr<Gringo::Input::NonGroundParser>           parser_;
    ModelHandler                                              modelHandler_;
    FinishHandler                                             finishHandler_;
    ClingoStatistics                                          clingoStats_;
    Clasp::ClaspFacade                                       *clasp_ = nullptr;
    Clasp::Cli::ClaspCliConfig                               &claspConfig_;
    PostGroundFunc                                            pgf_;
    PreSolveFunc                                              psf_;
    std::unique_ptr<Potassco::TheoryData>                     data_;
    std::vector<std::unique_ptr<Clasp::ClingoPropagatorInit>> propagators_;
    ClingoPropagatorLock                                      propLock_;
    Gringo::MessagePrinter                                   &logger_;
#if WITH_THREADS
    std::unique_ptr<ClingoSolveFuture> solveFuture_;
#endif
    std::unique_ptr<ClingoSolveIter>   solveIter_;
    bool enableEnumAssupmption_ = true;
    bool clingoMode_;
    bool verbose_               = false;
    bool parsed                 = false;
    bool grounded               = false;
    bool incremental            = false;
    bool configUpdate_          = false;
};

// {{{1 declaration of ClingoLib

class ClingoLib : public Clasp::EventHandler, public ClingoControl {
    using StringVec    = std::vector<std::string>;
public:
    ClingoLib(Gringo::Scripts &scripts, int argc, char const **argv, Gringo::MessagePrinter &log);
    virtual ~ClingoLib();
protected:
    void initOptions(ProgramOptions::OptionContext& root);
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
    Gringo::Control *newControl(int argc, char const **argv) override;
    void freeControl(Gringo::Control *ctl) override;
    Gringo::Symbol parseValue(std::string const &str) override;
    Gringo::Input::GroundTermParser parser;
    Gringo::Scripts scripts;
    Gringo::DefaultMessagePrinter logger;
};

// }}}1

#endif // _GRINGO_CLINGOCONTROL_HH
