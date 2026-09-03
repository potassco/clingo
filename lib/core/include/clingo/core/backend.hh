#pragma once

#include <clingo/core/core.hh>
#include <clingo/core/symbol.hh>

#include <clingo/util/hash.hh>
#include <clingo/util/small_vector.hh>

#include <ostream>

namespace CppClingo {

//! @addtogroup core_output
//! @{

//! An id to refer to elements of a logic program.
//!
//! The semantics of ids is context dependent.
using prg_id_t = uint32_t;
//! A span of ids.
using PrgIdSpan = std::span<prg_id_t const>;
//! A vector of ids.
using PrgIdVec = std::vector<prg_id_t>;
//! An signed version of `id_t`.
using prg_sid_t = std::make_signed_t<prg_id_t>;
//! A program literal.
//!
//! A program literal must be in the range of `lit_min` to `lit_max` excluding
//! number 0.
using prg_lit_t = int32_t;
//! A program atom.
//!
//! A program atom must be in the range of 1 to `lit_max`.
using prg_atom_t = uint32_t;
//! A weight used in weight and minimize constraints.
using prg_weight_t = int32_t;
//! Type to represent sums of weights.
using prg_sum_t = int64_t;
//! A span of program literals.
using PrgLitSpan = std::span<prg_lit_t const>;
//! A vector of literals.
using PrgLitVec = std::vector<prg_lit_t>;
//! A span of program literals.
using WeightedPrgLitSpan = std::span<std::pair<prg_lit_t, prg_weight_t> const>;
//! A vector of program literals.
using WeightedPrgLitVec = std::vector<std::pair<prg_lit_t, prg_weight_t>>;

//! The maximum id.
static constexpr auto prg_id_max = std::numeric_limits<prg_id_t>::max() - 1;
//! The maximum literal.
static constexpr auto prg_lit_max = std::numeric_limits<prg_lit_t>::max();
//! The minimum literal.
static constexpr auto prg_lit_min = -prg_lit_max;

class Literal; // defined in the Literal task

//! A program atom (positive by construction).
class Atom {
  public:
    constexpr Atom() = default;
    [[nodiscard]] constexpr auto index() const noexcept -> prg_atom_t { return rep_; }
    [[nodiscard]] constexpr auto to_lit(bool sign = false) const noexcept -> Literal; // defined after Literal
    [[nodiscard]] auto hash() const -> size_t { return Util::value_hash(rep_); }
    friend constexpr auto operator==(Atom, Atom) noexcept -> bool = default;
    friend constexpr auto operator<=>(Atom, Atom) noexcept = default;
    static constexpr auto to_rep(Atom a) noexcept -> prg_atom_t { return a.rep_; }
    static constexpr auto from_rep(prg_atom_t r) noexcept -> Atom {
        assert(r <= static_cast<prg_atom_t>(prg_lit_max));
        return Atom{r};
    }
    friend auto operator<<(std::ostream &out, Atom a) -> std::ostream & { return out << a.rep_; }
  private:
    constexpr explicit Atom(prg_atom_t r) noexcept : rep_{r} {}
    prg_atom_t rep_ = 0;
};
static_assert(std::is_trivially_copyable_v<Atom>);
static_assert(sizeof(Atom) == sizeof(prg_atom_t));

//! A program literal (signed atom; sign = polarity).
class Literal {
  public:
    constexpr Literal() = default;
    [[nodiscard]] constexpr auto atom() const noexcept -> Atom {
        return Atom::from_rep(static_cast<prg_atom_t>(rep_ < 0 ? -rep_ : rep_));
    }
    [[nodiscard]] constexpr auto sign() const noexcept -> bool { return rep_ < 0; }
    [[nodiscard]] constexpr auto operator~() const noexcept -> Literal { return Literal{static_cast<prg_lit_t>(-rep_)}; }
    [[nodiscard]] auto hash() const -> size_t { return Util::value_hash(rep_); }
    friend constexpr auto operator==(Literal, Literal) noexcept -> bool = default;
    friend constexpr auto operator<=>(Literal, Literal) noexcept = default;
    static constexpr auto to_rep(Literal l) noexcept -> prg_lit_t { return l.rep_; }
    static constexpr auto from_rep(prg_lit_t r) noexcept -> Literal {
        assert(r >= prg_lit_min);
        return Literal{r};
    }
    friend auto operator<<(std::ostream &out, Literal l) -> std::ostream & { return out << l.rep_; }
  private:
    constexpr explicit Literal(prg_lit_t r) noexcept : rep_{r} {}
    prg_lit_t rep_ = 0;
};
static_assert(std::is_trivially_copyable_v<Literal>);
static_assert(sizeof(Literal) == sizeof(prg_lit_t));

constexpr auto Atom::to_lit(bool sign) const noexcept -> Literal {
    auto v = static_cast<prg_lit_t>(rep_);
    return Literal::from_rep(sign ? static_cast<prg_lit_t>(-v) : v);
}

//! Abstract class connecting grounder and solver.
//!
//! The backend is responsible for passing grounded statements to the solver (or
//! other forms of backends).
class ProgramBackend {
  public:
    //! Destroy the backend.
    virtual ~ProgramBackend() = default;

