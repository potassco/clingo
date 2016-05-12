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
#include <clasp/cli/clasp_options.h>
#include <program_opts/application.h>
#include <program_opts/string_convert.h>

// {{{1 declaration of ClingoLpOutput

class ClingoLpOutput : public Gringo::Output::LparseOutputter {
    public:
        ClingoLpOutput(Clasp::Asp::LogicProgram& out) : prg_(out) {
            false_ = prg_.newAtom();
            prg_.setCompute(false_, false);
        }
        unsigned falseUid() { return false_; }
        unsigned newUid()   { return prg_.newAtom(); }
        void printBasicRule(unsigned head, LitVec const &body);
        void printChoiceRule(AtomVec const &atoms, LitVec const &body);
        void printCardinalityRule(unsigned head, unsigned lower, LitVec const &body);
        void printWeightRule(unsigned head, unsigned lower, LitWeightVec const &body);
        void printMinimize(LitWeightVec const &body);
        void printDisjunctiveRule(AtomVec const &atoms, LitVec const &body);
        void finishRules()   { /* noop */ }
        void printSymbol(unsigned atomUid, Gringo::Value v);
        void printExternal(unsigned atomUid, Gringo::TruthValue type);
        void finishSymbols() { /* noop */ }
        bool &disposeMinimize();

    private:
        void addBody(const LitVec& body);
        void addBody(const LitWeightVec& body);
        ClingoLpOutput(const ClingoLpOutput&);
        ClingoLpOutput& operator=(const ClingoLpOutput&);
        Clasp::Asp::LogicProgram& prg_;
        unsigned false_;
        std::stringstream str_;
        bool disposeMinimize_ = true;
};

// {{{1 declaration of ClingoOptions

struct ClingoOptions {
    using Foobar = std::vector<Gringo::FWSignature>;
    ProgramOptions::StringSeq defines;
    Gringo::Output::LparseDebug lparseDebug;
    bool verbose               = false;
    bool text                  = false;
    bool lpRewrite             = false;
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

inline bool parseFoobar(const std::string& str, ClingoOptions::Foobar& foobar) {
    for (auto &x : split(str, ",")) {
        auto y = split(x, "/");
        if (y.size() != 2) { return false; }
        unsigned a;
        if (!bk_lib::string_cast<unsigned>(y[1], a)) { return false; }
        foobar.emplace_back(y[0], a);
    }
    return true;
}

// {{{1 declaration of ClingoStatistics

struct ClingoStatistics : Gringo::Statistics {
    virtual Quantity    getStat(char const* key) const;
    virtual char const *getKeys(char const* key) const;
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
    virtual bool contains(Gringo::Value atom) const {
        auto atm = out.find(atom);
        return atm && model->isTrue(lp.getLiteral(atm->uid()));
    }
    virtual Gringo::ValVec atoms(int atomset) const {
        return out.atoms(atomset, [this, atomset](unsigned uid) -> bool { return bool(atomset & COMP) ^ model->isTrue(lp.getLiteral(uid)); });
    }
    virtual Gringo::Int64Vec optimization() const {
        return model->costs ? Gringo::Int64Vec(model->costs->begin(), model->costs->end()) : Gringo::Int64Vec();
    }
    virtual void addClause(LitVec const &lits) const {
        Clasp::LitVec claspLits;
        for (auto &x : lits) {
            Gringo::AtomState const *atom = out.find(x.second);
            Clasp::Literal lit = lp.getLiteral(atom && atom->hasUid() ? atom->uid() : 1);
            claspLits.push_back(x.first ? lit : ~lit);
        }
        claspLits.push_back(~ctx.stepLiteral());
        model->ctx->commitClause(claspLits);
    }
    virtual ~ClingoModel() { }
    Clasp::Asp::LogicProgram const   &lp;
    Gringo::Output::OutputBase const &out;
    Clasp::SharedContext const       &ctx;
    Clasp::Model const               *model;
};

// {{{1 declaration of ClingoSolveIter

#if WITH_THREADS
struct ClingoSolveIter : Gringo::SolveIter {
    ClingoSolveIter(Clasp::ClaspFacade::AsyncResult const &future, Clasp::Asp::LogicProgram const &lp, Gringo::Output::OutputBase const &out, Clasp::SharedContext const &ctx);

    virtual Gringo::Model const *next();
    virtual void close();
    virtual Gringo::SolveResult get();
    
    virtual ~ClingoSolveIter();

    Clasp::ClaspFacade::AsyncResult future;
    ClingoModel                     model;
};
#endif

// {{{1 declaration of ClingoSolveFuture

Gringo::SolveResult convert(Clasp::ClaspFacade::Result res);
#if WITH_THREADS
struct ClingoSolveFuture : Gringo::SolveFuture {
    ClingoSolveFuture(Clasp::ClaspFacade::AsyncResult const &res);
    // async
    virtual Gringo::SolveResult get();
    virtual void wait();
    virtual bool wait(double timeout);
    virtual void cancel();
    
    virtual ~ClingoSolveFuture();
    void reset(Clasp::ClaspFacade::AsyncResult res);

