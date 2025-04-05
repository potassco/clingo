#include "clingo/control/solver.hh"

#include "clingo/output/text.hh"

#include <clasp/solver.h>

#include <potassco/aspif_text.h>
#include <potassco/theory_data.h>

// #define DEBUG_BACKEND
#ifdef DEBUG_BACKEND
#include <clingo/util/print.hh>
#include <iostream>
#endif

namespace Clingo::Control {

namespace {

//! Implementation of the backend interface.
class ProgramBackendImpl : public ProgramBackend {
  public:
    ProgramBackendImpl(Clasp::Asp::LogicProgram &prg) : prg_{&prg} {}

  private:
    //! Hook before adding atoms.
    virtual void do_pre_show_atom([[maybe_unused]] Symbol sym, [[maybe_unused]] prg_lit_t lit) {}

    void do_preamble([[maybe_unused]] unsigned major, [[maybe_unused]] unsigned minor,
                     [[maybe_unused]] unsigned revision, [[maybe_unused]] bool incremental) override {
        // TODO: maybe assert that program updates are enabled
    }
    void do_end() override {}

    auto do_next_lit() -> prg_lit_t override {
        if (auto lit = prg_->newAtom(); std::cmp_less_equal(lit, prg_lit_max)) {
            return static_cast<prg_lit_t>(lit);
        }
        throw std::range_error("literals number of literals exhausted");
    }

    void do_rule(PrgLitSpan head, PrgLitSpan body, bool choice) override {
        bld_.clear();
        bld_.start(choice ? Potassco::HeadType::choice : Potassco::HeadType::disjunctive);
#ifdef DEBUG_BACKEND
        std::cerr << (choice ? "{ " : "") << Util::p_range(head, ", ") << (choice ? " }" : "") << " :- "
                  << Util::p_range(body, ", ") << ".\n";
#endif
        for (auto const &lit : head) {
            assert(lit > 0);
            bld_.addHead(lit);
        }
        bld_.startBody();
        for (auto const &lit : body) {
            bld_.addGoal(lit);
        }
        prg_->addRule(bld_);
    }

    void do_bd_aggr(PrgLitSpan head, WeightedPrgLitSpan body, prg_weight_t bound, bool choice) override {
        assert(bound > 0);
        bld_.clear();
        bld_.start(choice ? Potassco::HeadType::choice : Potassco::HeadType::disjunctive);
        for (auto const &atom : head) {
            assert(atom > 0);
            bld_.addHead(atom);
        }
        bld_.startSum(bound);
        for (auto const &[lit, weight] : body) {
            assert(weight > 0);
            bld_.addGoal(lit, weight);
        }
        prg_->addRule(bld_);
#ifdef DEBUG_BACKEND
        std::cerr << head << " :- " << "{"
                  << Util::p_range(body, "",
                                   [](auto &out, auto &wlit) { out << " " << wlit.first << "=" << wlit.second; })
                  << " } >= " << bound << ".\n";
#endif
    }

    void do_edge(prg_id_t u, prg_id_t v, PrgLitSpan body) override { prg_->addAcycEdge(u, v, body); }

    void do_heuristic(prg_lit_t atom, int32_t weight, int32_t prio, HeuristicType type, PrgLitSpan body) override {
        assert(atom > 0);
        static_assert(static_cast<unsigned>(Clingo::HeuristicType::init) ==
                      static_cast<unsigned>(Clasp::DomModType::init));
        static_assert(static_cast<unsigned>(Clingo::HeuristicType::factor) ==
                      static_cast<unsigned>(Clasp::DomModType::factor));
        static_assert(static_cast<unsigned>(Clingo::HeuristicType::false_) ==
                      static_cast<unsigned>(Clasp::DomModType::false_));
        static_assert(static_cast<unsigned>(Clingo::HeuristicType::level) ==
                      static_cast<unsigned>(Clasp::DomModType::level));
        static_assert(static_cast<unsigned>(Clingo::HeuristicType::sign) ==
                      static_cast<unsigned>(Clasp::DomModType::sign));
        static_assert(static_cast<unsigned>(Clingo::HeuristicType::true_) ==
                      static_cast<unsigned>(Clasp::DomModType::true_));
        prg_->addDomHeuristic(atom, static_cast<Clasp::DomModType>(type), weight, prio, body);
    }

    void do_external(prg_lit_t atom, ExternalType type) override {
        static_assert(static_cast<unsigned>(ExternalType::free) == static_cast<unsigned>(Potassco::TruthValue::free));
        static_assert(static_cast<unsigned>(ExternalType::true_) == static_cast<unsigned>(Potassco::TruthValue::true_));
        static_assert(static_cast<unsigned>(ExternalType::false_) ==
                      static_cast<unsigned>(Potassco::TruthValue::false_));
        static_assert(static_cast<unsigned>(ExternalType::release) ==
                      static_cast<unsigned>(Potassco::TruthValue::release));
        prg_->addExternal(atom, static_cast<Potassco::TruthValue>(type));
    }

