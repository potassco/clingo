#pragma once

#include <clingo/ground/instantiator.hh>
#include <clingo/ground/term.hh>

#include <clingo/core/symbol.hh>

#include <clingo/util/index_sequence.hh>
#include <clingo/util/ordered_map.hh>
#include <clingo/util/ordered_set.hh>
#include <clingo/util/small_vector.hh>

namespace CppClingo::Ground {

//! @addtogroup ground_base
//! @{

//! A set of variables.
using VariableSet = Util::ordered_set<size_t>;
//! A vector of variables.
using VariableVec = VariableSet::values_container_type;

//! Enumeration to capture the state of an atom.
// NOLINTNEXTLINE(performance-enum-size)
enum class StateAtom : uint64_t {
    //! Indicates that the atom is derived by a fact.
    fact = 0,
    //! Indicates that the atom is derived by some rule but not a fact.
    derived = 1,
    //! Indicates that the atom has not yet been derived by a rule.
    //!
    //! At the time rule (1) is grounded, atom x is not yet defined.
    //! Once (2) has been grounded, there is a definition for it.
    //!
    //!   a :- not x. (1)
    //!   x :- a.     (2)
    //!
    //! The flag indicates atoms that have neither been derived by facts, rules, or externals.
    unknown = 2,
};

//! Capture the state of an atom.
struct AtomInfo {
    uint64_t id : 62;    //!< A unique id among all atoms.
    StateAtom state : 2; //!< The atom state.
};

static_assert(sizeof(AtomInfo) == sizeof(uint64_t));

//! An atom consisting of a symbol and its (mutable) state.
using SymbolicAtom = std::pair<Symbol, AtomInfo>;

//! Enumeration indicating state updates of atoms.
enum class AtomUpdate : uint8_t {
    added = 0,     //!< A freshly added atom.
    changed = 1,   //!< An update atom.
    unchanged = 2, //!< An atom whose state did not change.
};

//! Helper class to manage generation counts.
//!
//! Old, new, and the all generation are considered. The latter is the union of
//! the prior.
class GenerationCounts {
  public:
    //! Update the generation counts.
    //!
    //! Calling with generation zero resets the counts. Calling with a
    //! generation greater than the current generation adds all atoms from the
    //! current all generation to the old generation.
    //!
    //! Calling the function multiple times for the same generation has no
    //! effect.

    //! Get the index of the first atom in the given generation.
    [[nodiscard]] auto begin(MatcherType type) const -> size_t {
        if (type == MatcherType::new_atoms) {
            return old_offset_;
        }
        return 0;
    }

    //! Get the index plus one of the last atom in the given generation.
    [[nodiscard]] auto end(MatcherType type) const -> size_t {
        if (type == MatcherType::old_atoms) {
            return old_offset_;
        }
        return all_offset_;
    }

    //! Check if the given index belongs to the given generation.
    [[nodiscard]] auto contains(size_t index, MatcherType type) const -> bool {
        return begin(type) <= index && index < end(type);
    }

    //! Check if there is an update.
    [[nodiscard]] auto has_update(size_t size) const -> bool { return all_offset_ < size; }

    //! Update the current generations.
    //!
    //! Generations should always increase. They can however be reset by
    //! setting the generation to 0.
    void update(size_t generation, size_t size) {
        // initialize the domain
        // (all atoms are marked as new)
        if (generation == 0) {
            generation_ = 0;
            old_offset_ = 0;
            all_offset_ = size;
        }
        // the generation has been incremented by one
        // (freshly added atoms are marked new)
        else if (generation_ + 1 == generation) {
            generation_ = generation;
            old_offset_ = all_offset_;
            all_offset_ = size;
            // the generation has been incremented by more than one
            // (all atoms are marked old)
        } else if (generation_ + 1 < generation) {
            generation_ = generation;
            old_offset_ = size;
            all_offset_ = size;
        }
    }

