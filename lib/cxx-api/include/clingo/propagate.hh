#pragma once

#include <clingo/base.hh>
#include <clingo/core.hh>

#include <clingo/propagate.h>

namespace Clingo {

//! @addtogroup cpp_propagate
//! Extend the search with propagators for arbitrary theories.
//!
//! @{

//! Enumeration of propagator check modes.
enum class PropagatorCheckMode : clingo_propagator_check_mode_t {
    none = clingo_propagator_check_mode_none,         //!< Do not call check at all.
    total = clingo_propagator_check_mode_total,       //!< Call check on total assignments.
    fixpoint = clingo_propagator_check_mode_fixpoint, //!< Call check on propagation fixpoints.
    both = clingo_propagator_check_mode_both,         //!< Call check on propagation fixpoints and total assignments.
};

//! Enumeration of propagator undo modes.
enum class PropagatorUndoMode : clingo_propagator_undo_mode_t {
    default_ = clingo_propagator_undo_mode_default, //!< Call undo for non-empty change lists.
    always = clingo_propagator_undo_mode_always,    //!< Also call undo for empty change lists.
};

//! Enumeration of weight constraint types.
enum class WeightConstraintType : clingo_weight_constraint_type_t {
    implication_left = clingo_weight_constraint_type_implication_left,   //!< The weight constraint implies the literal.
    implication_right = clingo_weight_constraint_type_implication_right, //!< The literal implies the weight constraint.
    equivalence = clingo_weight_constraint_type_equivalence, //!< The weight constraint is equivalent to the literal.
};

//! Enumeration of clause flags.
enum class ClauseFlags : clingo_clause_type_t {
    none = clingo_clause_type_learnt,  //!< The empty set of flags.
    lock = clingo_clause_type_static,  //!< Exempt the clause from deletion.
    tag = clingo_clause_type_volatile, //!< Delete the clause at the end of the current solving step.
};
//! Enable bitset enumeration for ClauseFlags.
CLINGO_ENABLE_BITSET_ENUM(ClauseFlags);

//! A trail is a sequence of solver literals.
//!
//! Literals in the trail are ordered by decision levels, where the first
//! literal with a larger level than the previous literals is a decision; the
//! following literals with the same level are implied by this decision literal.
//! Each decision level up to and including the current decision level has a
//! valid offset in the trail.
class Trail {
  public:
    //! The value type is a solver literal.
    using value_type = SolverLiteral;
    //! The size type.
    using size_type = std::size_t;
    //! The difference type.
    using difference_type = std::ptrdiff_t;
    //! The reference type, which simply refers to the value type.
    using reference = value_type;
    //! The pointer type, which is a proxy to the value type.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type, which is a random access iterator over the trail.
    using iterator = Detail::RandomAccessIterator<Trail>;

    //! Construct a trail from the underlying C representation.
    //!
    //! For internal use.
    //!
    //! @param assignment the C representation of the assignment object
    explicit Trail(clingo_assignment_t const *assignment) : assignment_{assignment} {}

    //! Get the literal at the given index in the trail.
    //!
    //! @param index the index of the literal
    //!
    //! @return the literal at the index
    [[nodiscard]] auto at(size_type index) const -> value_type {
        return Detail::call<clingo_assignment_trail_at>(assignment_, index);
    }

    //! @copydoc Trail::at()
    [[nodiscard]] auto operator[](size_type index) const -> value_type { return at(index); }

    //! Get the size of the trail.
    //!
    //! @return the size of the trail
    [[nodiscard]] auto size() const -> size_type { return Detail::call<clingo_assignment_trail_size>(assignment_); }

    //! Get an iterator to the beginning of the trail.
    //!
    //! @return an iterator to the beginning of the trail
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }

    //! Get an iterator to the end of the trail.
    //!
    //! @return an iterator to the end of the trail
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

