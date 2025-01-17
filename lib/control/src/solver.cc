#include <clingo/control/solver.hh>

#include <clingo/output/text.hh>

#include <clingo/util/checked_math.hh>
#include <clingo/util/enum.hh>

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
class BackendImpl : public Output::Backend {
  public:
    BackendImpl(Clasp::Asp::LogicProgram &prg) : prg_{&prg} {}

  private:
    auto do_next_lit() -> Output::lit_t override {
        if (lit_ < Output::lit_max) {
            return ++lit_;
        }
        throw std::range_error("literals number of literals exhausted");
    }

    void do_rule(Output::LitSpan head, Output::LitSpan body, bool choice) override {
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

    void do_bd_aggr(Output::lit_t head, Output::WeightedLitSpan body, Output::weight_t bound) override {
        assert(head > 0);
        assert(bound > 0);
        bld_.clear();
        bld_.start();
        bld_.addHead(head);
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

    void do_edge(Output::id_t u, Output::id_t v, Output::LitSpan body) override { prg_->addAcycEdge(u, v, body); }

    void do_heuristic(Output::lit_t atom, int32_t weight, int32_t prio, HeuristicType type,
                      Output::LitSpan body) override {
        assert(atom > 0);
        auto dmod = [type] {
            switch (type) {
                case Clingo::HeuristicType::init: {
                    return Clasp::DomModType::init;
                }
                case Clingo::HeuristicType::factor: {
                    return Clasp::DomModType::factor;
                }
                case Clingo::HeuristicType::false_: {
                    return Clasp::DomModType::false_;
                }
                case Clingo::HeuristicType::level: {
                    return Clasp::DomModType::level;
                }
                case Clingo::HeuristicType::sign: {
                    return Clasp::DomModType::sign;
                }
                case Clingo::HeuristicType::true_: {
                    return Clasp::DomModType::true_;
                }
            }
            Util::unreachable();
        }();
        prg_->addDomHeuristic(atom, dmod, weight, prio, body);
    }

    void do_external(Output::lit_t atom, ExternalType type) override {
        auto value = [type] {
            switch (type) {
                case ExternalType::true_: {
                    return Potassco::TruthValue::true_;
                }
                case ExternalType::false_: {
                    return Potassco::TruthValue::false_;
                }
                case ExternalType::free: {
                    return Potassco::TruthValue::free;
                }
            }
            Util::unreachable();
        }();
        prg_->addExternal(atom, value);
    }

    void do_project(Output::lit_t atom) override { prg_->addProject(std::array{static_cast<Potassco::Atom_t>(atom)}); }

    void do_minimize(Output::lit_t lit, Output::weight_t weight, Output::weight_t priority) override {
        prg_->addMinimize(priority, std::array{Potassco::WeightLit{lit, weight}});
    }

    void do_show(Symbol sym, Output::LitSpan body) override {
        buf_.reset();
        buf_ << sym;
        prg_->addOutput(buf_.c_str(), prg_->newCondition(body));
#ifdef DEBUG_BACKEND
        std::cerr << "#show " << sym << " : " << Util::p_range(body, ", ") << ".\n";
#endif
    }

    void do_theory_num(Output::id_t id, int32_t num) override { prg_->theoryData().addTerm(id, num); }

    void do_theory_str(Output::id_t id, char const *str) override { prg_->theoryData().addTerm(id, str); }

    void do_theory_fun(Output::id_t id, Output::id_t name, Output::IdSpan args) override {
        assert(!args.empty());
        prg_->theoryData().addTerm(id, name, args);
    }

    void do_theory_tup(Output::id_t id, TheoryTermTupleType type, Output::IdSpan args) override {
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

    void do_theory_elem(Output::id_t id, Output::IdSpan terms, Output::LitSpan cond) override {
        prg_->theoryData().addElement(id, terms, prg_->newCondition(cond));
    }

#ifdef DEBUG_BACKEND
    void print(Potassco::TheoryTerm const &term) {
        switch (term.type()) {
            case Potassco::Theory_t::number: {
                std::cerr << term.number();
                return;
            }
            case Potassco::Theory_t::symbol: {
                std::cerr << term.symbol();
                return;
            }
            case Potassco::Theory_t::compound: {
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

    void do_theory_atom(Output::lit_t lit_or_zero, Output::id_t name, Output::IdSpan elems,
                        std::optional<std::pair<Output::id_t, Output::id_t>> guard) override {
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

    Output::lit_t lit_ = 0;
    Util::OutputBuffer buf_;
    Potassco::RuleBuilder bld_;
    Clasp::Asp::LogicProgram *prg_;
};

//! Implementation of the model interface.
class ModelImpl : public Model, private SolveControl {
  public:
    ModelImpl(BaseMap const &base, Clasp::ClaspFacade &clasp) : base_{&base}, clasp_{&clasp} {}

    //! Sets the given model returing true if it is not null.
    auto set_model(Clasp::Model const *mdl) -> bool { return (mdl_ = mdl) != nullptr; }

    //! Sets the last model returning true if it is not null.
    auto set_last() -> bool {
        return (mdl_ = clasp_->solved() && clasp_->summary().sat() && clasp_->summary().model() != nullptr
                           ? clasp_->summary().model()
                           : nullptr) != nullptr;
    }

    //! Get the underlying clasp facade.
    auto clasp() -> Clasp::ClaspFacade const & { return *clasp_; }

  private:
    void do_symbols(SymbolSelectFlags type, SymbolVec &res) const override {
        assert(mdl_ != nullptr);
        // TODO: implement me
        if ((type & (SymbolSelectFlags::theory | SymbolSelectFlags::complement)) != SymbolSelectFlags::none) {
            throw std::logic_error("implement me: theory and complement selection modes");
        }
        if (test(type, SymbolSelectFlags::atoms)) {
            for (auto const &base : *base_) {
                for (size_t i = 0, e = base.second->size(); i != e; ++i) {
                    auto [sym, state] = *base.second->nth(i);
                    auto lit = solver_literal(state.id);
                    if (mdl_->isTrue(lit)) {
                        res.emplace_back(sym);
                    }
                }
            }
        }
        if (test(type, SymbolSelectFlags::terms)) {
            throw std::logic_error("implement me: store a term map somewhere");
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
        auto it = base_->find(std::tuple{sym.name(), sym.args().size(), sym.has_classical_sign()});
        if (it == base_->end()) {
            return false;
        }
        auto jt = it->second->find(sym);
        if (!jt) {
            return false;
        }
        return mdl_->isTrue(solver_literal(jt->value().id));
    }

    [[nodiscard]] auto do_is_true(Output::lit_t lit) const -> bool override {
        assert(mdl_ != nullptr);
        return mdl_->isTrue(solver_literal(lit));
    }

    [[nodiscard]] auto do_is_consequence(Output::lit_t lit) const -> ConsequenceType override {
        assert(mdl_ != nullptr);
        auto slit = solver_literal(lit);
        auto res = ConsequenceType::false_;
        if (mdl_->isDef(slit)) {
            res = ConsequenceType::true_;
        } else if (mdl_->isEst(slit)) {
            res = ConsequenceType::unknown;
        }
        if (res != ConsequenceType::false_ && !is_projected_(lit)) {
            res = ConsequenceType::false_;
        }
        return res;
    }

    [[nodiscard]] auto do_costs() const -> std::span<Output::sum_t const> override {
        assert(mdl_ != nullptr);
        return mdl_->costs;
    }

    [[nodiscard]] auto do_priorities() const -> std::span<Output::weight_t const> override {
        assert(mdl_ != nullptr && mdl_->ctx != nullptr);
        auto const *m = mdl_->ctx->minimizer();
        return m != nullptr ? m->prios : std::span<Output::weight_t const>{};
    }

    [[nodiscard]] auto do_optimality_proven() const -> bool override {
        assert(mdl_ != nullptr);
        return mdl_->opt;
    }

    [[nodiscard]] auto do_thread_id() const -> Output::id_t override {
        assert(mdl_ != nullptr);
        return mdl_->sId;
    }

    [[nodiscard]] auto do_control() -> SolveControl & override { return *this; }

    [[nodiscard]] auto do_add_clause(Output::LitSpan lits) -> bool override {
        assert(mdl_ != nullptr);
        lits_.clear();
        lits_.reserve(lits.size());
        for (auto const &lit : lits) {
            lits_.emplace_back(solver_literal(lit));
        }
        return mdl_->ctx->commitClause(lits_);
    }

    [[nodiscard]] auto is_projected_(Output::lit_t literal) const -> bool {
        if (slv().sharedContext()->output.projectMode() == Clasp::ProjectMode::project) {
            return prg().isProjected(literal);
        }
        return prg().isShown(literal);
    }

    //! Get the underlying logic program.
    [[nodiscard]] auto prg() const -> Clasp::Asp::LogicProgram const & { return *clasp_->asp(); }
    //! Get the underlying solver (which found the model).
    [[nodiscard]] auto slv() const -> Clasp::Solver const & { return *clasp_->ctx.solver(mdl_->sId); }
    //! Map the given program literal to its solver literal.
    [[nodiscard]] auto solver_literal(Output::lit_t lit) const -> Clasp::Literal {
        return Clasp::Asp::solverLiteral(prg(), lit);
    }
    //! Map the given id referring to a program literal to its solver literal.
    [[nodiscard]] auto solver_literal(uint64_t lit) const -> Clasp::Literal {
        return solver_literal(static_cast<Output::lit_t>(lit));
    }

    BaseMap const *base_;
    Clasp::ClaspFacade *clasp_;
    Clasp::Model const *mdl_ = nullptr;
    std::vector<Clasp::Literal> lits_;
};

//! An event handler that prints models to stdout.
//!
//! @todo This handler only exsists for testing purposes.
class EventHandlerTest : public Clasp::EventHandler {
  public:
    auto onModel(Clasp::Solver const &slv, Clasp::Model const &mdl) -> bool override {
        buf_ << "Model:";
        for (auto const &pred : slv.outputTable().fact_range()) {
            buf_ << " " << pred.c_str();
        }
        for (auto const &pred : slv.outputTable().pred_range()) {
            if (mdl.isTrue(pred.cond)) {
                buf_ << " " << pred.name.c_str();
            }
        }
        buf_ << "\n";
        buf_.flush();
        return true;
    }

  private:
    Util::OutputBuffer buf_{stdout};
};

//! An event handler that adapts clingo's event handler to clasp's.
class EventHandlerAdapter : public Clasp::EventHandler {
  public:
    EventHandlerAdapter(BaseMap const &base, Clasp::ClaspFacade &clasp, Control::UEventHandler eh)
        : mdl_{base, clasp}, eh_{std::move(eh)} {}

    auto onModel([[maybe_unused]] Clasp::Solver const &slv, Clasp::Model const &mdl) -> bool override {
        mdl_.set_model(&mdl);
        return eh_->on_model(mdl_);
    }

  private:
    ModelImpl mdl_;
    Control::UEventHandler eh_;
};

//! A solve handle that does nothing.
//!
//! @note This handle is used if no solver has been setup (for example, in
//! ground-only mode).
class SolveHandleFixed : public SolveHandle {
  private:
    auto do_get() -> SolveResult override { return SolveResult::empty; }
    void do_cancel() override {}
    void do_resume() override {}
    auto do_model() -> Model const * override { return nullptr; }
    auto do_last() -> Model const * override { return nullptr; }
    auto do_core() -> Output::LitSpan override { return {}; }
    auto do_wait([[maybe_unused]] double timeout) -> bool override { return true; }
};

//! The solve handle implementation.
class SolveHandleImpl : public SolveHandle {
  public:
    SolveHandleImpl(std::unique_ptr<Clasp::EventHandler> eh, BaseMap const &base, Clasp::ClaspFacade &clasp,
                    Clasp::ClaspFacade::SolveHandle const &hnd)
        : mdl_{base, clasp}, hnd_{hnd}, eh_{std::move(eh)} {}

  private:
    auto do_get() -> SolveResult override {
        static constexpr uint8_t i1 = 9;
        static constexpr uint8_t i2 = 65;
        auto cres = hnd_.get();
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
    void do_cancel() override { hnd_.cancel(); }
    void do_resume() override { hnd_.resume(); }
    auto do_model() -> Model const * override { return mdl_.set_model(hnd_.model()) ? &mdl_ : nullptr; }
    auto do_last() -> Model const * override { return mdl_.set_last() ? &mdl_ : nullptr; }
    auto do_core() -> Output::LitSpan override {
        auto const &clasp = mdl_.clasp();
        core_.clear();
        if (auto core = clasp.summary().unsatCore(); !core.empty()) {
            clasp.asp()->translateCore(core, core_);
        }
        return core_;
    }
    auto do_wait(double timeout) -> bool override {
        if (timeout == 0) {
            return hnd_.ready();
        }
        if (timeout < 0) {
            return hnd_.wait(), true;
        }
        return hnd_.waitFor(timeout);
    }

    ModelImpl mdl_;
    Clasp::ClaspFacade::SolveHandle hnd_;
    std::unique_ptr<Clasp::EventHandler> eh_;
    Potassco::LitVec mutable core_;
};

} // namespace

void Scripts::register_script(std::string_view name, UScript script) { scripts_.emplace_back(name, std::move(script)); }

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

Solver::Solver(Logger &log, SymbolStore &store, Scripts &scripts, Input::RewriteOptions opts, AppMode mode, FILE *out)
    : buf_{out}, out_{make_output_(store, mode)}, grd_{log, store, opts, *out_}, scripts_{&scripts}, mode_{mode} {}

auto Solver::make_output_(SymbolStore &store, AppMode mode) -> UOutputStm {
    switch (mode) {
        case AppMode::solve: {
            // FIXME: find a better place to do this
            cfg_.solve.numModels = 0;
            backend_ = std::make_unique<BackendImpl>(clasp_.startAsp(cfg_, true));
            return Output::make_backend_output(store, *backend_);
        }
        default: {
            return Output::make_text_output(buf_);
        }
    }
    Util::unreachable();
}

void Solver::main(std::span<std::string_view const> const &files,
                  std::optional<std::vector<Clingo::Input::ProgramParamVec>> const &params) {
    parse(files);
    main(params);
}

void Solver::main(std::optional<std::vector<Clingo::Input::ProgramParamVec>> const &params) {
    if (scripts_->callable("main", 0)) {
        scripts_->main(*this);
    } else {
        if (mode_ == AppMode::parse) {
            output_unprocessed_program(std::cout);
            return;
        }
        if (mode_ == AppMode::rewrite) {
            output_program(std::cout);
            return;
        }
        if (params) {
            for (auto const &param : *params) {
                ground(param, nullptr);
                if (mode_ == AppMode::solve) {
                    solve(nullptr);
                }
            }
        } else {
            ground(Clingo::Input::ProgramParamVec{{grd_.store().string("base"), {}}}, nullptr);
            if (mode_ == AppMode::solve) {
                solve(nullptr);
            }
        }
    }
}

auto Solver::solve(UEventHandler handler, Output::LitSpan assumptions, SolveMode mode) -> USolveHandle {
    if (mode_ == AppMode::solve) {
        if (state_ == State::solved || state_ == State::updated) {
            // we inject an emtpy ground
            ground(Input::ProgramParamVec{}, nullptr);
        }
        state_ = State::solved;
        clasp_.prepare();

        // convert solve mode
        auto cm = [](SolveMode mode) {
            auto res = Clasp::SolveMode::def;
            if (test(mode, SolveMode::yield)) {
                res |= Clasp::SolveMode::yield;
            }
            if (test(mode, SolveMode::async)) {
                res |= Clasp::SolveMode::async;
            }
            return res;
        };
        // convert assumptions
        auto ca = [this](auto const &lits) {
            auto res = Clasp::LitVec{};
            res.reserve(lits.size());
            std::ranges::transform(lits, std::back_inserter(res),
                                   [this](auto const &lit) { return Clasp::Asp::solverLiteral(*clasp_.asp(), lit); });
            return res;
        };
        // adapt the handler for clasp
        auto eh = handler == nullptr ? std::unique_ptr<Clasp::EventHandler>{std::make_unique<EventHandlerTest>()}
                                     : std::make_unique<EventHandlerAdapter>(grd_.base(), clasp_, std::move(handler));
        auto hnd = clasp_.solve(cm(mode), ca(assumptions), eh.get());
        // adapt the handle which also manages the lifetime of the handler
        return std::make_unique<SolveHandleImpl>(std::move(eh), grd_.base(), clasp_, hnd);
    }
    return std::make_unique<SolveHandleFixed>();
}

void Solver::join(Input::UnprocessedProgram const &prg) { grd_.join(prg); }

void Solver::parse(std::string_view str) { grd_.parse(str, scripts_); }

void Solver::parse(std::span<std::string_view const> const &files) { grd_.parse(files, scripts_); }

void Solver::add_const(String name, Symbol value) { grd_.add_const(name, value); }

void Solver::ground(Input::ProgramParamVec const &params, Ground::ScriptCallback *ctx) {
    if (mode_ == AppMode::solve && state_ != State::updated) {
        clasp_.update(true);
    }
    state_ = State::grounded;
    std::ignore = grd_.ground(params, ctx != nullptr ? ctx : scripts_);
}

void Solver::output_unprocessed_program(std::ostream &out) { grd_.output_unprocessed_program(out); }

void Solver::output_program(std::ostream &out) { grd_.output_program(out); }

} // namespace Clingo::Control