    Clasp::ClaspFacade::AsyncResult future;
    Gringo::SolveResult             ret = Gringo::SolveResult::UNKNOWN;
    bool                            done = false;
};
#endif

// {{{1 declaration of ClingoControl

class ClingoControl : public Gringo::Control, private Gringo::ConfigProxy, private Gringo::DomainProxy {
public:
    using StringVec        = std::vector<std::string>;
    using ExternalVec      = std::vector<std::pair<Gringo::Value, Gringo::TruthValue>>;
    using StringSeq        = ProgramOptions::StringSeq;
    using PostGroundFunc   = std::function<bool (Clasp::ProgramBuilder &)>;
    using PreSolveFunc     = std::function<bool (Clasp::ClaspFacade &)>;
    enum class ConfigUpdate { KEEP, REPLACE };

    ClingoControl(Gringo::Scripts &scripts, bool clingoMode, Clasp::ClaspFacade *clasp, Clasp::Cli::ClaspCliConfig &claspConfig, PostGroundFunc pgf, PreSolveFunc psf);
    void prepare_(Gringo::Control::ModelHandler mh, Gringo::Control::FinishHandler fh);
    void commitExternals();
    void parse_();
    void parse(const StringSeq& files, const ClingoOptions& opts, Clasp::Asp::LogicProgram* out, bool addStdIn = true);
    void main();
    bool onModel(Clasp::Model const &m);
    void onFinish(Clasp::ClaspFacade::Result ret);
    bool update();

    Clasp::LitVec toClaspAssumptions(std::vector<int> &&ass) const;
    std::vector<int> toLparseAssumptions(Gringo::Control::Assumptions &&ass) const;
    
    // {{{2 DomainProxy interface

    virtual ElementPtr iter(Gringo::Signature const &sig) const;
    virtual ElementPtr iter() const;
    virtual ElementPtr lookup(Gringo::Value const &atom) const;
    virtual size_t length() const;
    virtual std::vector<Gringo::FWSignature> signatures() const;

    // {{{2 ConfigProxy interface

    virtual bool hasSubKey(unsigned key, char const *name, unsigned* subKey = nullptr);
    virtual unsigned getSubKey(unsigned key, char const *name);
    virtual unsigned getArrKey(unsigned key, unsigned idx);
    virtual void getKeyInfo(unsigned key, int* nSubkeys = 0, int* arrLen = 0, const char** help = 0, int* nValues = 0) const;
    virtual const char* getSubKeyName(unsigned key, unsigned idx) const;
    virtual bool getKeyValue(unsigned key, std::string &value);
    virtual void setKeyValue(unsigned key, const char *val);
    virtual unsigned getRootKey();

    // {{{2 Control interface

    virtual Gringo::DomainProxy &getDomain();
    virtual void ground(Gringo::Control::GroundVec const &vec, Gringo::Any &&context);
    virtual void add(std::string const &name, Gringo::FWStringVec const &params, std::string const &part);
    virtual void load(std::string const &filename);
    virtual void prepareSolve(Assumptions &&ass);
    virtual Gringo::SolveResult solve(ModelHandler h);
    virtual bool blocked();
    virtual std::string str();
    virtual void assignExternal(Gringo::Value ext, Gringo::TruthValue);
    virtual Gringo::Value getConst(std::string const &name);
    virtual ClingoStatistics *getStats();
    virtual Gringo::ConfigProxy &getConf();
    virtual void useEnumAssumption(bool enable);
    virtual bool useEnumAssumption();
    virtual void cleanupDomains();
    virtual Gringo::SolveIter *solveIter();
    virtual Gringo::SolveFuture *solveAsync(ModelHandler mh, FinishHandler fh);

    // }}}2

    std::unique_ptr<Gringo::Output::OutputBase>             out;
    std::unique_ptr<Gringo::Output::LparseOutputter>        lpOut;
    Gringo::Scripts                                        &scripts;
    Gringo::Input::Program                                  prg;
    Gringo::Defines                                         defs;
    std::unique_ptr<Gringo::Input::NongroundProgramBuilder> pb;
    std::unique_ptr<Gringo::Input::NonGroundParser>         parser;
    ModelHandler                                            modelHandler;
    FinishHandler                                           finishHandler;
    ClingoStatistics                                        clingoStats;
    Clasp::ClaspFacade                                     *clasp = nullptr;
    Clasp::Cli::ClaspCliConfig                             &claspConfig_;
    PostGroundFunc                                          pgf_;
    PreSolveFunc                                            psf_;
    std::vector<int>                                        ass_;
#if WITH_THREADS
    std::unique_ptr<ClingoSolveFuture> solveFuture_;
    std::unique_ptr<ClingoSolveIter>   solveIter_;
#endif
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
    ClingoLib(Gringo::Scripts &scripts, int argc, char const **argv);
    virtual ~ClingoLib();
protected:
    void initOptions(ProgramOptions::OptionContext& root);
    static bool parsePositional(const std::string& s, std::string& out);
    // -------------------------------------------------------------------------------------------
    // Event handler
    virtual void onEvent(const Clasp::Event& ev);
    virtual bool onModel(const Clasp::Solver& s, const Clasp::Model& m);
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
    Gringo::Control *newControl(int argc, char const **argv);
    void freeControl(Gringo::Control *ctl);
    virtual Gringo::Value parseValue(std::string const &str);
    Gringo::Input::GroundTermParser parser;
    Gringo::Scripts scripts;
};

// }}}1

#endif // _GRINGO_CLINGOCONTROL_HH
