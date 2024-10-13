#include <clingo/control/solver.hh>

#include <clingo/output/text.hh>

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

void Scripts::do_main(Solver &slv) {
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

void Scripts::do_call(std::string_view name, SymbolSpan args, SymbolVec &out) {
    out.clear();
    for (auto const &script : scripts_) {
        if (script.second->callable(name, args.size())) {
            script.second->call(name, args, out);
        }
    }
}

Solver::Solver(Logger &log, SymbolStore &store, Scripts &scripts, Input::RewriteOptions opts, OutputMode mode)
    : buf_{stdout}, out_{Output::make_text_output(buf_)}, grd_{log, store, opts, *out_}, scripts_{&scripts} {
    static_cast<void>(mode);
}

void Solver::main(std::span<std::string_view const> const &files) {
    parse(files);
    if (scripts_->callable("main", 0)) {
        scripts_->main(*this);
    } else {
        parse(files);
        std::ignore = ground(Clingo::Input::ProgramParamVec{{grd_.store().string("base"), {}}});
    }
}

void Solver::parse(std::string_view str) { grd_.parse(str, scripts_); }

void Solver::parse(std::span<std::string_view const> const &files) { grd_.parse(files, scripts_); }

void Solver::add_const(String name, Symbol value) { grd_.add_const(name, value); }

auto Solver::ground(Input::ProgramParamVec const &params) -> bool { return grd_.ground(params, scripts_); }

void Solver::output_unprocessed_program(std::ostream &out) { grd_.output_unprocessed_program(out); }

void Solver::output_program(std::ostream &out) { grd_.output_program(out); }

} // namespace Clingo::Control
