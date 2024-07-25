#include <gringo/input/algo/check_syntax.hh>
#include <gringo/input/algo/parsev2.hh>

#include "parser_state.hh"

namespace Gringo::Input {

namespace Parse {

// Note: declared here to avoid single line include files
auto parse_term(ParserState &state) -> bool;

} // namespace Parse

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
    if (Parse::parse_term(*impl_) && impl_->branch_(Parse::TokenType::end)) {
        assert(impl_->empty() && !impl_->empty_value());
        auto res = impl_->pop_value<Term>();
        assert(impl_->empty_value());
        if (check_term(impl_->log(), res)) {
            return res;
        }
    }
    return std::nullopt;
}

} // namespace Gringo::Input
