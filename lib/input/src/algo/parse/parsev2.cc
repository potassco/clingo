#include <gringo/input/algo/check_syntax.hh>
#include <gringo/input/algo/parsev2.hh>

#include "parser_state.hh"

namespace Gringo::Input {

namespace {

template <class P, class C>
auto parse_expr(Parse::ParserState &state, P parse, C check) -> std::invoke_result_t<P, Parse::ParserState &> {
    auto lock = GCLock{state.store()};
    state.consume();
    if (auto lit = parse(state); lit && std::invoke(check, state.log(), *lit)) {
        if (!state.branch(Parse::TokenType::end)) {
            return state.expected<std::nullopt>(Parse::TokenType::end);
        }
        return lit;
    }
    return std::nullopt;
}

} // namespace

Parser::Parser(Logger &log, SymbolStore &store, std::istream &in, String file)
    : impl_{std::make_unique<Parse::ParserState>(log, store, in, file)} {}

Parser::Parser(Logger &log, SymbolStore &store, std::string_view in, String file)
    : impl_{std::make_unique<Parse::ParserState>(log, store, in, file)} {}

Parser::Parser(Parser &&other) noexcept = default;

auto Parser::operator=(Parser &&other) noexcept -> Parser & = default;

Parser::~Parser() noexcept = default;

auto Parser::parse_term() -> std::optional<Term> { return parse_expr(*impl_, Parse::parse_term, check_term); }

auto Parser::parse_literal() -> std::optional<Lit> { return parse_expr(*impl_, Parse::parse_literal, check_literal); }

} // namespace Gringo::Input
