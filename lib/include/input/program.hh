#pragma once

#include <stack>

#include <logger.hh>

#include <util/ordered_map.hh>
#include <util/unordered_map.hh>

#include <input/statement.hh>

namespace Gringo::Input {

//! @defgroup input_program Programs
//! @ingroup input_language
//!
//! Data structures and functions to represent and rewrite programs.
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
    simplify = 4,          //!< Enable simplifications.
};

//! Options to configure rewriting.
struct RewriteOptions {
    //! The rewrite level.
    RewriteLevel level = RewriteLevel::simplify;
    //! The projection mode.
    ProjectionMode project_mode = ProjectionMode::pure;
    //! Whether to project anonymous variables in negative literals.
    bool project_anonymous = false;
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

//! A program part.
struct ProgramPart {
    //! The facts in the program part.
    SymbolVec facts;
    //! The statements in the program part.
    StatementVec stms;
};

class Program;

//! Program gathering statements.
class UnprocessedProgram {
    // TODO: ugly!
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

//! Map from identifiers to constants.
using ConstMap = Util::unordered_map<String, std::pair<StatementConst, Symbol>>;

//! A program consisting of parts.
class Program {
  public:
    //! Initialize a program with a rewrite level.
    //!
    //! (The highest rewrite level has to be used for grounding.)
    Program(RewriteOptions opts) : opts_{std::move(opts)} {}
    //! Join with the given unprocessed program.
    void join(Logger &log, SymbolStore &store, UnprocessedProgram prg);

  private:
    //! The signature of a program part.
    //!
    //! (Parameters are numbered from 1 to n.)
    using Signature = std::pair<String, size_t>;
    //! Map from signatures to actual program parts.
    using PartMap = Util::ordered_map<Signature, ProgramPart, Util::value_hasher<Signature>>;

    //! The rewrite level of the program.
    RewriteOptions opts_;
    //! The meta statements in the program.
    StatementVec meta_stms_;
    //! The constants and their values.
    ConstMap const_defs_;
    //! The map of program parts.
    PartMap parts_;
};

//! @}

} // namespace Gringo::Input
