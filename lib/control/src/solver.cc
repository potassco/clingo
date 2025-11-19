#include "clingo/control/solver.hh"

#include "clingo/output/text.hh"

#include <clasp/solver.h>

#include <potassco/aspif_text.h>
#include <potassco/theory_data.h>

#include <future>
#include <utility>

// #define DEBUG_BACKEND
#ifdef DEBUG_BACKEND
#include <clingo/util/print.hh>
#include <iostream>
#endif

namespace CppClingo::Control {

namespace {

//! Implementation of the backend interface.
class ProgramBackendImpl : public ProgramBackend {
  public:
    ProgramBackendImpl(Clasp::Asp::LogicProgram &prg, TermBaseMap &terms) : prg_{&prg}, terms_{&terms} {}

  private:
    void do_preamble([[maybe_unused]] unsigned major, [[maybe_unused]] unsigned minor,
                     [[maybe_unused]] unsigned revision, [[maybe_unused]] bool incremental) override {
        assert(incremental == prg_->isIncremental());
    }
    void do_end() override {}

    auto do_next_lit() -> prg_lit_t override {
        if (auto lit = prg_->newAtom(); std::cmp_less_equal(lit, prg_lit_max)) {
            return static_cast<prg_lit_t>(lit);
        }
        throw std::range_error("literals number of literals exhausted");
    }

