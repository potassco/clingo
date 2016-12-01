// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
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

#ifndef CLINGO_CONTROL_HH
#define CLINGO_CONTROL_HH

#include <gringo/symbol.hh>
#include <gringo/types.hh>
#include <gringo/locatable.hh>
#include <gringo/backend.hh>
#include <gringo/logger.hh>
#include <gringo/output/literal.hh>
#include <potassco/clingo.h>
#include <clingo.h>

namespace Gringo {

// {{{1 declaration of SymbolicAtoms

using SymbolicAtoms = clingo_symbolic_atoms;
using SymbolicAtomIter = clingo_symbolic_atom_iterator_t;

} // namespace Gringo

struct clingo_symbolic_atoms {
    virtual Gringo::Symbol atom(Gringo::SymbolicAtomIter it) const = 0;
    virtual Potassco::Lit_t literal(Gringo::SymbolicAtomIter it) const = 0;
    virtual bool fact(Gringo::SymbolicAtomIter it) const = 0;
    virtual bool external(Gringo::SymbolicAtomIter it) const = 0;
    virtual Gringo::SymbolicAtomIter next(Gringo::SymbolicAtomIter it) = 0;
    virtual bool valid(Gringo::SymbolicAtomIter it) const = 0;
    virtual Gringo::SymbolicAtomIter begin(Gringo::Sig sig) const = 0;
    virtual Gringo::SymbolicAtomIter begin() const = 0;
    virtual Gringo::SymbolicAtomIter lookup(Gringo::Symbol atom) const = 0;
    virtual bool eq(Gringo::SymbolicAtomIter it, Gringo::SymbolicAtomIter jt) const = 0;
    virtual Gringo::SymbolicAtomIter end() const = 0;
    virtual std::vector<Gringo::Sig> signatures() const = 0;
    virtual size_t length() const = 0;
    virtual ~clingo_symbolic_atoms() noexcept = default;
};

namespace Gringo {

// {{{1 declaration of SolveResult

class SolveResult {
public:
    enum Satisfiabily : unsigned { Unknown=0, Satisfiable=1, Unsatisfiable=2 };
    SolveResult(Satisfiabily status, bool exhausted, bool interrupted)
    : repr_(static_cast<unsigned>(status) | (exhausted << 2) | (interrupted << 3)) { }
    SolveResult(unsigned repr)
    : repr_(repr) { }
    Satisfiabily satisfiable() const { return static_cast<Satisfiabily>(repr_ & 3); }
    bool exhausted() const { return (repr_ >> 2) & 1; }
    bool interrupted() const { return (repr_ >> 3) & 1; }
    operator unsigned() const { return repr_; }
private:
    unsigned repr_;
};

// {{{1 declaration of Model

enum class ModelType : clingo_model_type_t {
    StableModel = clingo_model_type_stable_model,
    BraveConsequences = clingo_model_type_brave_consequences,
    CautiousConsequences = clingo_model_type_cautious_consequences
};
using Model = clingo_model;
using Int64Vec = std::vector<int64_t>;

} // namespace Gringo

using ShowType = clingo_show_type_bitset_t;
struct clingo_model {
    using LitVec = std::vector<std::pair<Gringo::Symbol, bool>>;
    virtual bool contains(Gringo::Symbol atom) const = 0;
    virtual Gringo::SymSpan atoms(ShowType showset) const = 0;
    virtual Gringo::Int64Vec optimization() const = 0;
    virtual bool optimality_proven() const = 0;
    virtual void addClause(LitVec const &lits) const = 0;
    virtual uint64_t number() const = 0;
    virtual Potassco::Id_t threadId() const = 0;
    virtual Gringo::ModelType type() const = 0;
    virtual ~clingo_model() { }
};

