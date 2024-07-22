#pragma once

#include <memory>

namespace Gringo::Input {

class Parser {
  public:
    Parser(std::istream &in);
    Parser(Parser const &other) = delete;
    Parser(Parser &&other) noexcept;
    auto operator=(Parser const &other) -> Parser & = delete;
    auto operator=(Parser &&other) noexcept -> Parser &;
    ~Parser() noexcept;

    auto parse_term() -> bool;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Gringo::Input