    //! Get an iterator to the beginning of the given decision level.
    //!
    //! @param level the decision level to get the literals for
    //! @return the iterator pointing to the decision literal of the given level
    [[nodiscard]] auto begin(ProgramId level) const -> iterator {
        return iterator{*this, Detail::call<clingo_assignment_trail_begin>(assignment_, level)};
    }

    //! Get an iterator to the end of the given decision level.
    //!
    //! @param level the decision level to get the literals for
    //! @return the iterator pointing to end of the literals of the given level
    [[nodiscard]] auto end(ProgramId level) const -> iterator {
        return iterator{*this, Detail::call<clingo_assignment_trail_end>(assignment_, level)};
    }

    //! Get the subrange of the literals of the given decision level.
    //!
    //! @param level the decision level to get the literals for
    //! @return the subrange of the literals of the given level
    [[nodiscard]] auto level(ProgramId level) const { return std::ranges::subrange{begin(level), end(level)}; }

  private:
    clingo_assignment_t const *assignment_;
};

//! Represents a (partial) assignment of a particular solver.
//!
//! An assignment assigns truth values to a set of literals. A literal is
//! assigned to either @link Assignment::value() true or false, or is
//! unassigned@endlink. Furthermore, each assigned literal is associated with a
//! @link Assignment::level() decision level@endlink. There is exactly one
//! @link Assignment::decision() decision literal@endlink for each decision
//! level greater than zero. Assignments to all other literals on the same
//! level are consequences implied by the current and possibly previous
//! decisions. Assignments on level zero are immediate consequences of the
//! current program. Decision levels are consecutive numbers starting with zero
//! up to and including the @link Assignment::decision_level() current decision
//! level@endlink.
class Assignment {
  public:
    //! The value type, which is a solver literal.
    using value_type = SolverLiteral;
    //! The size type.
    using size_type = std::size_t;
    //! The difference type.
    using difference_type = std::ptrdiff_t;
    //! The reference type, which simply refers to the value type.
    using reference = value_type;
    //! The pointer type, which is a proxy to the value type.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type, which is a random access iterator over the assignment.
    using iterator = Detail::RandomAccessIterator<Assignment>;

    //! Construct an assignment from the underlying C representation.
    explicit Assignment(clingo_assignment_t const *assignment) : assignment_(assignment) {}
    //! Get the id of the thread to which the assignment belongs.
    //! @return the id of the solving thread owning this assignment
    [[nodiscard]] auto thread_id() const -> ProgramId { return Detail::call<clingo_assignment_thread_id>(assignment_); }

    //! Get the size of the assignment.
    //!
    //! This is the number of all positive problem literals including
    //! unassigned literals.
    //!
    //! @return the size of the assignment
    [[nodiscard]] auto size() const -> size_type { return Detail::call<clingo_assignment_size>(assignment_); }

    //! Get the literal at the given index in the assignment.
    //!
    //! @param size the index of the literal
    //! @return the literal at the index
    [[nodiscard]] auto at(size_type size) const -> value_type {
        return Detail::call<clingo_assignment_at>(assignment_, size);
    }

    //! @copydoc Assignment::at()
    [[nodiscard]] auto operator[](size_type size) const -> value_type { return at(size); }

    //! Get the decision literal for the given decision level.
    //!
    //! @param level the decision level to get the decision literal for
    //! @return the decision literal for the given level
    [[nodiscard]] auto decision(ProgramId level) const -> SolverLiteral {
        return Detail::call<clingo_assignment_decision>(assignment_, level);
    }

    //! Get the current decision level.
    //!
    //! @return the current decision level
    [[nodiscard]] auto decision_level() const -> ProgramId {
        return Detail::call<clingo_assignment_decision_level>(assignment_);
    }

    //! Check if the assignment is conflicting.
    //!
    //! @return whether the assignment is conflicting
    [[nodiscard]] auto has_conflict() const -> bool {
        return Detail::call<clingo_assignment_has_conflict>(assignment_);
    }

    //! Check whether the assignment contains the given literal.
    //!
    //! @return whether the assignment contains the literal
    [[nodiscard]] auto contains(SolverLiteral lit) const -> bool {
        return Detail::call<clingo_assignment_has_literal>(assignment_, lit);
    }

