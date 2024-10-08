#include <clingo/control/solver.hh>

#include <clingo/output/text.hh>

namespace Clingo::Control {

Solver::Solver(Logger &log, SymbolStore &store, Input::RewriteOptions opts, OutputMode mode)
    : buf_{stdout}, out_{Output::make_text_output(buf_)}, grd_{log, store, opts, *out_} {
    static_cast<void>(mode);
}

void Solver::parse(std::string_view str) { grd_.parse(str); }

void Solver::parse(std::vector<std::string> const &files) { grd_.parse(files); }

void Solver::add_const(String name, Symbol value) { grd_.add_const(name, value); }

void Solver::prepare() { grd_.prepare(); }

auto Solver::ground(Input::ProgramParamVec const &params) -> bool { return grd_.ground(params); }

void Solver::output_unprocessed_program(std::ostream &out) { grd_.output_unprocessed_program(out); }

void Solver::output_program(std::ostream &out) { grd_.output_program(out); }

} // namespace Clingo::Control
