#pragma once

#include <gringo/input/statement.hh>

#include <gringo/core/logger.hh>

#include <istream>

namespace Gringo::Input {

namespace Parse {

class ParserState;

}

//! @addtogroup input_parse
//! @{

//! A parser for the clingo language.
class Parser {
  public:
    //! Consruct parser.
    Parser(Logger &log, SymbolStore &store);
    //! Initialize parser.
    Parser(Parser const &other) = delete;
    Parser(Parser &&other) noexcept;
    auto operator=(Parser const &other) -> Parser & = delete;
    auto operator=(Parser &&other) noexcept -> Parser &;
    ~Parser() noexcept;

    //! Initialize parser reading from the given input stream.
    void init(std::istream &in, String file);
    //! Initialize parser with the given string.
    //!
    //! Note that the string is copied.
    void init(std::string_view in, String file);

    //! Parse a term.
    auto parse_term() -> std::optional<Term>;
    //! Parse a theory term.
    auto parse_theory_term() -> std::optional<TheoryTerm>;
    //! Parse a literal.
    auto parse_literal() -> std::optional<Lit>;

  private:
    std::unique_ptr<Parse::ParserState> impl_;
};

//! @}

} // namespace Gringo::Input
