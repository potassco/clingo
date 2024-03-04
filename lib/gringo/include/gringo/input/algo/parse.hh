#pragma once

#include <gringo/input/statement.hh>

#include <gringo/core/logger.hh>

namespace Gringo::Input {

//! @addtogroup input_parse
//! @{

//! @internal The scanner implementation.
class ScannerImpl;

//! A scanner to parse statements.
class Scanner {
  public:
    Scanner(Scanner &&other) noexcept;
    Scanner(Scanner const &other) = delete;

    auto operator=(Scanner &&other) noexcept -> Scanner &;
    auto operator=(Scanner const &other) -> Scanner & = delete;

    friend auto scan_stream(Logger &log, SymbolStore &store, std::istream &in) -> Scanner;
    friend auto scan_file(Logger &log, SymbolStore &store, char const *path) -> Scanner;
    friend auto scan_string(Logger &log, SymbolStore &store, std::string_view content) -> Scanner;

    //! Destroy the scanner.
    ~Scanner() noexcept;

    //! Scan the next statement.
    auto scan() -> std::optional<Stm>;

  private:
    Scanner(std::unique_ptr<ScannerImpl> impl);

    std::unique_ptr<ScannerImpl> impl_;
};

//! Parse a term.
auto parse_term(Logger &log, SymbolStore &store, std::string_view str) -> std::optional<Term>;
//! Parse a theory term.
auto parse_theory_term(Logger &log, SymbolStore &store, std::string_view str) -> std::optional<TheoryTerm>;
//! Parse a literal.
auto parse_literal(Logger &log, SymbolStore &store, std::string_view str) -> std::optional<Lit>;
//! Parse a head literal.
auto parse_head_literal(Logger &log, SymbolStore &store, std::string_view str) -> std::optional<HdLit>;
//! Parse a body literal.
auto parse_body_literal(Logger &log, SymbolStore &store, std::string_view str) -> std::optional<BdLit>;
//! Parse a statement.
auto parse_statement(Logger &log, SymbolStore &store, std::string_view str) -> std::optional<Stm>;

//! Return a scanner to parse statements one by one.
auto scan_stream(Logger &log, SymbolStore &store, std::istream &in) -> Scanner;
//! Return a scanner to parse statements one by one.
auto scan_file(Logger &log, SymbolStore &store, char const *path) -> Scanner;
//! Return a scanner to parse statements one by one.
auto scan_string(Logger &log, SymbolStore &store, std::string_view content) -> Scanner;

//! @}

} // namespace Gringo::Input
