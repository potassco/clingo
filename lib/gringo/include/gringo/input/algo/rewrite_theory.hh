#pragma once

#include <gringo/input/program.hh>

namespace Gringo::Input {

enum class Arity {
    unary = 0,
    binary = 1,
};

enum class Associativity {
    left = 0,
    right = 1,
    non_assoc = 2,
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

//! Parse theory atoms in the given statement with the given parser.
auto rewrite_theory(Logger &log, TheoryAtomParser const &parser, Stm const &stm) -> std::optional<Stm>;

} // namespace Gringo::Input
