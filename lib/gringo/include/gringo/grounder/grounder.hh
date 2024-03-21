#include <gringo/input/program.hh>

namespace Gringo {

enum class AtomState : uint64_t {
    // Indicates that the atom is derived by a fact.
    fact = 0,
    // Indicates that the atom is derived by some rule but not a fact.
    derived = 1,
    // Indicates that the atom is derived by some external but not a rule or fact.
    external = 2,
    // At the time rule (1) is grounded, atom x is not yet defined.
    // Once (2) has been grounded, there is a definition for it.
    //
    //   a :- not x. (1)
    //   x :- a.     (2)
    //
    // The flag indicates atoms that have neither been derived by facts, rules, or externals.
    unknown = 3,
};

struct AtomInfo {
    // A unique id among all atoms.
    mutable uint64_t id;
    // Indicates at which iteration an atom has been grounded.
    // It would be ideal if it were possible to get rid of this field.
    mutable uint64_t gen : 61;
    mutable AtomState state : 2;
};

using Atom = std::pair<Symbol, AtomInfo>;

class Domain {
  public:
    [[nodiscard]] auto contains(Symbol const &sym) const -> bool;
    [[nodiscard]] auto operator[](size_t pos) -> Atom &;
    [[nodiscard]] auto operator[](size_t pos) const -> Atom const &;

  private:
    Util::ordered_map<Symbol, AtomInfo> atoms_;
    size_t gen_ = 0;
};

//! A grounder for logic programs.
class Grounder {
  public:
    Grounder(Logger &log, SymbolStore &store, Input::RewriteOptions opts)
        : log_{log}, store_{store}, prg_{std::move(opts)} {};
    //! Parse a program from the given string.
    void parse(std::string_view prg);
    //! Parse the given files.
    void parse(std::vector<std::string> const &files);
    //! Prepare a program for grounding.
    void prepare();
    //! Ground the program.
    void ground(Input::ProgramParamVec const &params);

    //! Output the current unprocessed program.
    void output_unprocessed_program(std::ostream &out);
    //! Output the current program.
    void output_program(std::ostream &out);

  private:
    Logger &log_;
    SymbolStore &store_;
    Input::UnprocessedProgram unprocessed_prg_;
    Input::Program prg_;
};

} // namespace Gringo