    void do_project(PrgLitSpan atoms) override {
        for (auto const &atom : atoms) {
            prg_->addProject(std::array{static_cast<Potassco::Atom_t>(atom)});
        }
    }

    void do_assume(PrgLitSpan literals) override { prg_->addAssumption(literals); }

    void do_minimize(prg_weight_t priority, WeightedPrgLitSpan body) override {
        auto wlits = Util::small_vector<Potassco::WeightLit>{};
        wlits.reserve(body.size());
        for (auto const &[lit, weight] : body) {
            wlits.emplace_back(lit, weight);
        }
        prg_->addMinimize(priority, wlits);
    }

    void do_show(Symbol sym, PrgLitSpan body) override {
        buf_.reset();
        buf_ << sym;
        prg_->addOutput(buf_.c_str(), body);
#ifdef DEBUG_BACKEND
        std::cerr << "#show " << sym << " : " << Util::p_range(body, ", ") << ".\n";
#endif
    }

    void do_show_atom(Symbol sym, prg_lit_t lit) override {
        assert(lit > 0);
        do_pre_show_atom(sym, lit);
        buf_.reset();
        buf_ << sym;
        prg_->addOutput(buf_.c_str(), lit);
#ifdef DEBUG_BACKEND
        std::cerr << "#show " << sym << " : " << lit << ".\n";
#endif
    }

    Util::OutputBuffer buf_;
    Potassco::RuleBuilder bld_;
    Clasp::Asp::LogicProgram *prg_;
};

//! Implementation of the theory backend interface.
class TheoryBackendImpl : public TheoryBackend {
  public:
    TheoryBackendImpl(Clasp::Asp::LogicProgram &prg) : prg_{&prg} {}

  private:
    void do_num(prg_id_t id, int32_t num) override { prg_->theoryData().addTerm(id, num); }

    void do_str(prg_id_t id, std::string_view str) override { prg_->theoryData().addTerm(id, str); }

    void do_fun(prg_id_t id, prg_id_t name, PrgIdSpan args) override {
        assert(!args.empty());
        prg_->theoryData().addTerm(id, name, args);
    }

    void do_tup(prg_id_t id, TheoryTermTupleType type, PrgIdSpan args) override {
        prg_->theoryData().addTerm(
            id,
            [type] {
                switch (type) {
                    case TheoryTermTupleType::tuple: {
                        return Potassco::TupleType::paren;
                    }
                    case TheoryTermTupleType::list: {
                        return Potassco::TupleType::bracket;
                    }
                    case TheoryTermTupleType::set: {
                        return Potassco::TupleType::brace;
                    }
                }
                Util::unreachable();
            }(),
            args);
    }

    void do_elem(prg_id_t id, PrgIdSpan terms, PrgLitSpan cond) override {
        prg_->theoryData().addElement(id, terms, prg_->newCondition(cond));
    }

#ifdef DEBUG_BACKEND
    void print(Potassco::TheoryTerm const &term) {
        switch (term.type()) {
            case Potassco::TheoryTermType::number: {
                std::cerr << term.number();
                return;
            }
            case Potassco::TheoryTermType::symbol: {
                std::cerr << term.symbol();
                return;
            }
            case Potassco::TheoryTermType::compound: {
                if (term.isFunction()) {
                    print(prg_->theoryData().getTerm(term.function()));
                    std::cerr << "(" << Util::p_range(term, [this]([[maybe_unused]] auto &out, auto &term) {
                        print(prg_->theoryData().getTerm(term));
                    }) << ")";
                } else {
                    // TODO: trailing comma + proper parens
                    std::cerr << "(" << Util::p_range(term, [this]([[maybe_unused]] auto &out, auto &term) {
                        print(prg_->theoryData().getTerm(term));
                    }) << ")";
                }
                return;
            }
            default: {
                Util::unreachable();
            }
        }
    }

    void print(Potassco::TheoryAtom const &atom) {
        std::cerr << "&";
        print(prg_->theoryData().getTerm(atom.term()));
        std::cerr << " { " << Util::p_range(atom.elements(), "; ", [this]([[maybe_unused]] auto &out, auto &elem) {
            print(prg_->theoryData().getElement(elem));
        }) << " }";
    }

    void print(Potassco::TheoryElement const &elem) {
        std::cerr << Util::p_range(elem, [this]([[maybe_unused]] auto &out, auto &term) {
            print(prg_->theoryData().getTerm(term));
        }) << ": " << elem.condition();
    }
#endif

    void do_atom(prg_lit_t lit_or_zero, prg_id_t name, PrgIdSpan elems,
                 std::optional<std::pair<prg_id_t, prg_id_t>> guard) override {
        if (guard) {
            prg_->theoryData().addAtom(lit_or_zero, name, elems, guard->first, guard->second);
        } else {
            prg_->theoryData().addAtom(lit_or_zero, name, elems);
        }
#ifdef DEBUG_BACKEND
        std::cerr << lit_or_zero << " <> ";
        print(**(prg_->theoryData().end() - 1));
        std::cerr << "\n";
#endif
    }

