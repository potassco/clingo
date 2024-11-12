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

Solver::Solver(Logger &log, SymbolStore &store, Scripts &scripts, Input::RewriteOptions opts, OutputMode mode,
               FILE *out)
    : buf_{out}, out_{Output::make_text_output(buf_)}, grd_{log, store, opts, *out_}, scripts_{&scripts} {
    static_cast<void>(mode);
}

void Solver::main(AppMode mode, std::span<std::string_view const> const &files,
                  std::optional<std::vector<Clingo::Input::ProgramParamVec>> const &params) {
    parse(files);
    main(mode, params);
}

//! Dummy model printer.
//!
//! Note that currently all atoms are reported because there is no ClaspOutput yet and ids are zero.
class EH : public Clasp::EventHandler {
  public:
    EH(Grounder &grd) : grd_{&grd} {}
    auto onModel([[maybe_unused]] Clasp::Solver const &slv, Clasp::Model const &mdl) -> bool override {
        std::cerr << "Model:";
        for (auto const &[sig, base] : grd_->base()) {
            for (auto i = size_t{0}, e = base->size(); i != e; ++i) {
                auto const &atom = base->nth(i);
                if (slv.assignment().valid(atom->second.id) &&
                    mdl.isTrue(Clasp::toLit(static_cast<int32_t>(atom->second.id)))) {
                    std::cerr << " " << atom->first;
                }
            }
        }
        std::cerr << "\n";
        std::cerr.flush();
        return true;
    }

  private:
    Grounder *grd_;
};

void Solver::main(AppMode mode, std::optional<std::vector<Clingo::Input::ProgramParamVec>> const &params) {
    clasp_.start(cfg_, Clasp::Problem_t::asp);
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
            }
        } else {
            std::ignore = ground(Clingo::Input::ProgramParamVec{{grd_.store().string("base"), {}}}, nullptr);
        }
        if (mode == AppMode::ground) {
            return;
        }
        auto eh = EH{grd_};
        clasp_.solve(&eh);
    }
}

void Solver::join(Input::UnprocessedProgram const &prg) { grd_.join(prg); }

void Solver::parse(std::string_view str) { grd_.parse(str, scripts_); }

void Solver::parse(std::span<std::string_view const> const &files) { grd_.parse(files, scripts_); }

void Solver::add_const(String name, Symbol value) { grd_.add_const(name, value); }

auto Solver::ground(Input::ProgramParamVec const &params, Ground::ScriptCallback *ctx) -> bool {
    return grd_.ground(params, ctx != nullptr ? ctx : scripts_);
}

void Solver::output_unprocessed_program(std::ostream &out) { grd_.output_unprocessed_program(out); }

void Solver::output_program(std::ostream &out) { grd_.output_program(out); }

} // namespace Clingo::Control
