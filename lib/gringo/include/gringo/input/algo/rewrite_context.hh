#pragma once

#include <gringo/input/program.hh>

namespace Gringo::Input {

//! Enum for unary/binary operator arities.
enum class Arity {
    unary = 0,  //!< unary arity
    binary = 1, //!< binary arity
};

//! Enum for operator associativities.
enum class Associativity {
    left = 0,      //!< left associative
    right = 1,     //!< right associative
    non_assoc = 2, //!< no associativity
};

//! A parser for theory terms.
class TheoryTermParser {
  public:
    //! Add a theory operator definition to the term parser.
    void add(Logger &log, TheoryOpDefinition const &def);

    //! Check if the given operator is in the parse table raising a runtime error if absent.
    void check_operator(Logger &log, String op, Arity arity, Location loc) const;

    //! Parses the given unparsed term, replacing it by nested theory functions.
    auto parse(Logger &log, TheoryTermUnparsed const &term) const -> TheoryTerm;

  private:
    using Table = Util::unordered_map<std::pair<String, Arity>, std::pair<int, Associativity>>;
    using Stack = std::vector<std::pair<String, Arity>>;
    using Terms = std::vector<TheoryTerm>;

    //! Get priority and associativity of the given binary operator.
    auto priority_and_associativity_(String op) const -> std::pair<int, Associativity>;

    //! Get priority of the given unary or binary operator.
    auto priority_(String op, Arity arity) const -> int;

    //! Returns true if the stack has to be reduced.
    //!
    //! Returns true if the priority of the given binary operator is lower than the preceeding operator on the stack.
    auto check_(String op) const -> bool;

    //! Combines the last unary or binary term on the stack.
    void reduce_() const;

    Table table_;
    mutable Terms terms_;
    mutable Stack stack_;
};

//! A parser for theory atoms.
class TheoryAtomParser {
  public:
    //! Add a theory statement to the theory atom parser.
    void add_theory(Logger &log, StmTheory const &stm);

    //! Parse the given theory atom.
    template <bool has_sign>
    auto parse(Logger &log, TheoryAtom<has_sign> const &atom, bool fact) const -> std::optional<TheoryAtom<has_sign>>;

  private:
    using ParserIndex = size_t;
    using GuardTable = std::pair<StringSet, ParserIndex>;
    using AtomTable =
        Util::unordered_map<std::pair<String, int>, std::tuple<TheoryAtomType, ParserIndex, std::optional<GuardTable>>>;

    std::vector<TheoryTermParser> term_parsers_;
    AtomTable atom_table_;
};

//! A vector of term pairs where the second has been substituted by the first in some other term.
using AuxTermVec = std::vector<std::pair<Term, Term>>;

//! Map from identifiers to constants.
using ParamMap = Util::ordered_set<String>;

//! Helper to pass arguments to rewrite functions.
class RewriteContext {
  public:
    //! Helper to pop auxiliary variable assignments.
    struct pop_ {
        //! Pop the last variable term map pushed.
        void operator()(RewriteContext *ctx) const {
            if (ctx != nullptr) {
                ctx->pop();
                ctx = nullptr;
            }
        }
    };
    //! Helper to pop auxiliary variable assignments.
    using Guard = std::unique_ptr<RewriteContext, pop_>;
    //! Construct a rewrite context.
    RewriteContext(Logger &log, SymbolStore &store, RewriteOptions const &opts, TheoryAtomParser const &parser,
                   ParamMap const &param_map, ConstMap const &const_map)
        : log_{log}, opts_{opts}, parser_{parser}, const_map_{const_map}, param_map_{param_map}, gen_{store, {}, "_A"} {
    }
    //! Delete move/copy constructor.
    RewriteContext(RewriteContext &&) noexcept = delete;
    //! Initialize/reset the name generator.
    void init(StringSet names, char const *prefix) {
        aux_.clear();
        gen_.init(std::move(names), prefix);
    }
    //! Get the logger.
    [[nodiscard]] auto logger() const -> Logger & { return log_; }
    //! Get the logger.
    [[nodiscard]] auto options() const -> RewriteOptions const & { return opts_; }
    //! Get the parser.
    [[nodiscard]] auto parser() const -> TheoryAtomParser const & { return parser_; }
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
        return aux_.back();
    }
    //! Pop the last variable term map pushed.
    void pop() {
        assert(!aux_.empty());
        aux_.pop_back();
    }
    //! Push a fresh variable term map.
    [[nodiscard]] auto push() -> Guard {
        aux_.emplace_back();
        return Guard{this};
    }

  private:
    Logger &log_;                    //!< Logger to report messages.
    RewriteOptions const &opts_;     //! The rewrite options.
    TheoryAtomParser const &parser_; //!< The theory parser.
    ConstMap const &const_map_;      //!< Constant definitions.
    ParamMap const &param_map_;      //!< Map of Parameters.
    NameGen gen_;                    //!< Generator to create fresh variable names.
    std::vector<AuxTermVec> aux_;    //!< Vector of variable term pairs.
};

} // namespace Gringo::Input