    Clasp::Asp::LogicProgram *prg_;
};

//! Implementation of the program backend for aspif parser.
class ProgramBackendAdapter : public ProgramBackendImpl {
  public:
    ProgramBackendAdapter(Ground::Bases &bases, Clasp::Asp::LogicProgram &prg)
        : ProgramBackendImpl{prg}, bases_{&bases} {}

  private:
    void do_pre_show_atom(Symbol sym, prg_lit_t lit) override {
        auto sig = sym.signature();
        if (!sig) {
            throw std::runtime_error{"unexpected symbol for atom"};
        }
        auto &base = bases_->add_base(*sig);
        base.add(sym, Ground::StateAtom::unknown, [lit]() { return lit; });
    }

    Ground::Bases *bases_;
};

//! Implementation of the theory backend for aspif parser.
class TheoryBackendAdapter : public TheoryBackend {
  public:
    TheoryBackendAdapter(SymbolStore &store, Output::TheoryData &data) : store_{&store}, data_{&data} {}

  private:
    //! Get a remapped term.
    auto term_(prg_id_t id) -> prg_id_t { return term_map_[id]; }

    //! Get the remapped terms.
    auto term_(PrgIdSpan span) {
        Output::TheoryData::IdVec vec;
        vec.reserve(span.size());
        std::ranges::transform(span, std::back_inserter(vec), [this](auto id) { return term_(id); });
        return vec;
    }

    //! Remap the old term to the new one.
    void term_(prg_id_t id_old, prg_id_t id_new) { term_map_[id_old] = id_new; }

    //! Get the remapped element.
    auto elem_(prg_id_t id) -> prg_id_t { return elem_map_[id]; }

    //! Get the remapped elements.
    auto elem_(PrgIdSpan span) {
        Output::TheoryData::IdVec vec;
        vec.reserve(span.size());
        std::ranges::transform(span, std::back_inserter(vec), [this](auto id) { return elem_(id); });
        return vec;
    }

    //! Remap the old elem to the new one.
    void elem_(prg_id_t id_old, prg_id_t id_new) { elem_map_[id_old] = id_new; }

    void do_num(prg_id_t id, int32_t num) override { term_(id, data_->num(num)); }

    void do_str(prg_id_t id, std::string_view str) override { term_(id, data_->str(*store_->string(str))); }

    void do_fun(prg_id_t id, prg_id_t name, PrgIdSpan args) override {
        assert(!args.empty());
        term_(id, data_->fun(term_(name), term_(args)));
    }

    void do_tup(prg_id_t id, TheoryTermTupleType type, PrgIdSpan args) override {
        term_(id, data_->tup(type, term_(args)));
    }

    void do_elem(prg_id_t id, PrgIdSpan terms, PrgLitSpan cond) override { elem_(id, data_->elem(term_(terms), cond)); }

    void do_atom(prg_lit_t lit_or_zero, prg_id_t name, PrgIdSpan elems,
                 std::optional<std::pair<prg_id_t, prg_id_t>> guard) override {
        if (guard) {
            data_->atom(nullptr, name, elem_(elems), std::pair{term_(guard->first), term_(guard->second)});
        } else {
            data_->atom([lit_or_zero]() { return lit_or_zero; }, name, elem_(elems), std::nullopt);
        }
    }

    SymbolStore *store_;
    Output::TheoryData *data_;
    Util::unordered_map<prg_id_t, prg_id_t> term_map_;
    Util::unordered_map<prg_id_t, prg_id_t> elem_map_;
};

class ModelExtend : public Clasp::OutputTable::Theory {
  public:
    auto first([[maybe_unused]] const Clasp::Model &m) -> char const * override {
        index_ = 0;
        return next();
    }
    auto next() -> char const * override {
        if (index_ < syms_.size()) {
            buf_.reset();
            buf_ << syms_[index_++];
            return buf_.c_str();
        }
        return nullptr;
    }
    void clear() { syms_.clear(); }
    void add(SymbolSpan symbols) { syms_.insert(syms_.end(), symbols.begin(), symbols.end()); }
    [[nodiscard]] auto symbols() const -> SymbolSpan { return syms_; }

  private:
    Util::OutputBuffer buf_;
    SymbolVec syms_;
    size_t index_ = 0;
};

//! Implementation of the model interface.
class ModelImpl : public Model, private SolveControl {
  public:
    ModelImpl(Ground::Bases const &bases, Clasp::ClaspFacade &clasp) : bases_{&bases}, clasp_{&clasp} {
        clasp_->ctx.output.theory = &extend_;
    }
    ~ModelImpl() override { clasp_->ctx.output.theory = nullptr; }

    //! Sets the given model returning true if it is not null.
    auto set_model(Clasp::Model const *mdl, bool clear_extend = true) -> bool {
        // NOTE: Extended symbols carry over to the last model.
        if (clear_extend) {
            extend_.clear();
        }
        return (mdl_ = mdl) != nullptr;
    }

