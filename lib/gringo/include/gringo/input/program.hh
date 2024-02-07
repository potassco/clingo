#pragma once

#include <stack>

#include <gringo/logger.hh>

#include <gringo/util/ordered_map.hh>
#include <gringo/util/ordered_set.hh>
#include <gringo/util/unordered_map.hh>

#include <gringo/input/statement.hh>

namespace Gringo::Input {

//! @defgroup input_program Programs
//! Data structures and functions to represent and rewrite programs.
//!
//! @ingroup input_language
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

//! Options to configure rewriting.
struct RewriteOptions {
    //! The projection mode.
    ProjectionMode project_mode = ProjectionMode::pure;
    //! Whether to project anonymous variables in negative literals.
    bool project_anonymous = false;
};

// TODO: a map might also be an idea here to avoid duplicates for the same variable
//! A vector of term pairs where the second has been substituted by the first in some other term.
using AuxTermVec = std::vector<std::pair<Term, Term>>;

//! Map from identifiers to constants.
using ConstMap = Util::ordered_map<String, std::pair<StatementConst, Symbol>>;

//! Map from identifiers to constants.
using ParamMap = Util::ordered_set<String>;

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
    RewriteContext(Logger &log, SymbolStore &store, ParamMap &param_map, ConstMap &const_map, StringSet names,
                   char const *prefix)
        : log_{log}, const_map_{const_map}, param_map_{param_map}, gen_{store, names, prefix} {}
    //! Get the logger.
    [[nodiscard]] auto logger() const -> Logger & { return log_; }
    //! Get the symbol store.
    [[nodiscard]] auto store() const -> SymbolStore & { return gen_.store(); }
    //! Get the name generator.
    [[nodiscard]] auto gen() -> NameGen & { return gen_; }
    //! Check if the given identifier is a parameter defined by a program directive.
    //!
    //! If it is a parameter, return its index.
    [[nodiscard]] auto is_param(String name) const -> std::optional<int> {
        if (auto it = param_map_.find(name); it != param_map_.end()) {
            return std::distance(param_map_.begin(), it);
        }
        return std::nullopt;
    }
    //! Check if the given identifier is a parameter defined by a constant.
    //!
    //! If it is a parameter, return its value.
    [[nodiscard]] auto is_const(String name) const -> std::optional<Symbol> {
        if (auto it = const_map_.find(name); it != const_map_.end()) {
            assert(!is_param(name));
            return it->second.second;
        }
        return std::nullopt;
    }
    //! Check if there is at least one parameter (from a program or const statement).
    [[nodiscard]] auto has_params() const -> bool { return !const_map_.empty() || !param_map_.empty(); }
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
    ConstMap &const_map_;        //!< Constant definitions.
    ParamMap &param_map_;        //!< Map of Parameters.
    NameGen gen_;                //!< Generator to create fresh variable names.
    std::stack<AuxTermVec> aux_; //!< Vector of variable term pairs.
};

//! A program part.
struct ProgramPart {
    //! The (first) program part statement that introduced the part.
    StatementProgram part;
    //! The facts in the program part.
    SymbolVec facts;
    //! The statements in the program part.
    StatementVec stms;
};

//! Program grouping unprocessed statements.
struct UnprocessedProgram {
    //! Statements as input grouped by parts.
    using PartVec = std::vector<std::tuple<StatementProgram, StatementVec, SymbolVec>>;

    //! Unprocessed statemtents.
    PartVec parts;
    //! Unprocessed const statements.
    std::vector<StatementConst> const_stms;
    //! Meta statements.
    std::vector<Statement> meta_stms;
};

//! Add a statement.
void add(SymbolStore &store, Statement stm, UnprocessedProgram &prg);

//! A program consisting of parts.
class Program {
  public:
    //! Initialize a program with a rewrite level.
    //!
    //! (The highest rewrite level has to be used for grounding.)
    Program(RewriteOptions opts) : opts_{std::move(opts)} {}
    //! Join with the given unprocessed program.
    //!
    //! If fresh const statements are added, they will be merged with the previous ones.
    //! However, they are only applied once to newly added statements.
    void join(Logger &log, SymbolStore &store, UnprocessedProgram prg);
    //! Visit all the statements in the program.
    //!
    //! See the notes regarding const statements above.
    template <class F> void visit_stms(SymbolStore &store, F fun) const {
        for (auto const &[id, sym] : const_map_) {
            fun(Statement{StatementConst{sym.first.loc_, sym.first.type_, sym.first.name_,
                                         TermSymbol{location(sym.first.value_), sym.second}}});
        }
        for (auto const &stm : meta_stms_) {
            fun(stm);
        }
        for (auto const &[sig, part] : parts_) {
            auto pum = param_map_(store, part);
            auto loc = part.part.loc_;
            StringVec ids;
            ids.reserve(sig.second);
            std::transform(pum.begin(), pum.end(), std::back_inserter(ids), [](auto x) { return x.second; });
            fun(StatementProgram{loc, sig.first, std::move(ids)});
            for (auto const &fact : part.facts) {
                fun(Statement{Rule{loc, SimpleHeadLiteral{LiteralSymbolic{loc, Sign::none, TermSymbol{loc, fact}}},
                                   BodyLiteralVec{}}});
            }
            for (auto const &stm : part.stms) {
                if (auto unmapped = unmap_(store, pum, stm); unmapped) {
                    fun(std::move(unmapped).value());
                } else {
                    fun(stm);
                }
            }
        }
    }

  private:
    //! The signature of a program part.
    //!
    //! (Parameters are numbered from 1 to n.)
    using Signature = std::pair<String, size_t>;
    //! Map from signatures to actual program parts.
    using PartMap = Util::ordered_map<Signature, ProgramPart, Util::value_hasher<Signature>>;
    //! Map from parameters to their replacements.
    using ParamUnmap = Util::ordered_map<String, String>;

    //! Gather all identifiers appearing in a program part.
    [[nodiscard]] static auto param_map_(SymbolStore &store, ProgramPart const &part)
        -> Util::ordered_map<String, String>;
    //! Replace all bound paramets in a statement by parsable ids.
    [[nodiscard]] static auto unmap_(SymbolStore &store, ParamUnmap const &pum, Statement const &stm)
        -> std::optional<Statement>;

    //! The rewrite level of the program.
    RewriteOptions opts_;
    //! The meta statements in the program.
    StatementVec meta_stms_;
    //! The map of program parts.
    PartMap parts_;
    //! The constants and their values.
    ConstMap const_map_;
};

//! @}

} // namespace Gringo::Input