  private:
    //! Symbols before this offset are considered old.
    size_t old_offset_ = 0;
    //! Symbols before this offset are considered new or old.
    size_t all_offset_ = 0;
    //! The last generation at which the domain has been updated.
    size_t generation_ = 0;
};

//! A context object.
//!
//! Currently, the interface is empty and one could use a std::any as well.
//! The class exists for easy extensibility.
class BaseContext {
  public:
    //! Destroy the context.
    virtual ~BaseContext() = default;
};

//! The base implementation of an atom base.
//!
//! It implements generation counting and support for adding a context object.
template <class KeyType, class BaseType> class BaseImpl {
  public:
    //! The key identifies an atom and is usually associated with further state.
    using Key = KeyType;

    //! Get the index of the first atom in the given generation.
    [[nodiscard]] auto begin(MatcherType type) const -> size_t { return counts_.begin(type); }

    //! Get the index plus one of the last atom in the given generation.
    [[nodiscard]] auto end(MatcherType type) const -> size_t { return counts_.end(type); }

    //! Check if the base contains the given atom with in the given generation.
    [[nodiscard]] auto contains(Key const &sym, MatcherType type) const -> std::optional<size_t> {
        if (auto idx = base().index(sym); counts_.contains(idx, type)) {
            return idx;
        }
        return std::nullopt;
    }

    //! Update the generation counts.
    void update(size_t generation) { counts_.update(generation, base().size()); }

    //! Ensure that atoms are added to the given generation.
    void ensure(size_t generation) { update(generation > 0 ? generation - 1 : 0); }

    //! Get the context of the base with the desired type.
    //!
    //! Creates one if the base has none yet.
    template <class T> auto context() -> T & {
        if (context_ != nullptr) {
            if (auto res = dynamic_cast<T *>(context_.get()); res != nullptr) {
                return *res;
            }
            throw std::bad_cast();
        }
        context_ = std::make_unique<T>();
        return static_cast<T &>(*context_);
    }

    //! Clear the current context.
    void clear_context() { context_ = nullptr; }

    //! Check if the base has an update.
    [[nodiscard]] auto has_update() const -> bool { return counts_.has_update(base().size()); }

  private:
    friend BaseType;
    BaseImpl() = default;

    [[nodiscard]] auto base() -> BaseType & { return *static_cast<BaseType *>(this); }
    [[nodiscard]] auto base() const -> BaseType const & { return *static_cast<BaseType const *>(this); }

    GenerationCounts counts_;
    std::unique_ptr<BaseContext> context_;
};

//! An atom base used to store derivable atoms and associated state.
//!
//! The base tracks the generation of atoms for semi-naive evaluation,
//! and the state of atoms.
//!
//! An atom base can also store unknown atoms. For such atoms it is not yet
//! know whether there will be a rule deriving them. The only purpose is to
//! store them here is to associated them with a unique id.
class AtomBase : public BaseImpl<Symbol, AtomBase> {
  public:
    //! Map containing the atoms.
    using MapAtom = Util::ordered_map<Symbol, AtomInfo>;

    //! Check if the base is domain.
    //!
    //! A base is domain if it contains facts only.
    [[nodiscard]] auto domain() const {
        for (auto n = derived_.size(); domain_offset_ < n; ++domain_offset_) {
            if (atoms_.nth(derived_[domain_offset_])->second.state != StateAtom::fact) {
                return false;
            }
        }
        return true;
    }
    //! Check if the given atom is a fact.
    //!
    //! This function does not take into account to which generation an atom belongs.
    //! It can also return true for atoms added to upcoming generations.
    auto is_fact(Symbol sym) const -> bool {
        auto it = atoms_.find(sym);
        return it != atoms_.end() && it->second.state == StateAtom::fact;
    }

    //! Check if the base contains the given atom.
    [[nodiscard]] auto find(Symbol const &sym) -> std::optional<MapAtom::iterator> {
        auto it = atoms_.find(sym);
        if (it != atoms_.end() && it->second.state != StateAtom::unknown) {
            return it;
        }
        return std::nullopt;
    }