    //! Sets the last model returning true if it is not null.
    auto set_last() -> bool {
        return (mdl_ = clasp_->solved() && clasp_->summary().sat() && clasp_->summary().model() != nullptr
                           ? clasp_->summary().model()
                           : nullptr) != nullptr;
    }

    //! Get the underlying clasp facade.
    auto clasp() -> Clasp::ClaspFacade & { return *clasp_; }

  private:
    void do_symbols(SymbolSelectFlags type, SymbolVec &res) const override {
        assert(mdl_ != nullptr);
        if (auto show_a = intersects(type, SymbolSelectFlags::atoms),
            show_s = intersects(type, SymbolSelectFlags::shown);
            show_a || show_s) {
            for (auto const &base : bases_->atoms()) {
                for (size_t i = 0, e = show_a ? base.second->size() : base.second->num_shown(); i != e; ++i) {
                    auto [sym, state] = *base.second->nth(i);
                    if (mdl_->isTrue(solver_literal(state.id))) {
                        res.emplace_back(sym);
                    }
                }
            }
        }
        if (intersects(type, SymbolSelectFlags::terms | SymbolSelectFlags::shown)) {
            for (auto const &[term, state] : bases_->terms()) {
                auto const &[flags, conds] = state;
                if (flags.state == Ground::ShowTermState::done ||
                    std::ranges::any_of(conds, [this](auto const &uid) { return mdl_->isTrue(solver_literal(uid)); })) {
                    res.emplace_back(term);
                }
            }
        }
        if (intersects(type, SymbolSelectFlags::theory | SymbolSelectFlags::shown)) {
            auto syms = extend_.symbols();
            res.insert(res.end(), syms.begin(), syms.end());
        }
    }

    [[nodiscard]] auto do_number() const -> uint64_t override {
        assert(mdl_ != nullptr);
        return mdl_->num;
    }

    [[nodiscard]] auto do_type() const -> ModelType override {
        assert(mdl_ != nullptr);
        if (mdl_->type == Clasp::Model::brave) {
            return ModelType::brave_consequences;
        }
        if (mdl_->type == Clasp::Model::cautious) {
            return ModelType::cautious_consequences;
        }
        return ModelType::model;
    }

    [[nodiscard]] auto do_contains(Symbol sym) const -> bool override {
        assert(mdl_ != nullptr);
        if (sym.type() != SymbolType::function) {
            return false;
        }
        auto *base = bases_->get_base(std::tuple{sym.name(), sym.args().size(), sym.has_classical_sign()});
        if (base == nullptr) {
            return false;
        }
        auto it = base->find(sym);
        if (!it) {
            return false;
        }
        return mdl_->isTrue(solver_literal(it->value().id));
    }

    void do_extend(SymbolSpan symbols) override { extend_.add(symbols); }

    [[nodiscard]] auto do_is_true(prg_lit_t lit) const -> bool override {
        assert(mdl_ != nullptr);
        return mdl_->isTrue(solver_literal(lit));
    }

    [[nodiscard]] auto do_is_consequence(prg_lit_t lit) const -> ConsequenceType override {
        assert(mdl_ != nullptr);
        switch (Clasp::Asp::isConsequence(clasp_program(), lit, *mdl_)) {
            case Clasp::value_true: {
                return ConsequenceType::true_;
            }
            case Clasp::value_false: {
                return ConsequenceType::false_;
            }
            default: {
                return ConsequenceType::unknown;
            }
        }
    }

    [[nodiscard]] auto do_costs() const -> std::span<prg_sum_t const> override {
        assert(mdl_ != nullptr);
        return mdl_->costs;
    }

    [[nodiscard]] auto do_priorities() const -> std::span<prg_weight_t const> override {
        assert(mdl_ != nullptr && mdl_->ctx != nullptr);
        auto const *m = mdl_->ctx->minimizer();
        return m != nullptr ? m->prios : std::span<prg_weight_t const>{};
    }

    [[nodiscard]] auto do_optimality_proven() const -> bool override {
        assert(mdl_ != nullptr);
        return mdl_->opt;
    }

    [[nodiscard]] auto do_thread_id() const -> prg_id_t override {
        assert(mdl_ != nullptr);
        return mdl_->sId;
    }

    [[nodiscard]] auto do_control() -> SolveControl & override { return *this; }

    void do_add_clause(PrgLitSpan lits) override {
        assert(mdl_ != nullptr);
        lits_.clear();
        lits_.reserve(lits.size());
        for (auto const &lit : lits) {
            lits_.emplace_back(solver_literal(lit));
        }
        std::ignore = mdl_->ctx->commitClause(lits_);
    }

    [[nodiscard]] auto do_bases() const -> Ground::Bases const & override { return *bases_; }

    [[nodiscard]] auto do_clasp_program() const -> Clasp::Asp::LogicProgram const & override {
        return clasp_->asp() != nullptr ? *clasp_->asp() : throw std::runtime_error("not in solving mode");
    }

