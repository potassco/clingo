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
//! A span of program literals.
using WeightedLitSpan = std::span<std::pair<lit_t, weight_t>>;
//! A vector of program literals.
using WeightedLitVec = std::vector<std::pair<lit_t, weight_t>>;

//! The maximum literal.
static constexpr auto lit_max = std::numeric_limits<lit_t>::max();
//! The minimum literal.
static constexpr auto lit_min = -lit_max;

//! Abstract class connecting grounder and solver.
//!
//! The backend is repsonsible for passig grounded statements to the solver (or
//! other forms of backends).
class ProgramBackend {
  public:
    //! Destroy the backend.
    virtual ~ProgramBackend() = default;

    //! Return a fresh literal.
    //!
    //! All literals should be created using this function.
    //!
    //! @return the fresh literal
    auto next_lit() -> lit_t { return do_next_lit(); }

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

    //! Show the atom with the given symbol and program literal.
    //!
    //! @param sym the symbol to show
    //! @param body the condition when to show the symbol
    void show_atom(Symbol sym, lit_t lit) { do_show_atom(sym, lit); }

    //! Add an edge for acyclicity checking.
    //!
    //! @param u the source vertex
    //! @param v the target vertex
    //! @param body the body of the statement
    void edge(id_t u, id_t v, LitSpan body) { do_edge(u, v, body); }

    //! Add a heuristic directive.
    //!
    //! @param atom the atom to modify heuristically
    //! @param weight the weight of the modification
    //! @param prio the priority of the modification
    //! @param type the type of the modification
    //! @param body the body of the directive.
    void heuristic(lit_t atom, int32_t weight, int32_t prio, HeuristicType type, LitSpan body) {
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
    //! @param lit the literal to minimize
    //! @param weight the weight of the literal
    //! @param priority the priority of the literal
    void minimize(lit_t lit, weight_t weight, weight_t priority) { do_minimize(lit, weight, priority); }

  private:
    virtual auto do_next_lit() -> lit_t = 0;
    virtual void do_rule(LitSpan head, LitSpan body, bool choice) = 0;
    virtual void do_bd_aggr(lit_t head, WeightedLitSpan body, int32_t bound) = 0;
    virtual void do_show(Symbol sym, LitSpan body) = 0;
    virtual void do_show_atom(Symbol sym, lit_t lit) = 0;
    virtual void do_edge(id_t u, id_t v, LitSpan body) = 0;
    virtual void do_heuristic(lit_t atom, weight_t weight, weight_t prio, HeuristicType type, LitSpan body) = 0;
    virtual void do_external(lit_t atom, ExternalType type) = 0;
    virtual void do_project(lit_t atom) = 0;
    virtual void do_minimize(lit_t lit, weight_t weight, weight_t priority) = 0;
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
    //! @note The caller is repsonsible to assign unique ids.
    //!
    //! @param id the unique term id
    //! @param num the number
    void num(id_t id, weight_t num) { do_num(id, num); }
    //! Add a theory string.
    //!
    //! @note The caller is repsonsible to assign unique ids.
    //!
    //! @param id the unique term id
    //! @param str the string
    void str(id_t id, char const *str) { do_str(id, str); }
    //! Add a theory function.
    //!
    //! @note The caller is repsonsible to assign unique ids.
    //! @pre The name must be an id to a string.
    //!
    //! @param id the unique term id
    //! @param name the term id of the function name
    //! @param args the term ids of the arguments
    void fun(id_t id, id_t name, IdSpan args) { do_fun(id, name, args); }

    //! Add a theory tuple.
    //!
    //! @note The caller is repsonsible to assign unique ids.
    //!
    //! @param id the unique term id
    //! @param type the type of the tuple
    //! @param args the term ids of the arguments
    void tup(id_t id, TheoryTermTupleType type, IdSpan args) { do_tup(id, type, args); }

    //! Add a theory element.
    //!
    //! @note The caller is repsonsible to assign unique ids.
    //!
    //! @param id the unique element id
    //! @param terms the terms forming the tuple
    //! @param cond the condition of the element
    void elem(id_t id, IdSpan terms, LitSpan cond) { do_elem(id, terms, cond); }

    //! Add a theory atom.
    //!
    //! @param atom_or_zero the literal of the atom (zero for directives)
    //! @param name the name of the atom (must be a function or symbol)
    //! @param elems the elements of the atom
    //! @param guard the optional guard of the atom
    void atom(lit_t atom_or_zero, id_t name, IdSpan elems, std::optional<std::pair<id_t, id_t>> guard) {
        assert(atom_or_zero >= 0);
        do_atom(atom_or_zero, name, elems, guard);
    }

  private:
    virtual void do_num(id_t id, weight_t num) = 0;
    virtual void do_str(id_t id, char const *str) = 0;
    virtual void do_fun(id_t id, id_t name, IdSpan args) = 0;
    virtual void do_tup(id_t id, TheoryTermTupleType type, IdSpan args) = 0;
    virtual void do_elem(id_t id, IdSpan terms, LitSpan cond) = 0;
    virtual void do_atom(lit_t atom_or_zero, id_t name, IdSpan elems, std::optional<std::pair<id_t, id_t>> guard) = 0;
};
//! A unique pointer for a theory backend.
using UTheoryBackend = std::unique_ptr<TheoryBackend>;

//! Class similar to Potassco::TheoryData but with automatic id generation.
class TheoryData {
  public:
    using IdVec = Util::small_vector<id_t, 4>;
    using LitVec = Util::small_vector<lit_t, 4>;

