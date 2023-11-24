#pragma once

#include <logger.hh>

#include <input/statement.hh>

namespace Gringo::Input {

//! @defgroup input_rewrite Rewrite
//! @ingroup input_algo
//!
//! Functions to rewrite statements
//!
//! @{

//! Enumeration to select variables to project.
//!
//! @see Projection
enum class ProjectionMode {
    disabled = 0,  //!< Disable projection.
    anonymous = 1, //!< Only project anonymous variables.
    pure = 2,      //!< Project pure variables.
};

//! Enumeration of available rewrite levels.
enum class RewriteLevel {
    disabled = 0,          //!< Disable rewriting.
    rewrite_anonymous = 1, //!< Give names to anonymous variables.
    unpool = 2,            //!< Remove argument pools.
    project = 3,           //!< Project variables.
};

// TODO: a map might also be an idea here to avoid duplicates for the same variable
//! A vector of term pairs where the second has been substituted by the first in some other term.
using AuxTermVec = std::vector<std::pair<Term, Term>>;

//! Helper to pass arguments to rewrite functions.
struct RewriteContext {
    Logger &log;        //!< Logger to report messages.
    SymbolStore &store; //!< Symbol store to create fresh symbols.
    // TODO: could be made a pointer to optionally set it if required
    NameGen &gen; //!< Generator to create fresh variable names.
    // TODO: could be made a pointer to optionally set it if required
    AuxTermVec &aux; //!< Vector of variable term pairs.
};

//! @}

} // namespace Gringo::Input