    //! Map the given program literal to its solver literal.
    [[nodiscard]] auto solver_literal(std::integral auto lit) const -> Clasp::Literal {
        return Clasp::Asp::solverLiteral(clasp_program(), static_cast<prg_lit_t>(lit));
    }

    Ground::Bases const *bases_;
    Clasp::ClaspFacade *clasp_;
    Clasp::Model const *mdl_ = nullptr;
    std::vector<Clasp::Literal> lits_;
    ModelExtend extend_;
};

//! Convert clasp's solve result into clingo's simplified version.
auto convert(Clasp::SolveResult cres) -> SolveResult {
    static constexpr uint8_t i1 = 9;
    static constexpr uint8_t i2 = 65;
    if (cres.interrupted() && cres.signal != 0 && cres.signal != i1 && cres.signal != i2) {
        throw std::runtime_error("solving stopped by signal");
    }
    auto res = SolveResult::empty;
    if (cres.interrupted()) {
        res |= SolveResult::interrupted;
    }
    if (cres.sat()) {
        res |= SolveResult::satisfiable;
    }
    if (cres.unsat()) {
        res |= SolveResult::unsatisfiable;
    }
    if (cres.exhausted()) {
        res |= SolveResult::exhausted;
    }
    return res;
}

//! An event handler that adapts clingo's event handler to clasp's.
class EventHandlerAdapter : public Clasp::EventHandler {
  public:
    //! Construct a solve event handler adapter.
    //!
    //! This initializes the underlying solve handle, which in turn starts
    //! solving.
    EventHandlerAdapter(CallbackLock &lock, Logger &logger, ModelImpl &mdl, Control::UEventHandler eh)
        : lock_{&lock}, logger_{&logger}, mdl_{&mdl}, eh_{std::move(eh)} {}

    //! Intercept and report models.
    auto onModel([[maybe_unused]] Clasp::Solver const &slv, Clasp::Model const &mdl) -> bool override {
        if (eh_) {
            auto guard = std::lock_guard{*lock_};
            mdl_->set_model(&mdl);
            return eh_->on_model(*mdl_);
        }
        return true;
    }
    //! Intercept log and finish events.
    //!
    //! Finish events are passed on as on_stats and on_finish events.
    void onEvent(Clasp::Event const &event) override {
        using namespace Clasp;
        if (eh_) {
            if (auto const *res = event_cast<ClaspFacade::StepReady>(event); res != nullptr) {
                try {
                    auto guard = std::lock_guard{*lock_};
                    if (auto *stats = mdl_->clasp().getStats(); stats != nullptr) {
                        eh_->on_stats(*stats);
                    }
                    eh_->on_finish(convert(res->summary->result));
                } catch (...) {
                    // NOTE: ensure that exceptions don't escape finish events as this can interfere with solver cleanup
                    ptr_ = std::current_exception();
                }
            }
        }
        if (auto const *log = event_cast<LogEvent>(event); log != nullptr && log->isWarning()) {
            auto guard = std::lock_guard{*lock_};
            logger_->print(MessageCode::warn, log->msg);
        }
    }

    //! Report lower bounds.
    auto onUnsat([[maybe_unused]] Clasp::Solver const &slv, Clasp::Model const &mdl) -> bool override {
        if (auto const *ctx = mdl.ctx; eh_ && ctx != nullptr && ctx->optimize() && ctx->lowerBound().active()) {
            auto lower = ctx->lowerBound();
            assert(lower.level <= mdl.costs.size());
            bound_.reserve(ctx->minimizer()->numRules());
            bound_.assign(mdl.costs.begin(), mdl.costs.begin() + lower.level);
            bound_.push_back(lower.bound);
            auto guard = std::lock_guard{*lock_};
            eh_->on_unsat(bound_);
        }
        return true;
    }

    //! Report unsatisfiable cores.
    void onCore(Potassco::LitSpan core) {
        if (eh_) {
            auto guard = std::lock_guard{*lock_};
            eh_->on_core(core);
        }
    }

    //! Called once solving is finished.
    //!
    //! This is used for exception propagation. Unlike on_finish, this is not
    //! called in the solver thread but during SolveHandle::get.
    void onFinalize() {
        if (ptr_) {
            std::rethrow_exception(ptr_);
            ptr_ = nullptr;
        }
    }

    //! Get the underlying ModelImpl.
    [[nodiscard]] auto model() -> ModelImpl & { return *mdl_; }

    //! Get the underlying lock.
    [[nodiscard]] auto get_lock() const -> CallbackLock & { return *lock_; }

