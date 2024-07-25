#pragma once

#include <gringo/input/statement.hh>

#include <gringo/core/logger.hh>

#include <istream>

namespace Gringo::Input {

namespace Parse {

class ParserState;

}

class Parser {
  public:
    Parser(Logger &log, SymbolStore &store, std::istream &in, String file);
    Parser(Logger &log, SymbolStore &store, std::string_view in, String file);
    Parser(Parser const &other) = delete;
    Parser(Parser &&other) noexcept;
    auto operator=(Parser const &other) -> Parser & = delete;
    auto operator=(Parser &&other) noexcept -> Parser &;
    ~Parser() noexcept;

    auto parse_term() -> std::optional<Term>;
    auto parse_literal() -> std::optional<Lit>;

  private:
    std::unique_ptr<Parse::ParserState> impl_;
};

} // namespace Gringo::Input
