#include <gringo/input/literal.hh>

#include <gringo/input/rewrite/analyze.hh>

#include "parser_state.hh"

namespace Gringo::Input::Parse {

namespace {}

auto parse_statement(ParserState &state) -> std::optional<Stm> {
    static_cast<void>(state);
    throw std::runtime_error("implement me!!!");
    return std::nullopt;
}

} // namespace Gringo::Input::Parse