    //! Check if the given literal is false.
    //!
    //! @param lit the literal to check
    //! @return whether the literal is false
    [[nodiscard]] auto is_false(SolverLiteral lit) const -> bool {
        return Detail::call<clingo_assignment_is_false>(assignment_, lit);
    }

    //! Check if the truth value of the given literal is fixed.
    //!
    //! @param lit the literal to check
    //! @return whether the literal has a fixed truth value
    [[nodiscard]] auto is_fixed(SolverLiteral lit) const -> bool {
        return Detail::call<clingo_assignment_is_fixed>(assignment_, lit);
    }

    //! Check if the truth value of the given literal is free.
    //!
    //! @param lit the literal to check
    //! @return whether the literal is free
    [[nodiscard]] auto is_free(SolverLiteral lit) const -> bool {
        return Detail::call<clingo_assignment_truth_value>(assignment_, lit) == clingo_truth_value_free;
    }

    //! Check if the given literal is true.
    //!
    //! @param lit the literal to check
    //! @return whether the literal is true
    [[nodiscard]] auto is_true(SolverLiteral lit) const -> bool {
        return Detail::call<clingo_assignment_is_true>(assignment_, lit);
    }

    //! Check whether the assignment is total.
    //!
    //! An assignment is total if all literals are assigned a truth value,
    //!
    //! @return whether the assignment is total
    [[nodiscard]] auto is_total() const -> bool { return Detail::call<clingo_assignment_is_total>(assignment_); }

    //! Get the level on which the given literal was assigned.
    //!
    //! @param lit the literal to get the level for
    //! @return the level on which the literal was assigned
    [[nodiscard]] auto level(SolverLiteral lit) const -> ProgramId {
        return Detail::call<clingo_assignment_level>(assignment_, lit);
    }

    //! Get the truth value of the given literal.
    //!
    //! The optional returned is empty if the literal is unassigned and,
    //! otherwise, contains the truth value of the literal.
    //!
    //! @param lit the literal to get the truth value for
    //! @return the truth value of the literal
    [[nodiscard]] auto value(SolverLiteral lit) const -> std::optional<bool> {
        clingo_truth_value_t res = Detail::call<clingo_assignment_truth_value>(assignment_, lit);
        switch (res) {
            case clingo_truth_value_true: {
                return true;
            }
            case clingo_truth_value_false: {
                return false;
            }
            default: {
                return std::nullopt;
            }
        }
    }

    //! Get the root level of the assignment.
    //!
    //! Decisions levels smaller or equal to the root level are not backtracked
    //! during solving.
    //!
    //! @return the root level of the assignment
    [[nodiscard]] auto root_level() const -> ProgramId {
        return Detail::call<clingo_assignment_root_level>(assignment_);
    }

    //! Get the trail of the assignment.
    //!
    //! @return the trail of the assignment
    [[nodiscard]] auto trail() const -> Trail { return Trail{assignment_}; }

    //! Get an iterator to the beginning of the assignment.
    //!
    //! @return an iterator to the beginning of the assignment
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }

    //! Get an iterator to the end of the assignment.
    //!
    //! @return an iterator to the end of the assignment
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    clingo_assignment_t const *assignment_;
};

//! Class to control a user-defined propagator.
//!
//! This class provides methods for adding clauses, literals, and nogoods, as
//! well as managing watches and performing propagation.
class PropagateControl {
  public:
    //! Constructor from the underlying C representation.
    //!
    //! @param ctl the C representation of the control object
    explicit PropagateControl(clingo_propagate_control_t *ctl) : ctl_{ctl} {}
    //! Constructor from the underlying C representation.
    //!
    //! @param init the C representation of the init object
    explicit PropagateControl(clingo_propagate_init_t *init)
        : PropagateControl(Detail::call<clingo_propagate_init_control>(init)) {}