  private:
    CallbackLock *lock_;
    Logger *logger_;
    ModelImpl *mdl_;
    Clasp::SumVec bound_;
    Control::UEventHandler eh_;
    std::exception_ptr ptr_;
};

//! A solve handle that does nothing.
//!
//! @note This handle is used if no solver has been set up (for example, in
//! ground-only mode).
class SolveHandleFixed : public SolveHandle {
  private:
    auto do_get() -> SolveResult override { return SolveResult::empty; }
    void do_cancel() override {}
    void do_resume() override {}
    auto do_model() -> Model const * override { return nullptr; }
    auto do_last() -> Model const * override { return nullptr; }
    auto do_core() -> PrgLitSpan override { return {}; }
    auto do_wait([[maybe_unused]] double timeout) -> bool override { return true; }
};

auto convert(SolveMode mode) -> Clasp::SolveMode {
    auto res = Clasp::SolveMode::def;
    if (intersects(mode, SolveMode::yield)) {
        res |= Clasp::SolveMode::yield;
    }
    if (intersects(mode, SolveMode::async)) {
        res |= Clasp::SolveMode::async;
    }
    return res;
}

constexpr int random_signal = 65;
constexpr int kill_signal = 9;

//! The solve handle implementation.
class SolveHandleImpl : public SolveHandle {
  public:
    SolveHandleImpl(CallbackLock &lock, Logger &log, ModelImpl &mdl, SolveMode mode, UEventHandler eh)
        : eh_{lock, log, mdl, std::move(eh)}, hnd_{mdl.clasp().solve(convert(mode), {}, &eh_)} {}

    ~SolveHandleImpl() override { cancel(); }

  private:
    auto do_get() -> SolveResult override {
        auto guard = unlock_guard{eh_.get_lock()};
        auto res = hnd_.get();
        if (res.interrupted() && res.signal != 0 && res.signal != kill_signal && res.signal != random_signal) {
            throw std::runtime_error("solving stopped by signal");
        }
        if (res.unsat()) {
            eh_.onCore(do_core());
        }
        eh_.onFinalize();
        return convert(res);
    }
    void do_cancel() override {
        auto guard = unlock_guard{eh_.get_lock()};
        hnd_.cancel();
    }
    void do_resume() override {
        auto guard = unlock_guard{eh_.get_lock()};
        hnd_.resume();
    }
    auto do_model() -> Model const * override {
        auto guard = unlock_guard{eh_.get_lock()};
        return eh_.model().set_model(hnd_.model()) ? &eh_.model() : nullptr;
    }
    auto do_last() -> Model const * override { return eh_.model().set_last() ? &eh_.model() : nullptr; }
    auto do_core() -> PrgLitSpan override {
        auto const &clasp = eh_.model().clasp();
        core_.clear();
        if (auto core = clasp.summary().unsatCore(); !core.empty()) {
            clasp.asp()->translateCore(core, core_);
        }
        return core_;
    }
    auto do_wait(double timeout) -> bool override {
        auto guard = unlock_guard{eh_.get_lock()};
        if (timeout == 0) {
            return hnd_.ready();
        }
        if (timeout < 0) {
            return hnd_.wait(), true;
        }
        return hnd_.waitFor(timeout);
    }