    //! Add an atom to the base.
    template <class Gen> auto add(Symbol atom, StateAtom state, Gen &&gen) -> std::pair<MapAtom::iterator, AtomUpdate> {
        auto [it, ins] = atoms_.try_emplace(atom, 0, state);
        if (ins) {
            it.value().id = std::invoke(std::forward<Gen>(gen));
            if (state != StateAtom::unknown) {
                derived_.add(atom_index_(it));
            }
            return {it, AtomUpdate::added};
        }
        auto &prev = it.value();
        if (state < prev.state) {
            if (prev.state == StateAtom::unknown) {
                prev.state = state;
                derived_.add(atom_index_(it));
                return {it, AtomUpdate::added};
            }
            prev.state = state;
            return {it, AtomUpdate::changed};
        }
        return {it, AtomUpdate::unchanged};
    }

    //! Get the number of derived atoms.
    [[nodiscard]] auto size() const -> size_t { return derived_.size(); }
    //! Get the number of shown atoms.
    [[nodiscard]] auto num_shown() const -> size_t { return show_offset_; }

    //! Get the atom index of the given symbol.
    //!
    //! Note that only derived atoms have indices.
    auto index(Symbol const &sym) const -> size_t {
        if (auto it = atoms_.find(sym); it != atoms_.end() && it->second.state != StateAtom::unknown) {
            return derived_.find(atom_index_(it));
        }
        return size();
    }
    //! Get the i-th atom in the base.
    auto nth(size_t i) const -> MapAtom::const_iterator { return atoms_.nth(derived_[i]); }
    //! Get the i-th atom in the base.
    auto nth(size_t i) -> MapAtom::iterator { return atoms_.nth(derived_[i]); }

    //! Mark all symbols held by the base.
    void mark(SymbolCollector &gc) {
        clear_context();
        for (auto const &[sym, atom] : atoms_) {
            gc.mark(sym);
        }
    }

    //! Mark derived atoms as shown.
    //!
    //! Returns the index of the first atom not previously shown.
    [[nodiscard]] auto mark_shown() -> size_t { return std::exchange(show_offset_, size()); }

    //! Mark derived atoms as shown.
    //!
    //! Returns the index of the first atom not previously shown.
    [[nodiscard]] auto mark_negated() -> size_t { return std::exchange(negate_offset_, size()); }

    //! Mark derived atoms as projected.
    //!
    //! Returns the index of the first atom not previously projected.
    [[nodiscard]] auto mark_projected() -> size_t { return std::exchange(project_offset_, size()); }

    //! Simplify the atom base.
    //!
    //! Removes false atoms and marks facts based on the predicat.
    //!
    //! @param pred a predicate  to obtain a truth value
    //! @param rem the number of removed atoms
    //! @param fact the number of atoms that became facts
    template <class Pred> void simplify(Pred const &pred, size_t &rem, size_t &fact) {
        // NOTE: the show offset can be zero for auxiliary domains
        assert(show_offset_ == 0 || show_offset_ == size());
        assert(negate_offset_ == 0 || negate_offset_ == size());
        assert(project_offset_ == 0 || project_offset_ == size());
        auto true_atoms = SymbolVec{};
        erase_if(atoms_, [&](SymbolicAtom const &item) {
            if (item.second.state == StateAtom::unknown) {
                ++rem;
                return true;
            }
            auto tv = pred(item.second.id);
            if (tv == TruthValue::bot) {
                ++rem;
                return true;
            }
            if (tv == TruthValue::top && item.second.state != StateAtom::fact) {
                ++fact;
                true_atoms.emplace_back(item.first);
            }
            return false;
        });
        for (auto const &sym : true_atoms) {
            atoms_.find(sym).value().state = StateAtom::fact;
        }
        derived_.assign(0, atoms_.size());
        assert(derived_.size() == atoms_.size());
        domain_offset_ = 0;
        show_offset_ = show_offset_ > 0 ? size() : 0;
        negate_offset_ = negate_offset_ > 0 ? size() : 0;
        project_offset_ = project_offset_ > 0 ? size() : 0;
    }

