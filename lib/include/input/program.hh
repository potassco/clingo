#pragma once

#include <util/ordered_map.hh>
#include <util/unordered_map.hh>

#include <input/statement.hh>

#include <input/algo/rewrite_base.hh>

namespace Gringo::Input {

//! @defgroup input_program Program
//! @ingroup input_language
//!
//! Data structures and functions to represent programs.
//!
//! @{

//! A program part.
class ProgramPart {
  private:
    //! The facts in the program part.
    SymbolVec facts_;
    //! The statements in the program part.
    StatementVec stms_;
};

class Program;

//! Program gathering statements.
class UnprocessedProgram {
    friend class Program;

  public:
    //! Add a statement.
    void add(SymbolStore &store, Statement stm);

  private:
    //! Statements as input grouped by parts.
    using PartVec = std::vector<std::tuple<StatementProgram, StatementVec, SymbolVec>>;

    //! Unprocessed statemtents.
    PartVec parts_;
    //! Unprocessed const statements.
    std::vector<StatementConst> const_stms_;
    //! Meta statements.
    std::vector<Statement> meta_stms_;
};

//! A program consisting of parts.
class Program {
  public:
    //! Initialize a program with a rewrite level.
    //!
    //! (The highest rewrite level has to be used for grounding.)
    Program(RewriteLevel level) : level_{level} {}
    //! Add the given unprocessed progam.
    //!
    //! @todo:
    //! 1. organize programs into parts protecting parameters
    //! 2. apply const directives
    //! 3. rewrite statements
    auto update(Logger &log, SymbolStore &store, UnprocessedProgram prg) -> bool;

  private:
    //! The signature of a program part.
    //!
    //! (Parameters are numbered from 1 to n.)
    using Signature = std::pair<String, unsigned>;
    //! Map from signatures to actual program parts.
    using PartMap = Util::ordered_map<Signature, ProgramPart, Util::value_hasher<Signature>>;
    //! Map from const parameters to their values.
    using ConstMap = Util::unordered_map<String, std::optional<Symbol>>;

    //! The rewrite level of the program.
    RewriteLevel level_;
    //! The meta statements in the program.
    StatementVec meta_stms_;
    //! The constants and their values.
    ConstMap const_defs_;
    //! The map of program parts.
    PartMap parts_;
};

//! @}

} // namespace Gringo::Input
