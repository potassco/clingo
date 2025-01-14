#include <clingo/core/output.hh>

#include <clingo/util/checked_math.hh>
#include <clingo/util/enum.hh>
#include <clingo/util/graph.hh>
#include <clingo/util/interval_set.hh>
#include <clingo/util/ordered_map.hh>
#include <clingo/util/unordered_map.hh>

namespace Clingo::Output {

//! @addtogroup output
//! @{

//! An id to refer to elements of a logic program.
//!
//! The semantics of ids is context dependent.
using id_t = uint32_t;
//! A span of ids.
using IdSpan = std::span<id_t const>;
//! An signed version of `id_t`.
using sid_t = std::make_signed_t<id_t>;
//! A program literal.
//!
//! A program literal must be in the range of `lit_min` to `lit_max` excluding
//! number 0.
using lit_t = int32_t;
//! A program atom.
//!
//! A program atom must be in the range of 1 to `lit_max`.
using atom_t = uint32_t;
//! A weight used in weight and minimize constraints.
using weight_t = int32_t;
//! Type to represent sums of weights.
using sum_t = int64_t;
//! A span of program literals.
using LitSpan = std::span<lit_t const>;
//! A vector of literals.
using LitVec = std::vector<lit_t>;
//! A vector of program literals.
using WeightedLitSpan = std::span<std::pair<lit_t, weight_t>>;

//! The maximum literal.
static constexpr auto lit_max = std::numeric_limits<lit_t>::max();
//! The minimum literal.
static constexpr auto lit_min = -lit_max;

//! Abstract class connecting grounder and solver.
//!
//! The backend is repsonsible for passig grounded statements to the solver (or
//! other forms of backends).
class Backend {
  public:
    //! Destroy the backend.
    virtual ~Backend() = default;

    //! Return a fresh literal.
    //!
    //! All literals should be created using this function.
    //!
    //! @return the fresh literal
    auto next_lit() -> Output::lit_t { return do_next_lit(); }

    //! Define a weight constraint.
    //!
    //! @param head the literal that is derived
    //! @param body the weighted body literals
    //! @param bound the lower bound of the constraint
    void bd_aggr(lit_t head, WeightedLitSpan body, int32_t bound) { do_bd_aggr(head, body, bound); }

    //! Add a disjunctive or choice rule.
    //!
    //! Note that negative literals in the head are supported.
    //!
    //! @param head the literals forming the head
    //! @param body the literals forming the body
    //! @param choice whether the rule is a choice or disjunctive rule
    void rule(LitSpan head, LitSpan body, bool choice) { do_rule(head, body, choice); }

    //! Show the given symbol if the given conditon is true.
    //!
    //! @param sym the symbol to show
    //! @param body the condition when to show the symbol
    void show(Symbol sym, LitSpan body) { do_show(sym, body); }

    //! Add an edge for acyclicity checking.
    //!
    //! @param u the source vertex
    //! @param v the target vertex
    //! @param body the body of the statement
    void edge(Output::id_t u, Output::id_t v, Output::LitSpan body) { do_edge(u, v, body); }

    //! Add a heuristic directive.
    //!
    //! @param atom the atom to modify heuristically
    //! @param weight the weight of the modification
    //! @param prio the priority of the modification
    //! @param type the type of the modification
    void heuristic(lit_t atom, int32_t weight, int32_t prio, HeuristicType type, Output::LitSpan body) {
        assert(atom > 0);
        do_heuristic(atom, weight, prio, type, body);
    }
    //! Declare the given atom as external.
    //!
    //! @param atom the atom to declare external
    //! @param type the truth value of the atom
    void external(lit_t atom, ExternalType type) {
        assert(atom > 0);
        do_external(atom, type);
    }

    //! Project the given atom.
    //!
    //! @param atom the atom to project
    void project(lit_t atom) {
        assert(atom > 0);
        do_project(atom);
    }

    //! Project the given atom.
    //!
    //! @param atom the literal to minimize
    //! @param weight the weight of the literal
    //! @param priority the priority of the literal
    void minimize(lit_t lit, weight_t weight, weight_t priority) { do_minimize(lit, weight, priority); }

