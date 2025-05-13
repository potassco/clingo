#include <clingo/core/backend.hh>
#include <clingo/core/output.hh>

#include <clingo/util/checked_math.hh>
#include <clingo/util/enum.hh>
#include <clingo/util/graph.hh>
#include <clingo/util/interval_set.hh>
#include <clingo/util/ordered_map.hh>
#include <clingo/util/unordered_map.hh>

namespace CppClingo::Output {

//! @addtogroup output
//! @{

//! Class similar to Potassco::TheoryData but with automatic id generation.
class TheoryData {
  public:
    //! A vector of ids.
    using IdVec = Util::small_vector<prg_id_t, 4>;
    //! A vector of literals.
    using LitVec = Util::small_vector<prg_lit_t, 4>;

    //! Construct a theory data object.
    //!
    //! @param store the underlying symbol store
    //! @param backend the underlying backend
    TheoryData(SymbolStore &store, UTheoryBackend backend) : store_{&store}, backend_{std::move(backend)} {}

    //! Add a number term.
    //!
    //! @param num the number
    //! @return the term id
    auto num(prg_weight_t num) -> prg_id_t;

    //! Add a string term.
    //!
    //! @param str the string
    //! @return the term id
    auto str(String str) -> prg_id_t;

    //! Add a function term.
    //!
    //! The given name must refer to a string.
    //!
    //! @param name the name of the function
    //! @param args the arguments of the function
    //! @return the term id
    auto fun(prg_id_t name, IdVec args) -> prg_id_t;
    //! Overload for strings.
    //!
    //! @param name the name of the function
    //! @param args the arguments of the function
    //! @return the term id
    auto fun(String name, IdVec args) -> prg_id_t { return fun(str(name), std::move(args)); }

    //! Overload for strings and spans.
    auto fun(String name, PrgIdSpan args) -> prg_id_t { return fun(str(name), IdVec{args.begin(), args.end()}); }

    //! Add a tuple term.
    //!
    //! @param type the type of the tuple
    //! @param args the arguments of the tuple
    //! @return the term id
    auto tup(TheoryTermTupleType type, IdVec args) -> prg_id_t;

    //! Overload for spans.
    auto tup(TheoryTermTupleType type, PrgIdSpan args) -> prg_id_t {
        return tup(type, IdVec{args.begin(), args.end()});
    }

    //! Convert a symbol into a theory term.
    //!
    //! @param sym the symbol to convert
    //! @return the term id
    auto sym(Symbol sym) -> prg_id_t;

    //! Add a theory element.
    //!
    //! @param tuple the ids of terms forming the tuple
    //! @param cond the condition
    //! @return the element id
    auto elem(IdVec tuple, LitVec cond) -> prg_id_t;

    //! Overload for spans.
    auto elem(PrgIdSpan tuple, PrgLitSpan cond) -> prg_id_t {
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
    auto atom(std::function<prg_lit_t()> const &atom, prg_id_t name, IdVec elems,
              std::optional<std::pair<prg_id_t, prg_id_t>> guard) -> prg_lit_t;

    //! Overload for strings.
    auto atom(std::function<prg_lit_t()> const &atom, Symbol name, IdVec elems,
              std::optional<std::pair<String, prg_id_t>> guard) -> prg_lit_t;

    //! Overload for strings and spans.
    auto atom(std::function<prg_lit_t()> const &atom, Symbol name, PrgIdSpan elems,
              std::optional<std::pair<String, prg_id_t>> guard) -> prg_lit_t {
        return this->atom(atom, name, IdVec{elems.begin(), elems.end()}, guard);
    }

    //! Clear the theory data.
    void reset() noexcept;

  private:
    using StringMap = Util::unordered_map<SharedString, prg_id_t>;
    using NumMap = Util::unordered_map<prg_weight_t, prg_id_t>;
    using FunMap = Util::unordered_map<std::pair<prg_id_t, IdVec>, prg_id_t>;
    using TupMap = Util::unordered_map<std::pair<TheoryTermTupleType, IdVec>, prg_id_t>;
    using ElemMap = Util::unordered_map<std::pair<IdVec, LitVec>, prg_id_t>;
    using AtomMap =
        Util::unordered_map<std::tuple<prg_id_t, IdVec, std::optional<std::pair<prg_id_t, prg_id_t>>>, prg_lit_t>;

    //! Helper to insert elements into the term maps.
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
    prg_id_t ids_ = 0;
};

//! Create an output that forwards ground statements to a backend.
//!
//! Backends accept a simpler format as provided by the grounder. This output
//! brings the statements into the required form and passes them to the
//! backend.
//!
//! @param store the store holding symbols
//! @param backend the target backend
//! @param theory the target backend
//! @return the output
auto make_backend_output(SymbolStore &store, ProgramBackend &backend, TheoryData &theory) -> UOutputStm;

//! @}

} // namespace CppClingo::Output
