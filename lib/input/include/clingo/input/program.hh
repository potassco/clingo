#pragma once

#include <clingo/input/statement.hh>

#include <clingo/core/logger.hh>

#include <clingo/util/enum.hh>
#include <clingo/util/ordered_map.hh>
#include <clingo/util/ordered_set.hh>

namespace CppClingo::Input {

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
using ConstMap = Util::ordered_map<SharedString, std::pair<StmConst, SharedSymbol>>;

//! Map from parameters to their replacements.
using ParamUnmap = Util::ordered_map<SharedString, SharedString>;

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

//! Program grouping unprocessed statements.
class UnprocessedProgram {
  public:
    //! Ensure that the next statement is added to the base part.
    void ensure_base() { ensure_base_ = true; }

    //! Add a statement.
    void add(SymbolStore &store, Stm stm);

    //! Reset the program to its initial state removing all added statements.
    void clear();

    //! Check if the program is empty.
    [[nodiscard]] auto empty() const -> bool;

    //! Mark symbols occurring in the program.
    void mark(SymbolCollector &gc) const;

    //! Join with another unprocessed program.
    void join(UnprocessedProgram const &other);

    //! Unprocessed statements.
    [[nodiscard]] auto parts() const -> ProgramPartVec const & { return parts_; }
    //! Meta statements.
    [[nodiscard]] auto meta_stms() const -> StmVec const & { return meta_stms_; }
    //! Ensure base.
  private:
    ProgramPartVec parts_;
    StmVec meta_stms_;
    bool ensure_base_ = true;
};

//! The type of a component.
//!
//! Note that the positive flag is just about negative cycles within the
//! component. The flag is also set to false if the component contains a
//! negative literal derived in a later refined component.
enum class ComponentType : uint8_t {
    positive = 1,    //!< The component does not contain a negative cycle.
    single_pass = 2, //!< The component can be grounded in one pass.
};
//! Indicate that the component type is a bitset.
CLINGO_ENABLE_BITSET_ENUM(ComponentType);

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
    //! The literals a component depends on.
    Util::unordered_set<std::tuple<String, size_t, bool>> depend;
    //! This vector captures literals that are not yet complete.
    Util::ordered_map<Term const *, Util::ordered_set<Term const *>> incomplete;
    //! The type of the component.
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
    virtual void do_fact(SymbolVec const &facts) = 0;
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
        for (auto const &stm : script_stms_) {
            fun(stm);
        }
        for (auto const &stm : defined_stms_) {
            fun(stm);
        }
        for (auto const &[id, sym] : const_map_) {
            fun(Stm{StmConst{sym.first.loc(), sym.first.type(), sym.first.name(),
                             TermSymbol{location(sym.first.value()), *sym.second}}});
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
            auto ids = SharedStringArray{pum.begin(), pum.end(), [](auto const &x) { return *x.second; }};
            fun(StmProgram{loc, *sig.first, std::move(ids)});
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
    //! Get the meta statements in the program.
    auto meta_stms() -> StmVec const & { return meta_stms_; }

    //! Prepare the statements in a program for grounding.
    [[nodiscard]] auto analyze(SymbolStore &store, ProgramParamVec const &params, DependencyBuilder &bld) const -> bool;

    //! Mark symbols occurring in the program.
    void mark(SymbolCollector &gc) const;

    //! Check program and emit diagnostics.
    void check(Logger &log);

    //! Get a sorted vector of all signatures of theory directives in the program.
    [[nodiscard]] auto theory_directives() const -> TheorySigVec;

    //! Get the constants in the program.
    [[nodiscard]] auto const_map() const -> ConstMap const & { return const_map_; };

    //! Mark the given signature as provided.
    void mark_sig(Input::Sig const &sig);

  private:
    //! The signature of a program part.
    //!
    //! (Parameters are numbered from 1 to n.)
    using Signature = std::pair<SharedString, size_t>;
    //! Map from signatures to actual program parts.
    using PartMap = Util::ordered_map<Signature, ProgramPart>;

    //! Gather all identifiers appearing in a program part.
    [[nodiscard]] static auto param_map_(SymbolStore &store, ProgramPart const &part) -> ParamUnmap;
    //! Replace all bound parameters in a statement by parsable ids.
    [[nodiscard]] static auto unmap_(SymbolStore &store, ParamUnmap const &pum, Stm const &stm) -> std::optional<Stm>;

    //! The rewrite level of the program.
    RewriteOptions opts_;
    //! The meta statements in the program.
    StmVec meta_stms_;
    //! Script statements.
    std::vector<StmScript> script_stms_;
    //! Script statements.
    std::vector<StmDefined> defined_stms_;
    //! Theory statements.
    std::vector<StmTheory> thy_stms_;
    //! The map of program parts.
    PartMap parts_;
    //! The constants and their values.
    ConstMap const_map_;
    //! Signatures provided by the program.
    SharedSigSet provide_;
    //! Signatures the program depends on.
    Util::ordered_map<SharedSig, Location> depend_;
    //! Already checked dependencies.
    size_t depend_offset_ = 0;
};

//! @}

} // namespace CppClingo::Input
