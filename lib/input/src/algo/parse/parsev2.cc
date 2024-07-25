#include <gringo/input/algo/check_syntax.hh>
#include <gringo/input/algo/parsev2.hh>

#include "parser_state.hh"

namespace Gringo::Input {

Parser::Parser(Logger &log, SymbolStore &store, std::istream &in, String file)
    : impl_{std::make_unique<Parse::ParserState>(log, store, in, file)} {}

Parser::Parser(Logger &log, SymbolStore &store, std::string_view in, String file)
    : impl_{std::make_unique<Parse::ParserState>(log, store, in, file)} {}

Parser::Parser(Parser &&other) noexcept = default;

auto Parser::operator=(Parser &&other) noexcept -> Parser & = default;

Parser::~Parser() noexcept = default;

auto Parser::parse_term() -> std::optional<Term> {
    auto lock = GCLock{impl_->store()};
    impl_->consume();
    if (auto term = Parse::parse_term(*impl_); term && check_term(impl_->log(), *term)) {
        if (!impl_->branch(Parse::TokenType::end)) {
            return impl_->expected<std::nullopt>(Parse::TokenType::end);
        }
        return term;
    }
    return std::nullopt;
}

auto Parser::parse_literal() -> std::optional<Lit> {
    auto lock = GCLock{impl_->store()};
    impl_->consume();
    if (auto lit = Parse::parse_literal(*impl_); lit && check_literal(impl_->log(), *lit)) {
        if (!impl_->branch(Parse::TokenType::end)) {
            return impl_->expected<std::nullopt>(Parse::TokenType::end);
        }
        return lit;
    }
    return std::nullopt;
}

} // namespace Gringo::Input