    auto do_fact_lit() -> std::optional<prg_lit_t> override {
        // return the cached fact literal
        if (fact_lit_ != 0) {
            return fact_lit_;
        }
        // try to find a fact literal
        if (auto atom = prg_->factAtom(); atom) {
            fact_lit_ = Potassco::lit(atom);
            return fact_lit_;
        }
        // report that there is no fact literal (very unlikely)
        return std::nullopt;
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
        static_assert(static_cast<unsigned>(CppClingo::HeuristicType::init) ==
                      static_cast<unsigned>(Clasp::DomModType::init));
        static_assert(static_cast<unsigned>(CppClingo::HeuristicType::factor) ==
                      static_cast<unsigned>(Clasp::DomModType::factor));
        static_assert(static_cast<unsigned>(CppClingo::HeuristicType::false_) ==
                      static_cast<unsigned>(Clasp::DomModType::false_));
        static_assert(static_cast<unsigned>(CppClingo::HeuristicType::level) ==
                      static_cast<unsigned>(Clasp::DomModType::level));
        static_assert(static_cast<unsigned>(CppClingo::HeuristicType::sign) ==
                      static_cast<unsigned>(Clasp::DomModType::sign));
        static_assert(static_cast<unsigned>(CppClingo::HeuristicType::true_) ==
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

    void do_show_term(Symbol sym, PrgLitSpan body) override {
        auto id = terms_->add(sym, [&, this]() {
            buf_.reset();
            buf_ << sym;
            return prg_->newShowTerm(buf_.view());
        });
        prg_->addShowTerm(id, body);
#ifdef DEBUG_BACKEND
        std::cerr << "#show " << sym << " : " << Util::p_range(body, ", ") << ".\n";
#endif
    }

    void do_show_term(Symbol sym, prg_id_t id) override {
        buf_.reset();
        buf_ << sym;
        terms_->add(sym, id);
        prg_->newShowTerm(buf_.view(), id);
    }

    void do_show_term(prg_id_t id, PrgLitSpan body) override { prg_->addShowTerm(id, body); }

    void do_show_atom(Symbol sym, prg_lit_t lit) override {
        assert(lit > 0);
        buf_.reset();
        buf_ << sym;
        prg_->addAtomOutput(lit, buf_.view());
#ifdef DEBUG_BACKEND
        std::cerr << "#show " << sym << " : " << lit << ".\n";
#endif
    }

    Util::OutputBuffer buf_;
    Potassco::RuleBuilder bld_;
    Clasp::Asp::LogicProgram *prg_;
    TermBaseMap *terms_;
    prg_lit_t fact_lit_ = 0;
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

    void do_end() override {}

    Clasp::Asp::LogicProgram *prg_;
};

//! Implementation of the theory backend for aspif parser.
class TheoryBackendAdapter : public TheoryBackend {
  public:
    TheoryBackendAdapter(SymbolStore &store, Output::TheoryData &data) : store_{&store}, data_{&data} {}

  private:
    //! Get a remapped term.
    auto term_(prg_id_t id) -> prg_id_t {
        auto it = term_map_.find(id);
        if (it != term_map_.end()) {
            return it->second;
        }
        throw std::runtime_error{"unknown term id"};
    }

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
    auto elem_(prg_id_t id) -> prg_id_t {
        auto it = elem_map_.find(id);
        if (it != elem_map_.end()) {
            return it->second;
        }
        throw std::runtime_error{"unknown element id"};
    }

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
        auto lit = [lit_or_zero]() { return lit_or_zero; };
        if (guard) {
            data_->atom(lit, term_(name), elem_(elems), std::pair{term_(guard->first), term_(guard->second)});
        } else {
            data_->atom(lit, term_(name), elem_(elems), std::nullopt);
        }
    }

    void do_end() override {
        // clear the mappings but not the theory data
        term_map_.clear();
        elem_map_.clear();
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
    ModelImpl(Ground::Bases const &bases, TermBaseMap &terms, Clasp::ClaspFacade &clasp)
        : bases_{&bases}, terms_{&terms}, clasp_{&clasp} {
        clasp_->ctx.output.add(extend_);
    }
    ~ModelImpl() override { clasp_->ctx.output.remove(extend_); }

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
            for (auto const &[term, term_id] : *terms_) {
                if (clasp_program().isShowTermTrue(*mdl_, term_id)) {
                    res.emplace_back(*term);
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

    [[nodiscard]] auto do_term_base() const -> TermBaseMap const & override { return *terms_; }

    [[nodiscard]] auto do_clasp_program() const -> Clasp::Asp::LogicProgram const & override {
        return clasp_->asp() != nullptr ? *clasp_->asp() : throw std::runtime_error("not in solving mode");
    }

    //! Map the given program literal to its solver literal.
    [[nodiscard]] auto solver_literal(std::integral auto lit) const -> Clasp::Literal {
        return Clasp::Asp::solverLiteral(clasp_program(), static_cast<prg_lit_t>(lit));
    }

    Ground::Bases const *bases_;
    TermBaseMap *terms_;
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
class SolveEventHandlerAdapter : public Clasp::EventHandler {
  public:
    //! Construct a solve event handler adapter.
    //!
    //! This initializes the underlying solve handle, which in turn starts
    //! solving.
    SolveEventHandlerAdapter(CallbackLock &lock, Logger &logger, ModelImpl &mdl, Control::USolveEventHandler eh)
        : lock_{&lock}, logger_{&logger}, mdl_{&mdl}, eh_{std::move(eh)} {}

    //! Intercept and report models.
    auto onModel([[maybe_unused]] Clasp::Solver const &slv, Clasp::Model const &mdl) -> bool override {
        if (eh_) {
            auto guard = std::scoped_lock{*lock_};
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
                    auto guard = std::scoped_lock{*lock_};
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
            auto guard = std::scoped_lock{*lock_};
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
            auto guard = std::scoped_lock{*lock_};
            eh_->on_unsat(bound_);
        }
        return true;
    }

    //! Report unsatisfiable cores.
    void onCore(Potassco::LitSpan core) {
        if (eh_) {
            auto guard = std::scoped_lock{*lock_};
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
    Control::USolveEventHandler eh_;
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
    SolveHandleImpl(CallbackLock &lock, Logger &log, ModelImpl &mdl, SolveMode mode, USolveEventHandler eh,
                    std::function<void()> simplify)
        : eh_{lock, log, mdl, std::move(eh)}, hnd_{mdl.clasp().solve(convert(mode), {}, &eh_)},
          simplify_{std::move(simplify)} {}

    ~SolveHandleImpl() override {
        try {
            cancel();
        } catch (std::exception &e) {
            fprintf(stderr, "panic: shutting down the solve handle failed with %s\n", e.what());
            std::terminate();
        }
    }

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
        simplify_();
    }
    void do_resume() override {
        auto guard = unlock_guard{eh_.get_lock()};
        hnd_.resume();
    }
    auto do_model() -> Model const * override {
        auto guard = unlock_guard{eh_.get_lock()};
        return eh_.model().set_model(hnd_.model(), false) ? &eh_.model() : nullptr;
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

    SolveEventHandlerAdapter eh_;
    Clasp::ClaspFacade::SolveHandle hnd_;
    std::function<void()> simplify_;
    Potassco::LitVec mutable core_;
};

//! Integrate facts and inform the grounder about updated domains.
void end_step(std::vector<std::pair<prg_lit_t, SharedSymbol>> &added, Clasp::Asp::LogicProgram &prg, Grounder &grd) {
    for (auto const &[lit, sym] : added) {
        assert(lit > 0);
        auto sig = sym->signature();
        assert(sig.has_value());
        grd.mark_sig(*sig);
        if (prg.isFact(lit)) {
            auto *base = grd.base().get_base(*sig);
            assert(base != nullptr);
            auto it = base->find(*sym);
            assert(it.has_value() && it->value().state != Ground::StateAtom::unknown);
            it->value().state = Ground::StateAtom::fact;
        }
    }
    std::ignore = grd.ground({});
}

class BackendHandleImpl : public BackendHandle {
  public:
    BackendHandleImpl(Grounder &grounder, ProgramBackend &backend, Clasp::Asp::LogicProgram &prg,
                      Output::TheoryData &theory)
        : grd_{&grounder}, backend_{&backend}, prg_{&prg}, theory_{&theory} {}
    ~BackendHandleImpl() override { close(); }

  private:
    auto do_program() -> Clasp::Asp::LogicProgram & override { return *prg_; }

    auto do_theory() -> Output::TheoryData & override { return *theory_; }

    auto do_store() -> SymbolStore & override { return grd_->store(); }

    auto do_add_atom(Symbol atom) -> prg_lit_t override {
        if (auto sig = atom.signature(); sig && grd_ != nullptr) {
            auto &base = grd_->base().add_base(*sig);
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
        if (auto *grd = std::exchange(grd_, nullptr); grd != nullptr) {
            end_step(added_, *prg_, *grd);
        }
    }

    Grounder *grd_;
    ProgramBackend *backend_;
    Clasp::Asp::LogicProgram *prg_;
    Output::TheoryData *theory_;
    std::vector<std::pair<prg_lit_t, SharedSymbol>> added_;
};

} // namespace

//! Implementation of the program backend for aspif parser.
//!
//! Note the similarity to the backend adapter.
class Solver::ProgramBackendAdapter : public ProgramBackendImpl {
  public:
    ProgramBackendAdapter(Solver &solver)
        : ProgramBackendImpl{*solver.clasp_facade().asp(), solver.terms_}, solver_{&solver} {}

  private:
    //! Prepare the program to add aspif statements.
    void do_preamble([[maybe_unused]] unsigned major, [[maybe_unused]] unsigned minor,
                     [[maybe_unused]] unsigned revision, bool incremental) override {
        if (incremental) {
            solver_->enable_updates_();
        }
        solver_->prepare_();
    }

    //! Add delayed assumptions.
    void do_assume(PrgLitSpan literals) override {
        assumptions_.insert(assumptions_.end(), literals.begin(), literals.end());
    }

    //! Integrate facts and inform the grounder about updated domains.
    void do_end() override {
        solver_->clasp_facade().asp()->removeAssumption();
        solver_->clasp_facade().asp()->addAssumption(assumptions_);
        end_step(added_, *solver_->clasp_->asp(), solver_->grd_);
        added_.clear();
        assumptions_.clear();
    }

    //! Show the atom with the given symbol and literal.
    //!
    //! Shown atoms are not passed through to the backend here. Instead, they
    //! are added to the grounder's domain in a similar way as the backend
    //! does. However, for the aspif reader, we check that the atom has indeed
    //! just been added and received the same literal as the one given in the
    //! aspif output.
    void do_show_atom(Symbol sym, prg_lit_t lit) override {
        auto sig = sym.signature();
        if (!sig) {
            throw std::runtime_error{"unexpected symbol for atom in aspif file"};
        }
        auto &base = solver_->grd_.base().add_base(*sig);
        auto it = base.add(sym, Ground::StateAtom::derived, [lit]() { return lit; }).first;
        if (std::cmp_not_equal(it->second.id, lit)) {
            throw std::runtime_error{"redefinition of atom in aspif file"};
        }
        added_.emplace_back(lit, sym);
    }

    std::vector<std::pair<prg_lit_t, SharedSymbol>> added_;
    PrgLitVec assumptions_;
    Solver *solver_;
};

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
        CLINGO_REPORT_LOC(log, error, loc) << "script support for '" << name << "' not available";
        throw std::runtime_error("script support not available");
    }
}

void Scripts::main(Solver &slv) {
    for (auto const &script : scripts_) {
        if (script.second->callable("main", 0)) {
            script.second->main(slv);
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

void SymbolTable::init(CppClingo::Control::BaseView &view, std::ostream &out) {
    buf_.clear();
    view_ = &view;
    out_ = &out;
}

void SymbolTable::begin_step() {
    for (auto const &[sym, id] : view_->term_base()) {
        auto &state = output(*sym);
        if (state.term == 0) {
            state.term = 1;
            *out_ << "4 1 " << state.index << " " << id << "\n";
        }
    }
}

void SymbolTable::end_step() {
    for (auto const &[sig, base] : view_->bases().atoms()) {
        // NOTE: at this point non-empty bases should be either shown or not.
        if (base->num_shown() == 0) {
            continue;
        }
        for (size_t i = 0, e = base->size(); i != e; ++i) {
            auto it = base->nth(i);
            auto &state = output(it.key());
            if (state.atom == 0) {
                state.atom = 1;
                *out_ << "4 0 " << state.index << " " << it.value().id << "\n";
            }
        }
    }
}

auto SymbolTable::output(CppClingo::Symbol const &sym) -> State & {
    auto [it, ins] = done_.try_emplace(sym);
    if (ins) {
        switch (sym.type()) {
            case CppClingo::SymbolType::inf: {
                auto id = it.value().index = ids_++;
                *out_ << "4 3 " << id << " 0\n";
                break;
            }
            case CppClingo::SymbolType::sup: {
                auto id = it.value().index = ids_++;
                *out_ << "4 3 " << id << " 1" << "\n";
                break;
            }
            case CppClingo::SymbolType::number: {
                auto id = it.value().index = ids_++;
                *out_ << "4 4 " << id << " " << sym.num() << "\n";
                break;
            }
            case CppClingo::SymbolType::string: {
                auto id = it.value().index = ids_++;
                auto str = sym.str().view();
                *out_ << "4 5 " << id << " " << str.size() << " " << str << "\n";
                break;
            }
            case CppClingo::SymbolType::tuple: {
                auto args = sym.args();
                auto size = static_cast<std::ptrdiff_t>(buf_.size());
                for (auto const &arg : args) {
                    buf_.push_back(output(arg).index);
                }
                // NOTE: the iterator might have been invalidated above
                it = done_.find(sym);
                auto id = it.value().index = ids_++;
                *out_ << "4 6 " << id << " " << args.size();
                for (auto const &arg : std::span{buf_.begin() + size, buf_.end()}) {
                    *out_ << " " << arg;
                }
                buf_.resize(size);
                *out_ << "\n";
                break;
            }
            case CppClingo::SymbolType::function: {
                auto args = sym.args();
                auto size = static_cast<std::ptrdiff_t>(buf_.size());
                // NOTE: conversion from string to symbol is fast
                size_t name = output(CppClingo::SymbolStore::str_ref(sym.name())).index;
                for (auto const &arg : args) {
                    buf_.push_back(output(arg).index);
                }
                // NOTE: the iterator might have been invalidated above
                it = done_.find(sym);
                auto id = it.value().index = ids_++;
                *out_ << "4 7 " << id << " " << (sym.has_classical_sign() ? 1 : 0) << " " << name << " " << args.size();
                for (auto const &arg : std::span{buf_.begin() + size, buf_.end()}) {
                    *out_ << " " << arg;
                }
                buf_.resize(size);
                *out_ << "\n";
                break;
            }
        }
    }
    return it.value();
}

Solver::Solver(Clasp::ClaspFacade &clasp, Clasp::Cli::ClaspCliConfig &clasp_config, Logger &log, SymbolStore &store,
               Scripts &scripts, Input::RewriteOptions ropts, SolverOptions sopts, FILE *out)
    : clasp_{&clasp}, config_{clasp_config}, buf_{out}, out_{make_output_(store, sopts.mode)},
      grd_{log, store, ropts, *out_}, scripts_{&scripts}, opts_{sopts} {
}

auto Solver::make_output_(SymbolStore &store, AppMode mode) -> UOutputStm {
    switch (mode) {
        case AppMode::solve: {
            backend_ = std::make_unique<ProgramBackendImpl>(*clasp_->asp(), terms_);
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
        fprintf(stderr, "panic: %s\n", e.what());
        std::terminate();
    }
}

void Solver::main(std::span<std::string_view const> const &files) {
    parse(files);
    main();
}

void Solver::incmode_() {
    auto &store = grd_.store();
    auto part_check = store.string("check");
    auto part_step = store.string("step");
    auto part_base = store.string("base");
    auto part_query = store.string("query");

    parse(R"(#program check(t).
#external query(t). [true]
#program step(t).
#external query(t-1). [release]
)");

    size_t step = 0;
    auto ret = SolveResult::empty;
    auto cont = [&]() {
        if (opts_.imax && step >= *opts_.imax) {
            return false;
        }
        if (step == 0 || step < opts_.imin) {
            return true;
        }
        if (opts_.istop == IStop::sat && intersects(ret, SolveResult::satisfiable)) {
            return false;
        }
        if (opts_.istop == IStop::unsat && intersects(ret, SolveResult::unsatisfiable)) {
            return false;
        }
        if (opts_.istop == IStop::unknown && !intersects(ret, SolveResult::satisfiable | SolveResult::unsatisfiable)) {
            return false;
        }
        return true;
    };
    while (cont()) {
        using Param = CppClingo::Input::ProgramParam;
        CppClingo::Input::ProgramParamVec parts;
        auto num = CppClingo::SymbolStore::num(Util::safe_cast<int32_t>(step));
        parts.emplace_back(Param{part_check, {num}});
        if (step > 0) {
            parts.emplace_back(Param{part_step, {num}});
        } else {
            parts.emplace_back(Param{part_base, {}});
        }
        ground(parts, nullptr);
        if (opts_.mode == AppMode::solve) {
            ret = solve()->get();
        }
        step += 1;
    }
}

void Solver::enable_updates_() {
    if (opts_.mode == AppMode::solve) {
        if (opts_.single_shot) {
            clasp_->keepProgram();
        } else if (!clasp_->incremental()) {
            clasp_->enableProgramUpdates();
        }
    }
}

void Solver::main() {
    if (!block_main_ && scripts_->callable("main", 0)) {
        enable_updates_();
        scripts_->main(*this);
    } else {
        if (opts_.mode == AppMode::parse) {
            output_unprocessed_program(std::cout);
            return;
        }
        if (opts_.mode == AppMode::rewrite) {
            output_program(std::cout);
            return;
        }
        bool inc = intersects(includes_, BuiltinIncludes::incmode);
        if (opts_.mode == AppMode::solve && inc) {
            enable_updates_();
        }
        if (inc) {
            incmode_();
        } else {
            auto const &parts = grd_.get_parts();
            ground(parts ? parts->elems() : ProgramParamVec{{grd_.store().string("base"), {}}}, nullptr);
            if (opts_.mode == AppMode::solve) {
                solve(nullptr);
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

auto Solver::solve(USolveEventHandler handler, PrgLitSpan assumptions, SolveMode mode) -> USolveHandle {
    if (opts_.mode == AppMode::solve) {
        auto guard = unlock_guard{lock_};
        if (state_ == State::solved || state_ == State::initial) {
            // we inject an emtpy ground
            ground(Input::ProgramParamVec{}, nullptr);
        }
        state_ = State::solved;
        clasp_->asp()->addAssumption(assumptions);
        if (clasp_->prepare()) {
            theory_->reset();
            if (mdl_ == nullptr) {
                mdl_ = std::make_unique<ModelImpl>(grd_.base(), terms_, *clasp_);
            } else {
                // NOLINTNEXTLINE
                static_cast<ModelImpl *>(mdl_.get())->set_model(nullptr);
            }
            // NOLINTNEXTLINE
            return std::make_unique<SolveHandleImpl>(lock_, grd_.log(), static_cast<ModelImpl &>(*mdl_), mode,
                                                     std::move(handler), [this]() { simplify_(); });
        }
        theory_->reset();
    }
    return std::make_unique<SolveHandleFixed>();
}

void Solver::join(Input::UnprocessedProgram const &prg) {
    grd_.join(prg);
}

void Solver::parse_with(std::function<void(ProgramBackend *, TheoryBackend *)> cb) {
    if (opts_.mode == AppMode::solve) {
        auto bck = ProgramBackendAdapter{*this};
        auto thy = TheoryBackendAdapter{grd_.store(), *theory_};
        std::invoke(std::move(cb), &bck, &thy);
    } else {
        std::invoke(std::move(cb), nullptr, nullptr);
    }
}

void Solver::parse(std::string_view str) {
    includes_ |= grd_.parse(str, scripts_);
}

void Solver::parse(std::span<std::string_view const> const &files) {
    parse_with([&](ProgramBackend *bck, TheoryBackend *thy) { includes_ |= grd_.parse(files, scripts_, bck, thy); });
}

void Solver::add_const(String name, Symbol value) {
    grd_.add_const(name, value);
}

auto Solver::const_map() -> Input::ConstMap const & {
    return grd_.const_map();
}

class GroundHandle::Impl {
  public:
    Impl() = default;

    Impl(Impl const &) = delete;
    Impl(Impl &&other) noexcept = delete;
    auto operator=(Impl const &) -> Impl & = delete;
    auto operator=(Impl &&other) noexcept -> Impl & = delete;

    void start(Solver &slv, Input::ProgramParamVec params, UGroundEventHandler handler) {
        assert(!future_.valid());
        future_ =
            std::async(std::launch::async, [this, &slv, handler = std::move(handler), params = std::move(params)]() {
                try {
                    auto res = slv.ground(params, handler.get(), &stop_);
                    if (handler) {
                        handler->finish(res);
                    }
                    return res;
                } catch (...) {
                    if (handler) {
                        handler->finish(GroundResult::interrupted);
                    }
                    throw;
                }
            });
    }

    [[nodiscard]] auto wait(double timeout) -> bool {
        if (!future_.valid()) {
            return true;
        }
        if (timeout < 0) {
            future_.wait();
            return true;
        }
        return future_.wait_for(std::chrono::duration<double>(timeout)) == std::future_status::ready;
    }

    [[nodiscard]] auto get() -> GroundResult {
        if (future_.valid()) {
            res_ = future_.get();
        }
        return res_;
    }

    void cancel() {
        if (future_.valid()) {
            stop_.request_stop();
            future_.wait();
        }
    }

    ~Impl() noexcept {
        try {
            cancel();
        } catch (std::exception const &e) {
            fprintf(stderr, "panic: %s\n", e.what());
            std::terminate();
        }
    }

  private:
    std::future<GroundResult> future_;
    GroundResult res_ = GroundResult::interrupted;
    Util::StopFlag stop_;
};

GroundHandle::GroundHandle(Solver &solver, Input::ProgramParamVec params, UGroundEventHandler handler)
    : impl_{std::make_unique<Impl>()} {
    impl_->start(solver, std::move(params), std::move(handler));
}

GroundHandle::~GroundHandle() noexcept = default;

GroundHandle::GroundHandle(GroundHandle &&other) noexcept = default;

auto GroundHandle::operator=(GroundHandle &&other) noexcept -> GroundHandle & = default;

auto GroundHandle::wait(double timeout) -> bool {
    return impl_->wait(timeout);
}

void GroundHandle::cancel() {
    impl_->cancel();
}

auto GroundHandle::get() -> GroundResult {
    return impl_->get();
}

auto Solver::start_ground(Input::ProgramParamVec params, UGroundEventHandler handler) -> GroundHandle {
    return GroundHandle{*this, std::move(params), std::move(handler)};
}

auto Solver::ground(Input::ProgramParamVec const &params, Ground::ScriptCallback *ctx, Util::StopFlag *stop)
    -> GroundResult {
    auto res = GroundResult::ok;
    if (opts_.mode >= AppMode::ground) {
        prepare_();
        res = grd_.ground(params, ctx != nullptr ? ctx : scripts_, stop);
        clasp_->ctx.report(Grounded{params});
    }
    return res;
}

void Solver::output_unprocessed_program(std::ostream &out) {
    grd_.output_unprocessed_program(out);
}

void Solver::output_program(std::ostream &out) {
    grd_.output_program(out);
}

void Solver::register_propagator(UPropagator propagator) {
    if (opts_.mode == AppMode::solve) {
        clasp_facade().registerPropagator(*propagator, true);
        if (propagator->hasHeuristic()) {
            clasp_facade().registerHeuristic(*propagator.get());
        }
        propagators_.emplace_back(std::move(propagator));
    }
}

auto Solver::backend() -> UBackendHandle {
    if (backend_ != nullptr && clasp_->asp() != nullptr && theory_ != nullptr) {
        prepare_();
        return std::make_unique<BackendHandleImpl>(grd_, *backend_, *clasp_->asp(), *theory_);
    }
    throw std::runtime_error("not in solving mode");
}

void Solver::simplify_() {
    if (opts_.mode != AppMode::solve || !clasp_->incremental() || !clasp_->ctx.ok()) {
        return;
    }
    auto value = [clasp = clasp_](prg_lit_t lit) {
        auto const &prg = *clasp->asp();
        // NOTE: externals are not simplified because they must be available in
        // domains until released.
        if (prg.isExternal(std::abs(lit))) {
            return TruthValue::unknown;
        }
        auto slit = Clasp::Asp::solverLiteral(prg, lit);
        auto val = clasp->ctx.master()->topValue(slit.var());
        if (val == Clasp::value_free) {
            return TruthValue::unknown;
        }
        return val == trueValue(slit) ? TruthValue::top : TruthValue::bot;
    };
    CLINGO_REPORT(grd_.log(), debug) << "simplify...";
    size_t rem = 0;
    size_t fact = 0;
    for (auto const &base : grd_.base().atoms()) {
        base.second->simplify(value, rem, fact);
    }
    for (auto const &base : grd_.base().projected()) {
        base.second->p_base().simplify(value, rem, fact);
    }
    CLINGO_REPORT(grd_.log(), debug) << "  removed: " << rem;
    CLINGO_REPORT(grd_.log(), debug) << "  fact: " << fact;
    out_->simplify(value);
}

void Solver::prepare_() {
    if (opts_.mode == AppMode::solve) {
        clasp_->update();
    }
    state_ = State::grounded;
}

} // namespace CppClingo::Control
