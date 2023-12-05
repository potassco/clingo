#pragma once

#include <cassert>
#include <stack>

#include <logger.hh>

#include <input/program.hh>

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

// TODO: a map might also be an idea here to avoid duplicates for the same variable
//! A vector of term pairs where the second has been substituted by the first in some other term.
using AuxTermVec = std::vector<std::pair<Term, Term>>;

//! Helper to pass arguments to rewrite functions.
class RewriteContext {
  public:
    //! Helper to pop auxiliary variable assignments.
    struct _pop {
        //! Pop the last variable term map pushed.
        void operator()(RewriteContext *ctx) const {
            if (ctx != nullptr) {
                ctx->pop();
                ctx = nullptr;
            }
        }
    };
    //! Helper to pop auxiliary variable assignments.
    using Guard = std::unique_ptr<RewriteContext, _pop>;
    //! Construct a rewrite context.
    RewriteContext(Logger &log, SymbolStore &store, StringSet names, char const *prefix)
        : log_{log}, gen_{store, names, prefix} {}
    //! Get the logger.
    [[nodiscard]] auto logger() const -> Logger & { return log_; }
    //! Get the symbol store.
    [[nodiscard]] auto store() const -> SymbolStore & { return gen_.store(); }
    //! Get the name generator.
    [[nodiscard]] auto gen() -> NameGen & { return gen_; }
    //! Get the variable term map.
    [[nodiscard]] auto aux() -> AuxTermVec & {
        assert(!aux_.empty());
        return aux_.top();
    }
    //! Pop the last variable term map pushed.
    void pop() {
        assert(!aux_.empty());
        aux_.pop();
    }
    //! Push a fresh variable term map.
    [[nodiscard]] auto push() -> Guard {
        aux_.emplace();
        return Guard{this};
    }

  private:
    Logger &log_;                //!< Logger to report messages.
    NameGen gen_;                //!< Generator to create fresh variable names.
    std::stack<AuxTermVec> aux_; //!< Vector of variable term pairs.
};

//! @}

} // namespace Gringo::Input