    EventHandlerAdapter eh_;
    Clasp::ClaspFacade::SolveHandle hnd_;
    Potassco::LitVec mutable core_;
};

} // namespace

void Scripts::register_script(std::string_view name, UScript script) {
    scripts_.emplace_back(name, std::move(script));
}

void Scripts::do_exec(Location const &loc, Logger &log, std::string_view name, std::string_view code) {
    bool found = false;
    for (auto const &script : scripts_) {
        if (script.first == name) {
            script.second->exec(code);
            found = true;
        }
    }
    if (!found) {
        GRINGO_REPORT_LOC(log, error, loc) << "script support for '" << name << "' not available";
        throw std::runtime_error("script support not available");
    }
}

void Scripts::main(Solver &slv, std::optional<ProgramParamsVec> const &params) {
    for (auto const &script : scripts_) {
        if (script.second->callable("main", 0)) {
            script.second->main(slv, params);
        }
    }
}

auto Scripts::do_callable(std::string_view name, size_t args) -> bool {
    for (auto const &script : scripts_) {
        if (script.second->callable(name, args)) {
            return true;
        }
    }
    return false;
}

void Scripts::do_call(Location const &loc, std::string_view name, SymbolSpan args, SymbolVec &out) {
    out.clear();
    for (auto const &script : scripts_) {
        if (script.second->callable(name, args.size())) {
            script.second->call(loc, name, args, out);
        }
    }
}

Solver::Solver(Clasp::ClaspFacade &clasp, Clasp::Cli::ClaspCliConfig &clasp_config, Logger &log, SymbolStore &store,
               Scripts &scripts, Input::RewriteOptions opts, AppMode mode, FILE *out)
    : clasp_{&clasp}, clasp_config_{&clasp_config}, buf_{out}, out_{make_output_(store, mode)},
      grd_{log, store, opts, *out_}, scripts_{&scripts}, mode_{mode} {
}

auto Solver::make_output_(SymbolStore &store, AppMode mode) -> UOutputStm {
    switch (mode) {
        case AppMode::solve: {
            backend_ = std::make_unique<ProgramBackendImpl>(*clasp_->asp());
            theory_ = std::make_unique<Output::TheoryData>(store, std::make_unique<TheoryBackendImpl>(*clasp_->asp()));
            return Output::make_backend_output(store, *backend_, *theory_);
        }
        default: {
            return Output::make_text_output(buf_);
        }
    }
    Util::unreachable();
}

void Solver::interrupt() noexcept {
    try {
        clasp_facade().interrupt(random_signal);
    } catch (std::exception const &e) {
        printf("panic: %s\n", e.what());
        std::abort();
    }
}

void Solver::main(std::span<std::string_view const> const &files, std::optional<ProgramParamsVec> const &params) {
    parse(files);
    main(params);
}

void Solver::incmode_() {
    // TODO: add options
    auto const &map = const_map();
    auto &store = grd_.store();
    auto get = [&](char const *name, SharedSymbol def) {
        auto it = map.find(name);
        if (it != map.end()) {
            return it->second.second;
        }
        return def;
    };

    auto imin = get("imin", SymbolStore::num(0));
    auto imax = [&]() {
        auto it = map.find("imax");
        return it != map.end() ? std::make_optional(it->second.second) : std::nullopt;
    }();
    auto istop = get("istop", SymbolStore::str(store.string("SAT")));
    auto stop_sat = SymbolStore::str(store.string("SAT"));
    auto stop_unsat = SymbolStore::str(store.string("UNSAT"));
    auto stop_unknown = SymbolStore::str(store.string("UNKNOWN"));
    auto part_check = store.string("check");
    auto part_step = store.string("step");
    auto part_base = store.string("base");
    auto part_query = store.string("query");

    if (imin->type() != SymbolType::number) {
        throw std::invalid_argument{"imin must be a number"};
    }
    if (imax && imax->get().type() != SymbolType::number) {
        throw std::invalid_argument{"imin must be a number"};
    }

    parse(R"(#program check(t).
#external query(t-1). [release]
#external query(t). [true]
)");

    auto step = Number{0};
    auto ret = SolveResult::empty;
    auto cont = [&]() {
        if (imax && step >= imax->get().num()) {
            return false;
        }
        if (step == 0 || step < imin->num()) {
            return true;
        }
        if (istop == stop_sat && intersects(ret, SolveResult::satisfiable)) {
            return false;
        }
        if (istop == stop_unsat && intersects(ret, SolveResult::unsatisfiable)) {
            return false;
        }
        if (istop == stop_unknown && !intersects(ret, SolveResult::satisfiable | SolveResult::unsatisfiable)) {
            return false;
        }
        return true;
    };
    while (cont()) {
        using Param = Clingo::Input::ProgramParam;
        Clingo::Input::ProgramParamVec parts;
        parts.emplace_back(Param{part_check, {store.num(step)}});
        if (step > 0) {
            parts.emplace_back(Param{part_step, {store.num(step)}});
        } else {
            parts.emplace_back(Param{part_base, {}});
        }
        ground(parts, nullptr);
        if (mode_ == AppMode::solve) {
            ret = solve()->get();
        }
        step += 1;
    }
}

void Solver::main(std::optional<ProgramParamsVec> const &params) {
    if (!block_main_ && scripts_->callable("main", 0)) {
        if (mode_ == AppMode::solve) {
            clasp_->enableProgramUpdates();
        }
        scripts_->main(*this, params);
    } else {
        if (mode_ == AppMode::parse) {
            output_unprocessed_program(std::cout);
            return;
        }
        if (mode_ == AppMode::rewrite) {
            output_program(std::cout);
            return;
        }
        bool inc = intersects(includes_, BuiltinIncludes::incmode);
        if (mode_ == AppMode::solve && (params->size() >= 2 || inc)) {
            clasp_->enableProgramUpdates();
        }
        if (inc) {
            incmode_();
        } else if (params) {
            for (auto const &param : *params) {
                ground(param, nullptr);
                if (mode_ == AppMode::solve) {
                    solve(nullptr);
                }
            }
        } else {
            ground(Clingo::Input::ProgramParamVec{{grd_.store().string("base"), {}}}, nullptr);
            if (mode_ == AppMode::solve) {
                solve(nullptr)->get();
            }
        }
    }
}

auto Solver::map_model(Clasp::Model const &mdl) -> Model & {
    assert(mdl_);
    // NOLINTNEXTLINE
    static_cast<ModelImpl &>(*mdl_).set_model(&mdl, false);
    return *mdl_;
}

auto Solver::solve(UEventHandler handler, PrgLitSpan assumptions, SolveMode mode) -> USolveHandle {
    if (mode_ == AppMode::solve) {
        auto guard = unlock_guard{lock_};
        if (state_ == State::solved || state_ == State::initial) {
            // we inject an emtpy ground
            ground(Input::ProgramParamVec{}, nullptr);
        }
        state_ = State::solved;
        clasp_->asp()->addAssumption(assumptions);
        clasp_->prepare();
        theory_->reset();
        if (mdl_ == nullptr) {
            mdl_ = std::make_unique<ModelImpl>(grd_.base(), *clasp_);
        }
        // NOLINTNEXTLINE
        return std::make_unique<SolveHandleImpl>(lock_, grd_.log(), static_cast<ModelImpl &>(*mdl_), mode,
                                                 std::move(handler));
    }
    return std::make_unique<SolveHandleFixed>();
}

void Solver::join(Input::UnprocessedProgram const &prg) {
    grd_.join(prg);
}

void Solver::parse(std::string_view str) {
    if (mode_ == AppMode::solve) {
        auto bck = ProgramBackendAdapter{grd_.base(), *clasp_->asp()};
        auto thy = TheoryBackendAdapter{grd_.store(), *theory_};
        // TODO : pass bck and thy to parse
        includes_ |= grd_.parse(str, scripts_);
    } else {
        includes_ |= grd_.parse(str, scripts_);
    }
}

void Solver::parse(std::span<std::string_view const> const &files) {
    if (mode_ == AppMode::solve) {
        auto bck = ProgramBackendAdapter{grd_.base(), *clasp_->asp()};
        auto thy = TheoryBackendAdapter{grd_.store(), *theory_};
        // TODO : pass bck and thy to parse
        includes_ |= grd_.parse(files, scripts_);
    } else {
        includes_ |= grd_.parse(files, scripts_);
    }
}

void Solver::add_const(String name, Symbol value) {
    grd_.add_const(name, value);
}

auto Solver::const_map() -> Input::ConstMap const & {
    return grd_.const_map();
}

void Solver::ground(Input::ProgramParamVec const &params, Ground::ScriptCallback *ctx) {
    prepare_();
    std::ignore = grd_.ground(params, ctx != nullptr ? ctx : scripts_);
}

void Solver::output_unprocessed_program(std::ostream &out) {
    grd_.output_unprocessed_program(out);
}

void Solver::output_program(std::ostream &out) {
    grd_.output_program(out);
}

void Solver::register_propagator(UPropagator propagator) {
    if (mode_ == AppMode::solve) {
        clasp_facade().registerPropagator(*propagator, true);
        if (propagator->hasHeuristic()) {
            clasp_facade().registerHeuristic(*propagator.get());
        }
        propagators_.emplace_back(std::move(propagator));
    }
}

namespace {

class BackendHandleImpl : public BackendHandle {
  public:
    BackendHandleImpl(Grounder &grounder, ProgramBackend &backend, Clasp::Asp::LogicProgram &prg,
                      Output::TheoryData &theory)
        : grounder_{&grounder}, backend_{&backend}, prg_{&prg}, theory_{&theory} {}
    ~BackendHandleImpl() override { close(); }