    //! Initialize the backend.
    //!
    //! @param major the major version component
    //! @param minor the major version component
    //! @param revision the major version component
    //! @param incremental whether program updates have to be enabled
    void preamble(unsigned major, unsigned minor, unsigned revision, bool incremental) {
        do_preamble(major, minor, revision, incremental);
    }

    //! Start a new step.
    //!
    //! Indicate that a new program module is going to be added. This must be
    //! called directly after `preamble()` and after `end_step()` before
    //! calling any other methods of this interface.
    void begin_step() { do_begin_step(); }

    //! Finalize the current grounding step.
    //!
    //! A step can consist of multiple grounding steps.
    void end_ground() { do_end_ground(); }

    //! Finalize the current step.
    //!
    //! This function must be called right before solving. A step can consist
    //! of multiple grounding steps. Even with no solver attached, this
    //! function must be called to indicate that the current program module is
    //! complete.
    void end_step() { do_end_step(); }

    //! Return a fresh (positive) literal.
    //!
    //! All literals should be created using this function.
    //!
    //! @return the fresh literal
    auto next_lit() -> prg_lit_t { return do_next_lit(); }

    //! Get a factual literal if one existis.
    //!
    //! @return the literal
    auto fact_lit() -> std::optional<prg_lit_t> { return do_fact_lit(); }

    //! Define a weight constraint.
    //!
    //! @pre Literals in the head must be positive.
    //!
    //! @param head the literal that is derived
    //! @param body the weighted body literals
    //! @param bound the lower bound of the constraint
    //! @param choice whether the rule has a choice head
    void bd_aggr(PrgLitSpan head, WeightedPrgLitSpan body, int32_t bound, bool choice) {
        assert(std::ranges::all_of(head, [](auto const &x) { return x > 0; }));
        do_bd_aggr(head, body, bound, choice);
    }

    //! Add a disjunctive or choice rule.
    //!
    //! @pre Literals in the head must be positive.
    //!
    //! @param head the literals forming the head
    //! @param body the literals forming the body
    //! @param choice whether the rule is a choice or disjunctive rule
    void rule(PrgLitSpan head, PrgLitSpan body, bool choice) {
        assert(std::ranges::all_of(head, [](auto const &x) { return x > 0; }));
        do_rule(head, body, choice);
    }

    //! Show the given symbol if the given condition is true.
    //!
    //! @param sym the symbol to show
    //! @param body the condition when to show the symbol
    void show_term(Symbol sym, PrgLitSpan body) { do_show_term(sym, body); }

    //! Associate the given symbol with an id.
    //!
    //! @note Raises an exception if the given symbol already has an id.
    //!
    //! @param sym the symbol
    //! @param id the id
    void show_term(Symbol sym, prg_id_t id) { do_show_term(sym, id); }

    //! Show the symbol with the given id if the given condition is true.
    //!
    //! @param id the symbol to show
    //! @param body the condition when to show the symbol
    void show_term(prg_id_t id, PrgLitSpan body) { do_show_term(id, body); }

    //! Show the atom with the given symbol and program literal.
    //!
    //! @pre The literal must be positive.
    //!
    //! @param sym the symbol to show
    //! @param lit the literal when to show the symbol
    void show_atom(Symbol sym, prg_lit_t lit) { do_show_atom(sym, lit); }

    //! Add an edge for acyclicity checking.
    //!
    //! @param u the source vertex
    //! @param v the target vertex
    //! @param body the body of the statement
    void edge(prg_id_t u, prg_id_t v, PrgLitSpan body) { do_edge(u, v, body); }

    //! Add a heuristic directive.
    //!
    //! @pre The atom must be positive.
    //!
    //! @param atom the atom to modify heuristically
    //! @param weight the weight of the modification
    //! @param prio the priority of the modification
    //! @param type the type of the modification
    //! @param body the body of the directive.
    void heuristic(prg_lit_t atom, int32_t weight, int32_t prio, HeuristicType type, PrgLitSpan body) {
        assert(atom > 0);
        do_heuristic(atom, weight, prio, type, body);
    }
    //! Declare the given atom as external.
    //!
    //! @pre The atom must be positive.
    //!
    //! @param atom the atom to declare external
    //! @param type the truth value of the atom
    void external(prg_lit_t atom, ExternalType type) {
        assert(atom > 0);
        do_external(atom, type);
    }

    //! Project the given atoms.
    //!
    //! @pre The literals atoms must be positive.
    //!
    //! @param atoms the atoms to project
    void project(PrgLitSpan atoms) {
        assert(std::ranges::all_of(atoms, [](auto const &x) { return x > 0; }));
        do_project(atoms);
    }

    //! Assume the given literals.
    //!
    //! @param literals the literals to assume
    void assume(PrgLitSpan literals) { do_assume(literals); }

