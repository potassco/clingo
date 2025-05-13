#pragma once

#include <clingo/input/statement.hh>

#include <clingo/core/backend.hh>
#include <clingo/core/logger.hh>

#include <istream>

namespace CppClingo::Input {

namespace Parse {

class ParserState;

}

//! @addtogroup input_parse
//! @{

//! A parser for the clingo language.
class Parser {
  public:
    //! Construct the parser.
    //!
    //! The parser is in an invalid state and must be initialized first.
    Parser(Logger &log, SymbolStore &store, ProgramBackend *prg_backend = nullptr,
           TheoryBackend *thy_backend = nullptr);
    //! Copy construct the parser.
    Parser(Parser const &other) = delete;
    //! Move construct the parser.
    Parser(Parser &&other) noexcept;
    //! Copy assign the parser.
    auto operator=(Parser const &other) -> Parser & = delete;
    //! Move assign the parser.
    auto operator=(Parser &&other) noexcept -> Parser &;
    //! Destroy the parser.
    ~Parser() noexcept;

    //! Initialize parser reading from the given input stream.
    void init(std::istream &in, String file);
    //! Initialize parser with the given string.
    //!
    //! Note that the string is copied.
    void init(std::string_view in, String file);

    //! Parse a symbol.
    auto parse_symbol() -> std::optional<SharedSymbol>;
    //! Parse program params to ground.
    auto parse_program_parts() -> std::optional<ProgramParamVec>;
    //! Parse a const definition of form name=symbol.
    auto parse_const_def() -> std::optional<std::pair<SharedString, SharedSymbol>>;
    //! Parse a term.
    auto parse_term() -> std::optional<Term>;
    //! Parse a theory term.
    auto parse_theory_term() -> std::optional<TheoryTerm>;
    //! Parse a literal.
    auto parse_literal() -> std::optional<Lit>;
    //! Parse a body literal.
    auto parse_body_literal() -> std::optional<BdLit>;
    //! Parse a head literal.
    auto parse_head_literal() -> std::optional<HdLit>;
    //! Parse a statement.
    auto parse_statement() -> std::optional<Stm>;

    //! Scan statements.
    //!
    //! If no statement is returned, the end of input has been reached. The
    //! Boolean indicates a parse error. Note that the functions tries to
    //! recover from errors and might still be able to continue parsing.
    auto scan() -> std::pair<std::optional<Stm>, bool>;

  private:
    std::unique_ptr<Parse::ParserState> impl_;
};

//! @}

} // namespace CppClingo::Input
