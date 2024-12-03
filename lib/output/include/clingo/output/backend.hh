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
using WeightedLitSpan = std::vector<std::pair<lit_t, weight_t>>;

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

//! Which weights have to be considered for cycle computation.
enum class CycleType : uint8_t { none, positive, negative, both };
[[maybe_unused]] void is_bit_set_enum(CycleType type);
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
    Backend(SymbolStore &store) : store_{&store} {
        // TODO: the store might be necessary later
        static_cast<void>(store_);
    };
    //! Return a fresh literal.
    //!
    //! All literals should be created using this function.
    //!
    //! @return the fresh literal
    auto next_lit() -> Output::lit_t;

    //! Negate the given literal.
    //!
    //! Introduces a Tseitin literal if the given literal is negative. Flag rec
    //! can be set to false if the literal occurrence does not occur in a
    //! positive cycle.
    //!
    //! @param lit the literal to negate
    //! @return the negated literal
    auto negate(lit_t lit) -> lit_t;

    //! Get a unique id identifying the given literals.
    //!
    //! The function stores a map from the set of literals to the unique identifiers.
    //!
    //! @param lits the literals
    auto cond(LitSpan lits) -> id_t;

    //! Define a conjunction of conditional literal.
    //!
    //! The given literal is derived by the given conditional literals.
    //!
    //! @param lit the literal that is derived
    //! @param elems the elements forming the conditional literal
    void cond_lit(lit_t lit, CondLitSpan elems);

    //! Define a sum aggregate.
    //!
    //! The condition ids of the aggregate elements can be obtained using
    //! function `cond()`.
    //!
    //! @param lit the literal that is derived
    //! @param fun the aggregate function
    //! @param elems the elements of the aggregate
    //! @param guard the aggregate guards
    void bd_aggr(lit_t lit, AggregateFunction fun, BdAggrElemSpan elems, GuardSpan guards);

    //! Add a disjunctive or choice rule.
    //!
    //! Note that negative literals in the head are supported.
    //!
    //! @param head the literals forming the head
    //! @param body the literals forming the body
    //! @param choice whether the rule is a choice or disjunctive rule
    void rule(LitSpan head, LitSpan body, bool choice);

    //! Finish a grounding step.
    //!
    //! Some language constructs require additional translation.
    //! Such translations are applied here.
    void end_step();

    //! She the given symbol if the given conditon is true.
    void show(Symbol sym, LitSpan body) { do_show(sym, body); }

    //! Destroy the backend.
    virtual ~Backend() = default;

  private:
    //! The maximum id of a condition.
    static constexpr auto cond_max = std::numeric_limits<id_t>::max() >> 1;

    //! Available condition types.
    //!
    //! Conditions are clauses associated with a literal. The equivalence between
    //! the literal and the clause is either established with an implication (a
    //! rule) or an equivalence.
    enum class EQType : uint8_t {
        none,        //!< the condition is not used
        implication, //!< only forward direction is necessary
        equivalence, //!< forward and backward directions are necessary
    };

    struct LitInfo {
        id_t scc = 0;
        lit_t neg = 0;
        EQType type = EQType::none;
    };

    using LitVec = std::vector<lit_t>;
    //! A sum aggregate element.
    using BdSumAggrElem = std::pair<Number, LitVec>;
    //! A vector of sum aggregate elements.
    using BdSumAggrElemVec = std::vector<BdSumAggrElem>;
    //! A vector of sum aggregates.
    using BdSumAggrVec = std::vector<std::tuple<lit_t, BdSumAggrElemVec, NumberSet::interval, NumberSet, CycleType>>;

    using LitMap = std::vector<LitInfo>;
    using CondMap = Util::ordered_map<Output::LitVec, lit_t>;
    using CondLits = std::vector<std::pair<lit_t, CondLitVec>>;

    virtual void do_rule(LitSpan head, LitSpan body, bool choice) = 0;
    virtual void do_bd_aggr(lit_t head, WeightedLitSpan body, int32_t bound) = 0;
    virtual void do_show(Symbol sym, LitSpan body) = 0;

    [[nodiscard]] auto info_(lit_t lit) -> LitInfo &;

    void mark_(lit_t lit, EQType type);

    //! Translate conditions based on how they are used.
    void tr_conds_();

    //! @param sccs strongly connected components of literals
    void tr_cond_lits_();

    static auto analyze_sum_(BdAggrElemSpan elems, GuardSpan guards)
        -> std::tuple<BdSumAggrElemVec, NumberSet::interval, NumberSet, CycleType>;
    //! Translate aggregate literals taking positive cycles into account.
    //!
    //! @param sccs strongly connected components of literals
    void tr_aggr_();

    SymbolStore *store_;
    Output::lit_t lit_ = 0;
    Output::LitVec aux1_;
    Output::LitVec aux2_;
    Util::Graph graph_;
    LitMap lits_;
    CondMap conds_;
    CondLits cond_lits_;
    BdSumAggrVec sum_aggrs_;
};
using UBackend = std::unique_ptr<Backend>;

class Preprocessor : public Backend {};

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
