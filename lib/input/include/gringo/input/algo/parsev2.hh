#pragma once

#include <gringo/input/term.hh>

#include <gringo/core/logger.hh>

#include <istream>

namespace Gringo::Input {

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

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Gringo::Input