    TheoryData(SymbolStore &store, UTheoryBackend backend) : store_{&store}, backend_{std::move(backend)} {}

    //! Add a number term.
    //!
    //! @param num the number
    //! @return the term id
    auto num(weight_t num) -> id_t;

    //! Add a string term.
    //!
    //! @param str the string
    //! @return the term id
    auto str(String str) -> id_t;

    //! Add a function term.
    //!
    //! @param name the name of the function
    //! @param args the arguments of the function
    //! @return the term id
    auto fun(String name, IdVec args) -> id_t;

    //! Overload for spans.
    auto fun(String name, IdSpan args) -> id_t { return fun(name, IdVec{args.begin(), args.end()}); }

    //! Add a tuple term.
    //!
    //! @param type the type of the tuple
    //! @param args the arguments of the tuple
    //! @return the term id
    auto tup(TheoryTermTupleType type, IdVec args) -> id_t;

    //! Overload for spans.
    auto tup(TheoryTermTupleType type, IdSpan args) -> id_t { return tup(type, IdVec{args.begin(), args.end()}); }

    //! Convert a symbol into a theory term.
    //!
    //! @param sym the symbol to convert
    //! @return the term id
    auto sym(Symbol sym) -> id_t;

    //! Add a theory element.
    //!
    //! @param tuple the ids of terms forming the tuple
    //! @param cond the condition
    //! @return the element id
    auto elem(IdVec tuple, LitVec cond) -> id_t;

    //! Overload for spans.
    auto elem(IdSpan tuple, LitSpan cond) -> id_t {
        return elem(IdVec{tuple.begin(), tuple.end()}, LitVec{cond.begin(), cond.end()});
    }

    //! Add a theory atom.
    //!
    //! The first argument is a function to set the literal of the atom. It is
    //! only called if a fresh atom has been inserted. It must return a
    //! non-negative literal. The literal can be zero for directives.
    //!
    //! @param atom function to set the literal
    //! @param name the name of the atom
    //! @param elems the element ids
    //! @param guard the optional guard of the atom
    //! @return the literal of the theory atom
    auto atom(std::function<lit_t()> const &atom, Symbol name, IdVec elems,
              std::optional<std::pair<String, id_t>> guard) -> lit_t;

    //! Overload for spans.
    auto atom(std::function<lit_t()> const &atom, Symbol name, IdSpan elems,
              std::optional<std::pair<String, id_t>> guard) -> lit_t {
        return this->atom(atom, name, IdVec{elems.begin(), elems.end()}, guard);
    }

    //! Clear the theory data.
    void reset() noexcept;

  private:
    using StringMap = Util::unordered_map<SharedString, id_t>;
    using NumMap = Util::unordered_map<weight_t, id_t>;
    using FunMap = Util::unordered_map<std::pair<id_t, IdVec>, id_t>;
    using TupMap = Util::unordered_map<std::pair<TheoryTermTupleType, IdVec>, id_t>;
    using ElemMap = Util::unordered_map<std::pair<IdVec, LitVec>, id_t>;
    using AtomMap = Util::unordered_map<std::tuple<id_t, IdVec, std::optional<std::pair<id_t, id_t>>>, lit_t>;

    //! Helper to insert elemens into the term maps.
    //!
    //! @param map the map to insert in
    //! @param val the value to insert
    //! @return same as map.insert
    template <class M, class V> auto insert_(M &map, V &&val) -> std::pair<typename M::iterator, bool>;

    SymbolStore *store_;
    UTheoryBackend backend_;
    Util::OutputBuffer buf_;
    StringMap strings_;
    NumMap nums_;
    FunMap funs_;
    TupMap tups_;
    ElemMap elems_;
    AtomMap atoms_;
    id_t ids_ = 0;
};

//! Create an output that forwards ground statements to a backend.
//!
//! Backends accept a simpler format as provided by the grounder. This output
//! brings the statements into the required form and passes them to the
//! backend.
//!
//! @param store the store holding symbols
//! @param backend the target backend
auto make_backend_output(SymbolStore &store, ProgramBackend &backend, TheoryData &theory) -> UOutputStm;

//! @}

} // namespace Clingo::Output
