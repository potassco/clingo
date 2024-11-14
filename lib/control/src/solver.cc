#include <clingo/control/solver.hh>

#include <clingo/output/text.hh>

#include <clasp/solver.h>

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
    void do_rule(std::span<uint32_t const> head, std::span<int32_t const> body, bool choice) override {
        bld_.clear();
        bld_.start(choice ? Potassco::Head_t::choice : Potassco::Head_t::disjunctive);
        for (auto const &atom : head) {
            bld_.addHead(atom);
        }
        bld_.startBody();
        for (auto const &lit : body) {
            bld_.addGoal(lit);
        }
        prg_->addRule(bld_);
    }
    void do_show(Symbol sym, std::span<int32_t const> body) override {
        buf_.reset();
        buf_ << sym;
        prg_->addOutput(buf_.c_str(), prg_->newCondition(body));
    }

    Util::OutputBuffer buf_;
    Potassco::RuleBuilder bld_;
    Clasp::Asp::LogicProgram *prg_;
};

} // namespace

Solver::Solver(Logger &log, SymbolStore &store, Scripts &scripts, Input::RewriteOptions opts, OutputMode mode,
               FILE *out)
    : buf_{out}, out_{make_output_(mode)}, grd_{log, store, opts, *out_}, scripts_{&scripts},
      has_clasp_{mode == OutputMode::clasp} {}

auto Solver::make_output_(OutputMode mode) -> UOutputStm {
    switch (mode) {
        case OutputMode::text: {
            return Output::make_text_output(buf_);
        }
        case OutputMode::clasp: {
            // TODO: find a better place to do this
            cfg_.solve.numModels = 0;
            backend_ = std::make_unique<BackendClasp>(clasp_.startAsp(cfg_, true));
            return Output::make_backend_output(*backend_);
        }
    }
    Util::unreachable();
}

void Solver::main(AppMode mode, std::span<std::string_view const> const &files,
                  std::optional<std::vector<Clingo::Input::ProgramParamVec>> const &params) {
    parse(files);
    main(mode, params);
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

void Solver::main(AppMode mode, std::optional<std::vector<Clingo::Input::ProgramParamVec>> const &params) {
    if (scripts_->callable("main", 0)) {
        scripts_->main(*this);
    } else {
        if (mode == AppMode::parse) {
            output_unprocessed_program(std::cout);
            return;
        }
        if (mode == AppMode::rewrite) {
            output_program(std::cout);
            return;
        }
        if (params) {
            for (auto const &param : *params) {
                if (!ground(param, nullptr)) {
                    break;
                }
                if (mode == AppMode::solve) {
                    solve();
                }
            }
        } else {
            std::ignore = ground(Clingo::Input::ProgramParamVec{{grd_.store().string("base"), {}}}, nullptr);
            if (mode == AppMode::solve) {
                solve();
            }
        }
    }
}

void Solver::solve() {
    if (has_clasp_) {
        if (need_update_ == 2) {
            // calling ground with an empty parameter list ensures that the
            // program is updated
            std::ignore = ground(Input::ProgramParamVec{}, nullptr);
        }
        need_update_ = 2;
        clasp_.prepare();
        auto eh = EH{*clasp_.asp(), grd_};
        clasp_.solve(&eh);
    }
}

void Solver::join(Input::UnprocessedProgram const &prg) { grd_.join(prg); }

void Solver::parse(std::string_view str) { grd_.parse(str, scripts_); }

void Solver::parse(std::span<std::string_view const> const &files) { grd_.parse(files, scripts_); }

void Solver::add_const(String name, Symbol value) { grd_.add_const(name, value); }

auto Solver::ground(Input::ProgramParamVec const &params, Ground::ScriptCallback *ctx) -> bool {
    if (has_clasp_ && need_update_ > 0) {
        clasp_.update(true);
    }
    need_update_ = 1;
    // TODO: there is no need for a return value here. Instead, the program
    // should be marked unsatisfiable and further grounding be suppressed.
    // It should even be possible to handle this in the grounder.
    return grd_.ground(params, ctx != nullptr ? ctx : scripts_);
}

void Solver::output_unprocessed_program(std::ostream &out) { grd_.output_unprocessed_program(out); }

void Solver::output_program(std::ostream &out) { grd_.output_program(out); }

} // namespace Clingo::Control