    //! Minimize the given weighted literals.
    //!
    //! @param priority the priority of the literal
    //! @param body the weighted literals of the minimize constraint
    void minimize(prg_weight_t priority, WeightedPrgLitSpan body) { do_minimize(priority, body); }

  private:
    virtual void do_preamble(unsigned major, unsigned minor, unsigned revision, bool incremental) = 0;
    virtual void do_begin_step() = 0;
    virtual void do_end_ground() = 0;
    virtual void do_end_step() = 0;
    virtual auto do_next_lit() -> prg_lit_t = 0;
    virtual auto do_fact_lit() -> std::optional<prg_lit_t> = 0;

    virtual void do_rule(PrgLitSpan head, PrgLitSpan body, bool choice) = 0;
    virtual void do_bd_aggr(PrgLitSpan head, WeightedPrgLitSpan body, int32_t bound, bool choice) = 0;
    virtual void do_show_term(Symbol sym, PrgLitSpan body) = 0;
    virtual void do_show_term(Symbol sym, prg_id_t id) = 0;
    virtual void do_show_term(prg_id_t id, PrgLitSpan body) = 0;
    virtual void do_show_atom(Symbol sym, prg_lit_t lit) = 0;
    virtual void do_edge(prg_id_t u, prg_id_t v, PrgLitSpan body) = 0;
    virtual void do_heuristic(prg_lit_t atom, prg_weight_t weight, prg_weight_t prio, HeuristicType type,
                              PrgLitSpan body) = 0;
    virtual void do_external(prg_lit_t atom, ExternalType type) = 0;
    virtual void do_project(PrgLitSpan atoms) = 0;
    virtual void do_assume(PrgLitSpan literals) = 0;
    virtual void do_minimize(prg_weight_t priority, WeightedPrgLitSpan body) = 0;
};
//! A unique pointer for a program backend.
using UProgramBackend = std::unique_ptr<ProgramBackend>;

//! Abstract class connecting grounder and theory data.
class TheoryBackend {
  public:
    //! Destroy the backend.
    virtual ~TheoryBackend() = default;

    //! Add a theory number.
    //!
    //! @note The caller is responsible to assign unique ids.
    //!
    //! @param id the unique term id
    //! @param num the number
    void num(prg_id_t id, prg_weight_t num) { do_num(id, num); }
    //! Add a theory string.
    //!
    //! @note The caller is responsible to assign unique ids.
    //!
    //! @param id the unique term id
    //! @param str the string
    void str(prg_id_t id, std::string_view str) { do_str(id, str); }
    //! Add a theory function.
    //!
    //! @note The caller is responsible to assign unique ids.
    //! @pre The name must be an id to a string.
    //!
    //! @param id the unique term id
    //! @param name the term id of the function name
    //! @param args the term ids of the arguments
    void fun(prg_id_t id, prg_id_t name, PrgIdSpan args) { do_fun(id, name, args); }

    //! Add a theory tuple.
    //!
    //! @note The caller is responsible to assign unique ids.
    //!
    //! @param id the unique term id
    //! @param type the type of the tuple
    //! @param args the term ids of the arguments
    void tup(prg_id_t id, TheoryTermTupleType type, PrgIdSpan args) { do_tup(id, type, args); }

    //! Add a theory element.
    //!
    //! @note The caller is responsible to assign unique ids.
    //!
    //! @param id the unique element id
    //! @param terms the terms forming the tuple
    //! @param cond the condition of the element
    void elem(prg_id_t id, PrgIdSpan terms, PrgLitSpan cond) { do_elem(id, terms, cond); }

    //! Add a theory atom.
    //!
    //! @param atom_or_zero the literal of the atom (zero for directives)
    //! @param name the name of the atom (must be a function or symbol)
    //! @param elems the elements of the atom
    //! @param guard the optional guard of the atom
    void atom(prg_lit_t atom_or_zero, prg_id_t name, PrgIdSpan elems,
              std::optional<std::pair<prg_id_t, prg_id_t>> guard) {
        assert(atom_or_zero >= 0);
        do_atom(atom_or_zero, name, elems, guard);
    }

  private:
    virtual void do_num(prg_id_t id, prg_weight_t num) = 0;
    virtual void do_str(prg_id_t id, std::string_view str) = 0;
    virtual void do_fun(prg_id_t id, prg_id_t name, PrgIdSpan args) = 0;
    virtual void do_tup(prg_id_t id, TheoryTermTupleType type, PrgIdSpan args) = 0;
    virtual void do_elem(prg_id_t id, PrgIdSpan terms, PrgLitSpan cond) = 0;
    virtual void do_atom(prg_lit_t atom_or_zero, prg_id_t name, PrgIdSpan elems,
                         std::optional<std::pair<prg_id_t, prg_id_t>> guard) = 0;
};
//! A unique pointer for a theory backend.
using UTheoryBackend = std::unique_ptr<TheoryBackend>;

//! @}

} // namespace CppClingo
