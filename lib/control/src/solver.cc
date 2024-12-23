#include <clingo/control/solver.hh>

#include <clingo/output/text.hh>

#include <clingo/util/checked_math.hh>

#include <clasp/solver.h>

// #define DEBUG_BACKEND
#ifdef DEBUG_BACKEND
#include <clingo/util/print.hh>
#include <iostream>
#endif

namespace Clingo::Control {

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

namespace {

class BackendClasp : public Output::Backend {
  public:
    BackendClasp(Clasp::Asp::LogicProgram &prg) : prg_{&prg} {}

  private:
    auto do_next_lit() -> Output::lit_t override {
        if (lit_ < Output::lit_max) {
            return ++lit_;
        }
        throw std::range_error("literals number of literals exhausted");
    }

    void do_rule(Output::LitSpan head, Output::LitSpan body, bool choice) override {
        bld_.clear();
        bld_.start(choice ? Potassco::Head_t::choice : Potassco::Head_t::disjunctive);
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
                    return Potassco::Value_t::true_;
                }
                case ExternalType::false_: {
                    return Potassco::Value_t::false_;
                }
                case ExternalType::free: {
                    return Potassco::Value_t::free;
                }
            }
            Util::unreachable();
        }();
        prg_->addExternal(atom, value);
    }

    void do_project(Output::lit_t atom) override { prg_->addProject(std::array{static_cast<Potassco::Atom_t>(atom)}); }

    void do_minimize(Output::lit_t lit, Output::weight_t weight, Output::weight_t priority) override {
        prg_->addMinimize(priority, std::array{Potassco::WeightLit_t{lit, weight}});
    }

    void do_show(Symbol sym, Output::LitSpan body) override {
        buf_.reset();
        buf_ << sym;
        prg_->addOutput(buf_.c_str(), prg_->newCondition(body));
#ifdef DEBUG_BACKEND
        std::cerr << "#show " << sym << " : " << Util::p_range(body, ", ") << ".\n";
#endif
    }

    Output::lit_t lit_ = 0;
    Util::OutputBuffer buf_;
    Potassco::RuleBuilder bld_;
    Clasp::Asp::LogicProgram *prg_;
};

} // namespace

Solver::Solver(Logger &log, SymbolStore &store, Scripts &scripts, Input::RewriteOptions opts, AppMode mode, FILE *out)
    : buf_{out}, out_{make_output_(mode)}, grd_{log, store, opts, *out_}, scripts_{&scripts}, mode_{mode} {}

auto Solver::make_output_(AppMode mode) -> UOutputStm {
    switch (mode) {
        case AppMode::solve: {
            // TODO: find a better place to do this
            cfg_.solve.numModels = 0;
            backend_ = std::make_unique<BackendClasp>(clasp_.startAsp(cfg_, true));
            return Output::make_backend_output(*backend_);
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

//! Test model printer.
class EH : public Clasp::EventHandler {
  public:
    EH(Clasp::Asp::LogicProgram &prg, Grounder &grd) : prg_{&prg}, grd_{&grd} {}
    auto onModel([[maybe_unused]] Clasp::Solver const &slv, Clasp::Model const &mdl) -> bool override {
        buf_ << "Model:";
        for (auto const &[sig, base] : grd_->base()) {
            for (auto i = size_t{0}, e = base->size(); i != e; ++i) {
                auto const &atom = base->nth(i);
                if (auto lit = Clasp::Asp::solverLiteral(*prg_, static_cast<int32_t>(atom->second.id));
                    mdl.isTrue(lit)) {
                    buf_ << " " << atom->first;
                }
            }
        }
        buf_ << "\n";
        buf_.flush();
        return true;
    }

  private:
    Util::OutputBuffer buf_{stdout};
    Clasp::Asp::LogicProgram *prg_;
    Grounder *grd_;
};

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
                    solve();
                }
            }
        } else {
            ground(Clingo::Input::ProgramParamVec{{grd_.store().string("base"), {}}}, nullptr);
            if (mode_ == AppMode::solve) {
                solve();
            }
        }
    }
}

void Solver::solve() {
    if (mode_ == AppMode::solve) {
        if (state_ == State::solved || state_ == State::updated) {
            // we inject an emtpy ground
            ground(Input::ProgramParamVec{}, nullptr);
        }
        state_ = State::solved;
        clasp_.prepare();
        auto eh = EH{*clasp_.asp(), grd_};
        clasp_.solve(&eh);
    }
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