  private:
    auto do_program() -> Clasp::Asp::LogicProgram & override { return *prg_; }

    auto do_theory() -> Output::TheoryData & override { return *theory_; }

    auto do_store() -> SymbolStore & override { return grounder_->store(); }

    auto do_add_atom(Symbol atom) -> prg_lit_t override {
        if (auto sig = atom.signature(); sig && grounder_ != nullptr) {
            auto &base = grounder_->base().add_base(*sig);
            auto ret = base.add(atom, Ground::StateAtom::derived,
                                [this]() { return static_cast<size_t>(backend_->next_lit()); })
                           .first;
            auto lit = static_cast<prg_lit_t>(ret.value().id);
            if (ret.value().state != Ground::StateAtom::fact) {
                added_.emplace_back(lit, ret.key());
            }
            return lit;
        }
        throw std::runtime_error("invalid atom");
    }

    void do_close() override {
        for (auto const &[lit, sym] : added_) {
            assert(lit > 0);
            if (prg_->isFact(lit)) {
                auto sig = sym.signature();
                assert(sig.has_value());
                auto *base = grounder_->base().get_base(*sig);
                assert(base != nullptr);
                auto it = base->find(sym);
                assert(it.has_value() && it->value().state != Ground::StateAtom::unknown);
                it->value().state = Ground::StateAtom::fact;
            }
        }
        if (grounder_ != nullptr) {
            std::ignore = std::exchange(grounder_, nullptr)->ground({});
        }
    }

    Grounder *grounder_;
    ProgramBackend *backend_;
    Clasp::Asp::LogicProgram *prg_;
    Output::TheoryData *theory_;
    std::vector<std::pair<prg_lit_t, Symbol>> added_;
};

} // namespace

auto Solver::backend() -> UBackendHandle {
    if (backend_ != nullptr && clasp_->asp() != nullptr && theory_ != nullptr) {
        prepare_();
        return std::make_unique<BackendHandleImpl>(grd_, *backend_, *clasp_->asp(), *theory_);
    }
    throw std::runtime_error("not in solving mode");
}

void Solver::prepare_() {
    if (mode_ == AppMode::solve) {
        clasp_->update();
    }
    state_ = State::grounded;
}

} // namespace Clingo::Control