namespace Gringo {

// {{{1 declaration of SolveFuture

struct SolveFuture {
    using EventHandler = std::function<void (Model *)>;
    virtual SolveResult get() = 0;
    virtual Model const *model() = 0;
    virtual void wait() = 0;
    virtual bool wait(double timeout) = 0;
    virtual void cancel() = 0;
    virtual void resume() = 0;
    virtual void notify(EventHandler cb) = 0;
    virtual ~SolveFuture() { }
};

struct DefaultSolveFuture : SolveFuture {
    SolveResult get() override { resume(); return {SolveResult::Unknown, false, false}; }
    Model const *model() override { resume(); return nullptr; }
    void wait() override { resume(); }
    bool wait(double) override { resume(); return true; }
    void cancel() override { resume(); }
    void resume() override {
        if (!done_) {
            done_ = true;
            if (cb_) { cb_(nullptr); }
        }
    }
    void notify(EventHandler cb) override { cb_ = cb; }
    ~DefaultSolveFuture() override { resume(); }
private:
    EventHandler cb_;
    bool done_ = false;
};

// {{{1 declaration of ConfigProxy

struct ConfigProxy {
    virtual bool hasSubKey(unsigned key, char const *name) = 0;
    virtual unsigned getSubKey(unsigned key, char const *name) = 0;
    virtual unsigned getArrKey(unsigned key, unsigned idx) = 0;
    virtual void getKeyInfo(unsigned key, int* nSubkeys = 0, int* arrLen = 0, const char** help = 0, int* nValues = 0) const = 0;
    virtual const char* getSubKeyName(unsigned key, unsigned idx) const = 0;
    virtual bool getKeyValue(unsigned key, std::string &value) = 0;
    virtual void setKeyValue(unsigned key, const char *val) = 0;
    virtual unsigned getRootKey() = 0;
    virtual ~ConfigProxy() { }
};

// {{{1 declaration of Propagator

using PropagateControl = clingo_propagate_control;
using PropagateInit = clingo_propagate_init;

} // namespace Gringo

struct clingo_propagate_init {
    virtual Gringo::Output::DomainData const &theory() const = 0;
    virtual Gringo::SymbolicAtoms &getDomain() = 0;
    virtual Gringo::Lit_t mapLit(Gringo::Lit_t lit) = 0;
    virtual void addWatch(Gringo::Lit_t lit) = 0;
    virtual int threads() = 0;
    virtual ~clingo_propagate_init() noexcept = default;
};

namespace Gringo {

struct Propagator : Potassco::AbstractPropagator {
    virtual ~Propagator() noexcept = default;
    virtual void init(Gringo::PropagateInit &init) = 0;
};
using UProp = std::unique_ptr<Propagator>;

// {{{1 declaration of Control

using StringVec = std::vector<String>;
using Control = clingo_control;

} // namespace Gringo

struct clingo_control {
    using ModelHandler = std::function<bool (Gringo::Model const &)>;
    using FinishHandler = std::function<void (Gringo::SolveResult)>;
    using Assumptions = std::vector<std::pair<Gringo::Symbol, bool>>;
    using GroundVec = std::vector<std::pair<Gringo::String, Gringo::SymVec>>;
    using NewControlFunc = Gringo::Control* (*)(int, char const **);
    using FreeControlFunc = void (*)(Gringo::Control *);

    virtual Gringo::ConfigProxy &getConf() = 0;
    virtual Gringo::SymbolicAtoms &getDomain() = 0;

    virtual void ground(GroundVec const &vec, Gringo::Context *context) = 0;
    virtual Gringo::SolveResult solve(ModelHandler h, Assumptions &&assumptions) = 0;
    virtual Gringo::SolveFuture *solveAsync(ModelHandler mh, FinishHandler fh, Assumptions &&assumptions) = 0;
    virtual Gringo::SolveFuture *solveIter(Assumptions &&assumptions) = 0;
    virtual Gringo::SolveFuture *solveRefactored(Assumptions &&assumptions, clingo_solve_mode_bitset_t mode) = 0;
    virtual void interrupt() = 0;
    virtual void *claspFacade() = 0;
    virtual void add(std::string const &name, Gringo::StringVec const &params, std::string const &part) = 0;
    virtual void load(std::string const &filename) = 0;
    virtual Gringo::Symbol getConst(std::string const &name) = 0;
    virtual bool blocked() = 0;
    virtual void assignExternal(Gringo::Symbol ext, Potassco::Value_t val) = 0;
    virtual Potassco::AbstractStatistics *statistics() = 0;
    virtual void useEnumAssumption(bool enable) = 0;
    virtual bool useEnumAssumption() = 0;
    virtual void cleanupDomains() = 0;
    virtual Gringo::Output::DomainData const &theory() const = 0;
    virtual void registerPropagator(std::unique_ptr<Gringo::Propagator> p, bool sequential) = 0;
    virtual void registerObserver(Gringo::UBackend program, bool replace) = 0;
    virtual Potassco::Atom_t addProgramAtom() = 0;
    virtual Gringo::Backend *backend() = 0;
    virtual Gringo::Logger &logger() = 0;
    virtual void beginAdd() = 0;
    virtual void add(clingo_ast_statement_t const &stm) = 0;
    virtual void endAdd() = 0;
    virtual ~clingo_control() noexcept = default;
};

namespace Gringo {

// }}}1

} // namespace Gringo

#endif // CLINGO_CONTROL_HH
