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
using id_t = size_t;
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
//! A span of program literals.
using LitSpan = std::span<lit_t const>;
//! A vector of program literals.
using LitVec = std::vector<lit_t>;
//! A vector of program literals.
using WeightedLitSpan = std::span<std::pair<lit_t, weight_t>>;

//! A conditional literal consisting of a conclusion and a premise.
//!
//! Conclusions and premises are represented by ids referring to conditions,
//! which, in turn, are sets of literals. In the current implementation, the
//! condition is either absent or consists of exactly one literal. If the
//! conclusion is absent, it is considered to be false.
using CondLit = std::pair<std::optional<id_t>, id_t>;
//! A span of conditional literals.
using CondLitSpan = std::span<CondLit const>;
//! A vector of conditional literals.
using CondLitVec = std::vector<std::pair<std::optional<lit_t>, lit_t>>;

//! A sum aggregate element.
using BdAggrElem = std::pair<SymbolSpan, IndexSpan>;
//! A span of aggregate elements.
using BdAggrElemSpan = std::span<BdAggrElem const>;
//! A vector of aggregate elements.
using BdAggrElemVec = std::vector<BdAggrElem>;
//! Guard
using Guard = std::pair<Relation, Symbol>;
//! A span of guards.
using GuardSpan = std::span<Guard const>;
//! A vector of guards.
using GuardVec = std::vector<Guard>;

//! A set of numbers.
using NumberSet = Util::interval_set<Number>;

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

  private:
    virtual auto do_next_lit() -> lit_t = 0;
    virtual void do_rule(LitSpan head, LitSpan body, bool choice) = 0;
    virtual void do_bd_aggr(lit_t head, WeightedLitSpan body, int32_t bound) = 0;
    virtual void do_show(Symbol sym, LitSpan body) = 0;
};
using UBackend = std::unique_ptr<Backend>;

//! Create an output that forwards ground statements to a backend.
//!
//! Backends accept a simpler format as provided by the grounder. This output
//! brings the statements into the required form and passes them to the
//! backend.
//!
//! @param backend the target Backend
auto make_backend_output(Backend &backend) -> UOutputStm;

//! @}

} // namespace Clingo::Output
