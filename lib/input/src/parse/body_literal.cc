#include <gringo/input/literal.hh>

#include <gringo/input/rewrite/analyze.hh>

#include "parser_state.hh"

namespace Gringo::Input::Parse {

namespace {}

auto parse_body_literal(ParserState &state) -> std::optional<BdLit> {
    static_cast<void>(state);
    throw std::runtime_error("implement me!!!");
    return std::nullopt;
}

} // namespace Gringo::Input::Parse
