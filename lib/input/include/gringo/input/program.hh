#pragma once

#include <gringo/input/statement.hh>

#include <gringo/core/logger.hh>

#include <gringo/util/enum.hh>
#include <gringo/util/ordered_map.hh>
#include <gringo/util/ordered_set.hh>

namespace Gringo::Input {

//! @addtogroup input_program
//! @{

//! Enumeration to select variables to project.
//!
//! @see Projection
enum class ProjectionMode : uint8_t {
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
    //! The statements in the program part.
    StmVec stms;
    //! The facts in the program part.
    SymbolVec facts;
};
//! Statements grouped by parts.
using ProgramPartVec = std::vector<ProgramPart>;

using ProgramSig = std::pair<String, size_t>;
using ProgramParam = std::pair<String, std::vector<Symbol>>;
using ProgramParamVec = std::vector<ProgramParam>;

//! Program grouping unprocessed statements.
class UnprocessedProgram {
  public:
    //! Ensure that the next statement is added to the base part.
    void ensure_base() { ensure_base_ = true; }

    //! Add a statement.
    void add(SymbolStore &store, Stm stm);

    //! Reset the program to its initial state removing all added statements.
    void clear();

    //! Mark symbols occuring in the program.
    void mark(SymbolCollector &gc) const;

    //! Unprocessed statemtents.
    [[nodiscard]] auto parts() const -> ProgramPartVec const & { return parts_; }
    //! Unprocessed const statements.
    [[nodiscard]] auto const_stms() const -> std::vector<StmConst> const & { return const_stms_; }
    //! Theory statements.
    [[nodiscard]] auto thy_stms() const -> std::vector<StmTheory> const & { return thy_stms_; }
    //! Meta statements.
    [[nodiscard]] auto meta_stms() const -> StmVec const & { return meta_stms_; }
    //! Ensure base.
  private:
    ProgramPartVec parts_;
    std::vector<StmConst> const_stms_;
    std::vector<StmTheory> thy_stms_;
    StmVec meta_stms_;
    bool ensure_base_ = true;
};

//! The type of a component.
enum class ComponentType : uint8_t {
    domain = 1,      //!< The component evaluates to facts.
    single_pass = 2, //!< The component can be grounded in one pass.
};
consteval void is_bit_set_enum(ComponentType flags);

//! A refined component.
//!
//! A component consists of a (non-empty) set of statements and a set of incomplete literals.
//! Instances of incomplete literals are added while grounding a component.
//! In case of negative literals, instances might also be added after grounding the component.
//! We cannot assume that an instance of a incomplete negative literal is true
//! if there has been no instance deriving its positive counterpart previously.
struct Component {
    //! The statements in the component.
    std::vector<Stm const *> stms;
    //! This vector captures literals that are not yet complete.
    Util::ordered_map<Term const *, Util::ordered_set<Term const *>> incomplete;
    //! The type of the componnent.
    ComponentType type;
};

//! The list of components in groundable order.
using Components = std::vector<std::vector<Component>>;

//! Interface to process a rewritten and analyzed input program.
class DependencyBuilder {
  public:
    //! Default destructor.
    virtual ~DependencyBuilder() = default;
    //! Add parts to ground.
    void param(ProgramParam const &param) { do_param(param); }
    //! Add meta statements.
    void meta(std::vector<Stm> const &stms) { do_meta(stms); }
    //! Add facts.
    void fact(std::vector<Symbol> const &facts) { do_fact(facts); }
    //! Add components.
    [[nodiscard]] auto components(Components const &comps) -> bool { return do_components(comps); }

  private:
    virtual void do_param(ProgramParam const &param) = 0;
    virtual void do_meta(std::vector<Stm> const &stms) = 0;
    virtual void do_fact(std::vector<Symbol> const &facts) = 0;
    [[nodiscard]] virtual auto do_components(Components const &comps) -> bool = 0;
};

//! A program consisting of parts.
class Program {
  public:
    //! Initialize a program with a rewrite level.
    //!
    //! (The highest rewrite level has to be used for grounding.)
    Program(RewriteOptions opts) : opts_{opts} {}
    //! Join with the given unprocessed program.
    //!
    //! If fresh const statements are added, they will be merged with the previous ones.
    //! However, they are only applied once to newly added statements.
    void join(Logger &log, SymbolStore &store, UnprocessedProgram const &prg);
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
            std::transform(pum.begin(), pum.end(), std::back_inserter(ids), [](auto const &x) { return x.second; });
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

    //! Prepare the statements in a program for grounding.
    [[nodiscard]] auto analyze(SymbolStore &store, ProgramParamVec const &params, DependencyBuilder &bld) const -> bool;

    //! Mark symbols occuring in the program.
    void mark(SymbolCollector &gc) const;

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
    [[nodiscard]] static auto param_map_(SymbolStore &store,
                                         ProgramPart const &part) -> Util::ordered_map<String, String>;
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