    //! Add a theory number.
    //!
    //! @note The caller is repsonsible to assign unique ids.
    //!
    //! @param id the unique term id
    //! @param num the number
    void theory_num(id_t id, weight_t num) { do_theory_num(id, num); }
    //! Add a theory string.
    //!
    //! @note The caller is repsonsible to assign unique ids.
    //!
    //! @param id the unique term id
    //! @param str the string
    void theory_str(id_t id, char const *str) { do_theory_str(id, str); }
    //! Add a theory function.
    //!
    //! @note The caller is repsonsible to assign unique ids.
    //! @pre The name must be an id to a string.
    //!
    //! @param id the unique term id
    //! @param name the term id of the function name
    //! @param args the term ids of the arguments
    void theory_fun(id_t id, id_t name, IdSpan args) { do_theory_fun(id, name, args); }

    //! Add a theory tuple.
    //!
    //! @note The caller is repsonsible to assign unique ids.
    //!
    //! @param id the unique term id
    //! @param type the type of the tuple
    //! @param args the term ids of the arguments
    void theory_tup(id_t id, TheoryTermTupleType type, Output::IdSpan args) { do_theory_tup(id, type, args); }

    //! Add a theory element.
    //!
    //! @note The caller is repsonsible to assign unique ids.
    //!
    //! @param id the unique element id
    //! @param terms the terms forming the tuple
    //! @param cond the condition of the element
    void theory_elem(id_t id, Output::IdSpan terms, Output::LitSpan cond) { do_theory_elem(id, terms, cond); }

    //! Add a theory atom.
    //!
    //! @param atom_or_zero the literal of the atom (zero for directives)
    //! @param name the name of the atom (must be a function or symbol)
    //! @param elems the elements of the atom
    //! @param guard the optional guard of the atom
    void theory_atom(lit_t atom_or_zero, id_t name, Output::IdSpan elems, std::optional<std::pair<id_t, id_t>> guard) {
        assert(atom_or_zero >= 0);
        do_theory_atom(atom_or_zero, name, elems, guard);
    }

  private:
    virtual auto do_next_lit() -> lit_t = 0;
    virtual void do_rule(LitSpan head, LitSpan body, bool choice) = 0;
    virtual void do_bd_aggr(lit_t head, WeightedLitSpan body, int32_t bound) = 0;
    virtual void do_show(Symbol sym, LitSpan body) = 0;
    virtual void do_edge(Output::id_t u, Output::id_t v, Output::LitSpan body) = 0;
    virtual void do_heuristic(lit_t atom, weight_t weight, weight_t prio, HeuristicType type, Output::LitSpan body) = 0;
    virtual void do_external(lit_t atom, ExternalType type) = 0;
    virtual void do_project(lit_t atom) = 0;
    virtual void do_minimize(lit_t lit, weight_t weight, weight_t priority) = 0;
    virtual void do_theory_num(id_t id, weight_t num) = 0;
    virtual void do_theory_str(id_t id, char const *str) = 0;
    virtual void do_theory_fun(id_t id, id_t name, IdSpan args) = 0;
    virtual void do_theory_tup(id_t id, TheoryTermTupleType type, Output::IdSpan args) = 0;
    virtual void do_theory_elem(id_t id, Output::IdSpan terms, Output::LitSpan cond) = 0;
    virtual void do_theory_atom(lit_t atom_or_zero, id_t name, Output::IdSpan elems,
                                std::optional<std::pair<id_t, id_t>> guard) = 0;
};
using UBackend = std::unique_ptr<Backend>;

//! Create an output that forwards ground statements to a backend.
//!
//! Backends accept a simpler format as provided by the grounder. This output
//! brings the statements into the required form and passes them to the
//! backend.
//!
//! @param store the store holding symbols
//! @param backend the target backend
auto make_backend_output(SymbolStore &store, Backend &backend) -> UOutputStm;

//! @}

} // namespace Clingo::Output
