#pragma once

#include <tsl/ordered_map.h>

#include <input/statement.hh>

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

//! Enumeration of available rewrite levels.
enum class RewriteLevel {
    disabled = 0,          //!< Disable rewriting.
    rewrite_anonymous = 1, //!< Give names to anonymous variables.
    unpool = 2,            //!< Remove argument pools.
    project = 3,           //!< Project variables.
};

//! A program consisting of parts.
class Program {
  public:
    //! Initialize a program with a rewrite level.
    //!
    //! (The highest rewrite level has to be used for grounding.)
    Program(RewriteLevel level) : level_{level} {}
    //! Rewrite and add the given statements to the program.
    void update(StatementVec stms, SymbolVec facts);

  private:
    //! The signature of a program part.
    //!
    //! (Parameters are numbered from 1 to n.
    using Signature = std::pair<String, unsigned>;
    //! Map from signatures to actual program parts.
    using PartMap = tsl::ordered_map<Signature, ProgramPart, Util::value_hasher<Signature>>;
    // rewrite:
    // 1. protect parameters/maybe constants
    //    - it seems like only a variable can be used here
    // 2. evaluate const statements
    // 3. apply const statements
    // *. if a $ has been used,
    //    it would have to be replaced by something to make programs parsable
    //! The rewrite level of the program.
    RewriteLevel level_;
    //! The meta statements in the program.
    StatementVec meta_stms_;
    //! The constants and their values.
    std::unordered_map<String, std::optional<Symbol>> const_defs_;
    //! The map of program parts.
    PartMap parts_;
    //! Statements in the program that have not yet been rewritten.
    StatementVec unprocessed_;
};

//! @}

} // namespace Gringo::Input