  private:
    [[nodiscard]] auto atom_index_(MapAtom::const_iterator it) const -> size_t {
        return static_cast<size_t>(std::distance(atoms_.cbegin(), it));
    }

    MapAtom atoms_;
    Util::index_sequence<size_t> derived_;
    size_t mutable domain_offset_ = 0;
    size_t show_offset_ = 0;
    size_t negate_offset_ = 0;
    size_t project_offset_ = 0;
};

//! A unique pointer holding a base.
using UAtomBase = std::unique_ptr<AtomBase>;

//! The state capturing the base of a projection.
class ProjectState {
  public:
    //! Initialize the state.
    ProjectState(String name, size_t vars, AtomBase &base, UTerm p_head, UTerm p_body)
        : name_{name}, base_{&base}, p_head_{std::move(p_head)}, p_body_{std::move(p_body)} {
        ass_.resize(vars);
    }
    //! Get the base of the unprojected literal.
    [[nodiscard]] auto base() const -> AtomBase & { return *base_; }
    //! Get the base of the projected literal.
    [[nodiscard]] auto p_base() -> AtomBase & { return p_base_; }
    //! Get the auxiliary name of projected literal.
    [[nodiscard]] auto name() const -> String const & { return *name_; }
    //! Initialize the projected base.
    //!
    //! This populates the projected base.
    void init(InstantiationContext const &ctx, size_t gen);

  private:
    SharedString name_;
    AtomBase *base_;
    AtomBase p_base_;
    UTerm p_head_;
    UTerm p_body_;
    Assignment ass_;
    size_t imported_ = 0;
};
//! Unique pointer for `ProjectState`.
using UProjectState = std::unique_ptr<ProjectState>;

//! A map from a terms with projections associated state used during grounding.
//!
//! The terms represent a class of similar terms that can reuse the same projection state.
using ProjectMap = Util::ordered_map<Ground::UTerm, UProjectState>;

//! A map from signatures to atom bases.
using BaseMap = Util::ordered_map<std::tuple<String, size_t, bool>, UAtomBase>;

//! The base for all atoms and terms.
class Bases {
  public:
    //! Add an atom base.
    //!
    //! Names starting with a `#` are added as auxiliary bases.
    [[nodiscard]] auto add_base(std::tuple<String, size_t, bool> sig) -> Ground::AtomBase &;

    //! Get a base for the given signature.
    //!
    //! Returns a nullptr if there is no corresponding atom base.
    [[nodiscard]] auto get_base(std::tuple<String, size_t, bool> sig) const -> Ground::AtomBase *;

    //! Add a base for a projected atom.
    [[nodiscard]] auto add_project(SymbolStore &store, Ground::UTerm const &term, Ground::AtomBase &base)
        -> std::pair<Ground::UTerm, ProjectState *>;

    //! Get the atom map.
    [[nodiscard]] auto atoms() const -> BaseMap const & { return atoms_; }
    //! Get the map of projected atoms.
    [[nodiscard]] auto projected() const -> ProjectMap const & { return projected_; }

    //! Clear auxiliary atom bases.
    void clear_aux();

  private:
    BaseMap atoms_;
    BaseMap aux_;
    ProjectMap projected_;
};

//! Class to store state for grounding.
class State {
  public:
    //! Destructor.
    virtual ~State() = default;
    //! Output delayed literals.
    virtual void output(Logger &log, SymbolStore &store, OutputStm &out) = 0;
};
//! A unique pointer to a state.
using UState = std::unique_ptr<State>;
//! A vector of states.
using UStateVec = std::vector<UState>;

//! @}

} // namespace CppClingo::Ground
