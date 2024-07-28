#include <gringo/input/parser.hh>

#include <gringo/input/algo/check_syntax.hh>

#include "parse/parser_state.hh"

namespace Gringo::Input {

namespace {

template <class C> auto check_true([[maybe_unused]] Logger &log, [[maybe_unused]] C const &expr) -> bool {
    return true;
}

template <class P, class C>
auto parse_expr(Parse::ParserState &state, Parse::Condition cond, P parse,
                C check) -> std::invoke_result_t<P, Parse::ParserState &> {
    auto lock = GCLock{state.store()};
    state.condition(cond);
    state.consume();
    if (auto lit = std::invoke(parse, state); lit && std::invoke(check, state.log(), *lit)) {
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

auto Parser::parse_term() -> std::optional<Term> {
    return parse_expr(*impl_, Parse::Condition::normal, Parse::parse_term, check_term);
}

auto Parser::parse_literal() -> std::optional<Lit> {
    return parse_expr(*impl_, Parse::Condition::normal, Parse::parse_literal, check_literal);
}

auto Parser::parse_theory_term() -> std::optional<TheoryTerm> {
    return parse_expr(*impl_, Parse::Condition::theory, Parse::parse_theory_term, check_true<TheoryTerm>);
}

} // namespace Gringo::Input