    //! Adds a literal to the problem or the active solver thread.
    //!
    //! @note If the function is called in the context of a particular solver thread, the literal is solver local:
    //! solver local literals are not shared between solvers and are removed at the end of the current solving step.
    //! @param freeze whether to freeze the literal
    //! @return the added literal
    [[nodiscard]] auto add_literal(bool freeze = true) const -> SolverLiteral {
        return Detail::call<clingo_propagate_control_add_literal>(ctl_, freeze);
    }

    //! Add a clause to the solver.
    //!
    //! If adding the clause results in a conflict or requires backtracking, the function returns false
    //! and the calling propagator must not add further clauses from the current callback.
    //!
    //! @param literals the literals of the clause to add
    //! @param flags the flags for the clause
    //! @return whether the clause was added without conflict
    [[nodiscard]] auto add_clause(SolverLiteralSpan literals, ClauseFlags flags = ClauseFlags::none) const -> bool {
        return Detail::call<clingo_propagate_control_add_clause>(ctl_, literals.data(), literals.size(),
                                                                 static_cast<clingo_clause_type_t>(flags));
    }

    //! Add a clause to the solver.
    //!
    //! If adding the clause results in a conflict or requires backtracking, the function returns false
    //! and the calling propagator must not add further clauses from the current callback.
    //!
    //! @param literals the literals of the clause to add
    //! @param flags the flags for the clause
    //! @return whether the clause was added without conflict
    [[nodiscard]] auto add_clause(SolverLiteralList literals, ClauseFlags flags = ClauseFlags::none) const -> bool {
        return add_clause(SolverLiteralSpan{literals}, flags);
    }

    //! Add a weight constraint to the solver.
    //!
    //! @param literal the literal associated with the constraint
    //! @param literals the weighted literals of the constraint
    //! @param bound the bound of the constraint
    //! @param type the type of the weight constraint
    //! @return whether the constraint was added without a conflict
    [[nodiscard]] auto add_weight_constraint(SolverLiteral literal, WeightedLiteralSpan literals, Weight bound,
                                             WeightConstraintType type) const -> bool {
        return Detail::call<clingo_propagate_control_add_weight_constraint>(
            ctl_, literal, literals.data(), literals.size(), bound, static_cast<clingo_weight_constraint_type_t>(type));
    }

    //! Add a nogood to the solver.
    //!
    //! This is the same as calling add_clause() with the negated literals.
    //!
    //! @param literals the literals of the nogood to add
    //! @param flags the flags for the nogood
    //! @return whether the nogood was added without conflict
    [[nodiscard]] auto add_nogood(SolverLiteralSpan literals, ClauseFlags flags = ClauseFlags::none) const -> bool {
        return add_clause(Detail::transform(literals, [](auto const &lit) { return -lit; }), flags);
    }

    //! Add a watch for the given solver literal.
    //!
    //! @note If called in the context of initialization, the watch is added to all current and future solving threads.
    //! Otherwise, the watch is only added to the active solving thread.
    //!
    //! @param literal the literal to watch
    void add_watch(SolverLiteral literal) const {
        Detail::handle_error(clingo_propagate_control_add_watch(ctl_, literal));
    }

    //! Check whether the given solver literal is watched in the current context.
    //!
    //! @param literal the literal to check
    //! @return whether the literal is watched
    [[nodiscard]] auto has_watch(SolverLiteral literal) const -> bool {
        return Detail::call<clingo_propagate_control_has_watch>(ctl_, literal);
    }

    //! Remove a watch for the given solver literal.
    //!
    //! @param lit the literal to remove the watch for
    void remove_watch(SolverLiteral lit) const {
        Detail::handle_error(clingo_propagate_control_remove_watch(ctl_, lit));
    }

    //! Propagate consequences of previously added clauses.
    //!
    //! If a conflict is detected during propagation, the function returns
    //! false and the calling propagator must not add further clauses from the
    //! current callback.
    //!
    //! @return whether propagation succeeded without conflict
    [[nodiscard]] auto propagate() const -> bool { return Detail::call<clingo_propagate_control_propagate>(ctl_); }

