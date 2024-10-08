#include <clingo/control/solver.hh>

#include <clingo/output/text.hh>

namespace Clingo::Control {

void Scripts::register_script(std::string_view name, UScript script) { scripts_.emplace_back(name, std::move(script)); }

void Scripts::exec(std::string_view name, std::string_view code) {
    for (auto const &script : scripts_) {
        if (script.first == name) {
            script.second->exec(code);
        }
    }
}

void Scripts::do_exec([[maybe_unused]] std::string_view code) { throw std::logic_error("must not be called"); }

void Scripts::do_main(Solver &slv) {
    for (auto const &script : scripts_) {
        if (script.second->callable("main", 0)) {
            script.second->main(slv);
            return;
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

Solver::Solver(Logger &log, SymbolStore &store, Input::RewriteOptions opts, OutputMode mode)
    : buf_{stdout}, out_{Output::make_text_output(buf_)}, grd_{log, store, opts, *out_} {
    static_cast<void>(mode);
}

void Solver::main(std::vector<std::string> const &files) {
    parse(files);
    if (scripts_.callable("main", 0)) {
        scripts_.main(*this);
    } else {
        parse(files);
        std::ignore = ground(Clingo::Input::ProgramParamVec{{grd_.store().string("base"), {}}});
    }
}

void Solver::parse(std::string_view str) {
    // TODO: has to execute code here
    // therefore the scripts class has to implement another interface to execute scripts
    // the prepare method should take an additional argument to execute scripts
    grd_.parse(str);
}

void Solver::parse(std::vector<std::string> const &files) {
    // TODO: has to execute code here
    // therefore the scripts class has to implement another interface to execute scripts
    // the prepare method should take an additional argument to execute scripts
    grd_.parse(files);
}

void Solver::add_const(String name, Symbol value) { grd_.add_const(name, value); }

auto Solver::ground(Input::ProgramParamVec const &params) -> bool { return grd_.ground(params); }

void Solver::register_script(std::string_view name, UScript script) {
    scripts_.register_script(name, std::move(script));
}

void Solver::output_unprocessed_program(std::ostream &out) { grd_.output_unprocessed_program(out); }

void Solver::output_program(std::ostream &out) { grd_.output_program(out); }

} // namespace Clingo::Control
