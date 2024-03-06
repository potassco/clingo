#pragma once

#include <gringo/input/statement.hh>

#include <gringo/core/logger.hh>

#include <gringo/util/ordered_map.hh>

namespace Gringo::Input {

//! @addtogroup input_program
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

//! Map from identifiers to constants.
using ConstMap = Util::ordered_map<String, std::pair<StmConst, Symbol>>;

//! A program part.
struct ProgramPart {
    //! The (first) program part statement that introduced the part.
    StmProgram part;
    //! The facts in the program part.
    SymbolVec facts;
    //! The statements in the program part.
    StmVec stms;
};

//! Program grouping unprocessed statements.
struct UnprocessedProgram {
    //! Statements as input grouped by parts.
    using PartVec = std::vector<std::tuple<StmProgram, StmVec, SymbolVec>>;

    //! Unprocessed statemtents.
    PartVec parts;
    //! Unprocessed const statements.
    std::vector<StmConst> const_stms;
    //! Theory statements.
    std::vector<StmTheory> thy_stms;
    //! Meta statements.
    std::vector<Stm> meta_stms;
};

//! Add a statement.
void add(SymbolStore &store, Stm stm, UnprocessedProgram &prg);

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
            fun(Stm{StmConst{sym.first.loc(), sym.first.type(), sym.first.name(),
                             TermSymbol{location(sym.first.value()), sym.second}}});
        }
        for (auto const &stm : thy_stms_) {
            fun(stm);
        }
        for (auto const &stm : meta_stms_) {
            fun(stm);
        }
        for (auto const &[sig, part] : parts_) {
            auto pum = param_map_(store, part);
            auto loc = part.part.loc();
            StringVec ids;
            ids.reserve(sig.second);
            std::transform(pum.begin(), pum.end(), std::back_inserter(ids), [](auto x) { return x.second; });
            fun(StmProgram{loc, sig.first, std::move(ids)});
            for (auto const &fact : part.facts) {
                fun(Stm{StmRule{loc, HdLitSimple{LitSymbolic{loc, Sign::none, TermSymbol{loc, fact}}}, BdLitArray{}}});
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
    using PartMap = Util::ordered_map<Signature, ProgramPart>;
    //! Map from parameters to their replacements.
    using ParamUnmap = Util::ordered_map<String, String>;

    //! Gather all identifiers appearing in a program part.
    [[nodiscard]] static auto param_map_(SymbolStore &store, ProgramPart const &part)
        -> Util::ordered_map<String, String>;
    //! Replace all bound paramets in a statement by parsable ids.
    [[nodiscard]] static auto unmap_(SymbolStore &store, ParamUnmap const &pum, Stm const &stm) -> std::optional<Stm>;

    //! The rewrite level of the program.
    RewriteOptions opts_;
    //! The meta statements in the program.
    StmVec meta_stms_;
    //! Theory statements.
    std::vector<StmTheory> thy_stms_;
    //! The map of program parts.
    PartMap parts_;
    //! The constants and their values.
    ConstMap const_map_;
};

//! @}

} // namespace Gringo::Input