  private:
    clingo_propagate_control_t *ctl_;
};

//! Object to initialize a user-defined propagator before each solving step.
//!
//! Each @link cpp_base symbolic or theory atom@endlink is uniquely associated
//! with an aspif atom in form of a positive integer (@ref
//! Clingo::ProgramLiteral). Aspif literals additionally are signed to
//! represent default negation. Furthermore, there are non-zero integer solver
//! literals (@ref ::SolverLiteral). There is a surjective mapping from program
//! atoms to solver literals.
//!
//! All methods called during propagation use solver literals, whereas
//! Clingo::Atom::literal() and Clingo::TheoryAtom::literal() return program
//! literals. The function Clingo::PropagateInit::solver_literal() can be used to map
//! program literals or @link Clingo::TheoryElement::condition_id() condition
//! ids@endlink to solver literals.
class PropagateInit : public PropagateControl {
  public:
    //! Constructor from the underlying C representation.
    //!
    //! @param init the C representation of the initialization object
    explicit PropagateInit(clingo_propagate_init_t *init) : PropagateControl(init), init_{init} {}

    //! Get the library object associated with the propagator.
    //!
    //! @return the library object associated with the propagator
    [[nodiscard]] auto library() const -> Library {
        return Library{Detail::call<clingo_propagate_init_library>(init_), true};
    }

    //! Get the base object associated with the propagator.
    //!
    //! @return the base object associated with the propagator
    [[nodiscard]] auto base() const -> Base { return Base{Detail::call<clingo_propagate_init_base>(init_)}; }

    //! Get the check mode of the propagator.
    //!
    //! @return the check mode of the propagator
    [[nodiscard]] auto check_mode() const -> PropagatorCheckMode {
        return static_cast<PropagatorCheckMode>(Detail::call<clingo_propagate_init_get_check_mode>(init_));
    }

    //! Set the check mode of the propagator.
    //!
    //! @param mode the new check mode of the propagator
    void check_mode(PropagatorCheckMode mode) {
        Detail::handle_error(
            clingo_propagate_init_set_check_mode(init_, static_cast<clingo_propagator_check_mode_t>(mode)));
    }

    //! Get the number of threads used in subsequent solving.
    //!
    //! @return the number of threads used in subsequent solving
    [[nodiscard]] auto number_of_threads() const -> ProgramId {
        return Detail::call<clingo_propagate_init_number_of_threads>(init_);
    }

    //! Get the undo mode of the propagator.
    //!
    //! @return the undo mode of the propagator
    [[nodiscard]] auto undo_mode() const -> PropagatorUndoMode {
        return static_cast<PropagatorUndoMode>(Detail::call<clingo_propagate_init_get_undo_mode>(init_));
    }

    //! Set the undo mode of the propagator.
    //!
    //! @param mode the new undo mode of the propagator
    void undo_mode(PropagatorUndoMode mode) const {
        Detail::handle_error(
            clingo_propagate_init_set_undo_mode(init_, static_cast<clingo_propagator_undo_mode_t>(mode)));
    }

    //! Map a program literal to a solver literal.
    //!
    //! @param literal the program literal to map
    //! @return the corresponding solver literal
    [[nodiscard]] auto solver_literal(ProgramLiteral literal) const -> SolverLiteral {
        return Detail::call<clingo_propagate_init_solver_literal>(init_, literal);
    }

    //! Add a weighted literal to minimize to the solver.
    //!
    //! @param literal the literal to minimize
    //! @param weight the weight of the literal
    //! @param priority the priority of the literal
    void add_minimize(SolverLiteral literal, Weight weight, Weight priority) const {
        Detail::handle_error(clingo_propagate_init_add_minimize(init_, literal, weight, priority));
    }

    //! Freeze the given literal in the solver.
    //!
    //! Frozen literals are exempt from elimination during preprocessing.
    //!
    //! @param literal the literal to freeze
    void freeze_literal(SolverLiteral literal) const {
        Detail::handle_error(clingo_propagate_init_freeze_literal(init_, literal));
    }

  private:
    clingo_propagate_init_t *init_;
};

//! Interface for user-defined propagators.
class Propagator {
  public:
    //! The default constructor.
    Propagator() = default;
    //! Disable copy and move operations.
    Propagator(Propagator &&other) = delete;
    //! Disable copy and move operations.
    auto operator=(Propagator &&other) -> Propagator & = delete;
    //! The destructor.
    virtual ~Propagator() = default;

    //! Callback to initialize the propagator before each solving step.
    //!
    //! @param assignment the current assignment of the solver
    //! @param init the initialization object
    void init(Assignment assignment, PropagateInit init) { do_init(assignment, init); }
    //! Callback to initialize/setup thread-specific data for the propagator.
    //!
    //! This function is called once for each solving thread before solving of the current step is started.
    //!
    //! @param assignment the current assignment of the solver
    //! @param ctl the propagate control object
    void attach(Assignment assignment, PropagateControl ctl) { do_attach(assignment, ctl); }

    //! Callback to propagate consequences of the given literals.
    //!
    //! The function is only called for watched literals assigned on
    //! the current decision level.
    //!
    //! A propagator can add clauses here to propagate consequences of the
    //! underlying theory.
    //!
    //! @param assignment the current assignment of the solver
    //! @param ctl the propagate control object
    //! @param changes the literals that were assigned on the current decision level
    void propagate(Assignment assignment, PropagateControl ctl, ProgramLiteralSpan changes) {
        do_propagate(assignment, ctl, changes);
    }

    //! Called when the solver backtracks to a previous decision level.
    //!
    //! The function is called with literals from all previous propagate calls
    //! on the same decision level.
    //!
    //! Clingo::PropagateInit::undo_mode() can be used to control when this
    //! function is called.
    //!
    //! @param assignment the current assignment of the solver
    //! @param changes the literals that were assigned on the current decision level
    void undo(Assignment assignment, ProgramLiteralSpan changes) { do_undo(assignment, changes); }

    //! Callback that is called on propagation fixpoints or total assignments.
    //!
    //! Clingo::PropagateInit::check_mode() can be used to control when this
    //! callback is called.
    //!
    //! Like in Clingo::Propagator::Propagate(), a propagator can add clauses
    //! here to propagate consequences or discard the current assignment.
    //!
    //! @param assignment the current assignment of the solver
    //! @param ctl the propagate control object
    void check(Assignment assignment, PropagateControl ctl) { do_check(assignment, ctl); }

  private:
    virtual void do_init([[maybe_unused]] Assignment assignment, [[maybe_unused]] PropagateInit init) {}
    virtual void do_attach([[maybe_unused]] Assignment assignment, [[maybe_unused]] PropagateControl ctl) {}
    virtual void do_propagate([[maybe_unused]] Assignment assignment, [[maybe_unused]] PropagateControl ctl,
                              [[maybe_unused]] ProgramLiteralSpan changes) {}
    virtual void do_undo([[maybe_unused]] Assignment assignment, [[maybe_unused]] ProgramLiteralSpan changes) {}
    virtual void do_check([[maybe_unused]] Assignment assignment, [[maybe_unused]] PropagateControl ctl) {}
};

//! Interface for user-defined propagators with a decision heuristic.
class Heuristic : public Propagator {
  public:
    //! The default constructor.
    Heuristic() = default;

    //! Callback to decide on the next literal to assign.
    //!
    //! The fallback literal indicates the literal the solver would have
    //! assigned. This choice can be overridden by returning a different
    //! literal.
    //!
    //! @param assignment the current assignment of the solver
    //! @param literal the fallback literal
    auto decide(Assignment assignment, SolverLiteral literal) -> SolverLiteral {
        return do_decide(assignment, literal);
    }

  private:
    virtual auto do_decide([[maybe_unused]] Assignment assignment, clingo_literal_t literal) -> clingo_literal_t {
        return literal;
    }
};

//! @}

} // namespace Clingo
