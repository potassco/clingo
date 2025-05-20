#pragma once

#include <clingo/base.hh>
#include <clingo/core.hh>

#include <clingo/propagate.h>

namespace Clingo {

enum class PropgatorCheckMode : clingo_propagator_check_mode_t {
    none = clingo_propagator_check_mode_none,         //!< do not call check at all
    total = clingo_propagator_check_mode_total,       //!< call check on total assignments
    fixpoint = clingo_propagator_check_mode_fixpoint, //!< call check on propagation fixpoints
    both = clingo_propagator_check_mode_both,         //!< call check on propagation fixpoints and total assignments
};

enum class PropagatorUndoMode : clingo_propagator_undo_mode_t {
    default_ = clingo_propagator_undo_mode_default, //!< call undo for non-empty change lists
    always = clingo_propagator_undo_mode_always,    //!< also call check when check has been called
};

//! Enumeration of weight_constraint_types.
enum WeightConstraintType : clingo_weight_constraint_type_t {
    implication_left = clingo_weight_constraint_type_implication_left,   //!< the weight constraint implies the literal
    implication_right = clingo_weight_constraint_type_implication_right, //!< the literal implies the weight constraint
    equivalence = clingo_weight_constraint_type_equivalence, //!< the weight constraint is equivalent to the literal
};

enum ClauseFlags : clingo_clause_type_t {
    none = clingo_clause_type_learnt,  //!< the empty set of flags
    lock = clingo_clause_type_static,  //!< exempt the clause from deletion
    tag = clingo_clause_type_volatile, //!< delete the clause at the end of the current solving step
};
CLINGO_ENABLE_BITSET_ENUM(ClauseFlags);

class Trail {
  public:
    using value_type = SolverLiteral;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<Trail>;

    explicit Trail(clingo_assignment_t const *assignment) : assignment_{assignment} {}

    [[nodiscard]] auto at(size_type index) const -> value_type {
        return Detail::call<clingo_assignment_trail_at>(assignment_, index);
    }

    [[nodiscard]] auto operator[](size_type index) const -> value_type { return at(index); }

    [[nodiscard]] auto size() const -> size_type { return Detail::call<clingo_assignment_trail_size>(assignment_); }

    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }

    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

    [[nodiscard]] auto begin(ProgramId level) const -> iterator {
        return iterator{*this, Detail::call<clingo_assignment_trail_begin>(assignment_, level)};
    }

    [[nodiscard]] auto end(ProgramId level) const -> iterator {
        return iterator{*this, Detail::call<clingo_assignment_trail_end>(assignment_, level)};
    }

    [[nodiscard]] auto level(ProgramId level) const { return std::ranges::subrange{begin(level), end(level)}; }

  private:
    clingo_assignment_t const *assignment_;
};

class Assignment {
  public:
    using value_type = SolverLiteral;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<Assignment>;

    explicit Assignment(clingo_assignment_t const *assignment) : assignment_(assignment) {}

    [[nodiscard]] auto size() const -> size_type { return Detail::call<clingo_assignment_size>(assignment_); }

    [[nodiscard]] auto operator[](size_type size) const -> value_type {
        return Detail::call<clingo_assignment_at>(assignment_, size);
    }

    [[nodiscard]] auto decision(ProgramId level) const -> SolverLiteral {
        return Detail::call<clingo_assignment_decision>(assignment_, level);
    }

    [[nodiscard]] auto decision_level() const -> ProgramId {
        return Detail::call<clingo_assignment_decision_level>(assignment_);
    }

    [[nodiscard]] auto has_conflict() const -> bool {
        return Detail::call<clingo_assignment_has_conflict>(assignment_);
    }

    [[nodiscard]] auto contains(SolverLiteral lit) const -> bool {
        return Detail::call<clingo_assignment_has_literal>(assignment_, lit);
    }

    [[nodiscard]] auto is_false(SolverLiteral lit) const -> bool {
        return Detail::call<clingo_assignment_is_false>(assignment_, lit);
    }

    [[nodiscard]] auto is_fixed(SolverLiteral lit) const -> bool {
        return Detail::call<clingo_assignment_is_fixed>(assignment_, lit);
    }

    [[nodiscard]] auto is_free(SolverLiteral lit) const -> bool {
        return Detail::call<clingo_assignment_truth_value>(assignment_, lit) == clingo_truth_value_free;
    }

    [[nodiscard]] auto is_total() const -> bool { return Detail::call<clingo_assignment_is_total>(assignment_); }

    [[nodiscard]] auto is_true(SolverLiteral lit) const -> bool {
        return Detail::call<clingo_assignment_is_true>(assignment_, lit);
    }

    [[nodiscard]] auto level(SolverLiteral lit) const -> ProgramId {
        return Detail::call<clingo_assignment_level>(assignment_, lit);
    }

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

    [[nodiscard]] auto root_level() const -> ProgramId {
        return Detail::call<clingo_assignment_root_level>(assignment_);
    }

    [[nodiscard]] auto trail() const -> Trail { return Trail{assignment_}; }

    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }

    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    clingo_assignment_t const *assignment_;
};

class PropagateInit {
  public:
    explicit PropagateInit(clingo_propagate_init_t *init) : init_{init} {}

    [[nodiscard]] auto assignment() const -> Assignment {
        return Assignment{Detail::call<clingo_propagate_init_assignment>(init_)};
    }

    [[nodiscard]] auto library() const -> Library {
        return Library{Detail::call<clingo_propagate_init_library>(init_), true};
    }

    [[nodiscard]] auto base() const -> Base { return Base{Detail::call<clingo_propagate_init_base>(init_)}; }

    [[nodiscard]] auto check_mode() const -> PropgatorCheckMode {
        return static_cast<PropgatorCheckMode>(Detail::call<clingo_propagate_init_get_check_mode>(init_));
    }

    void check_mode(PropgatorCheckMode mode) {
        Detail::handle_error(
            clingo_propagate_init_set_check_mode(init_, static_cast<clingo_propagator_check_mode_t>(mode)));
    }

    [[nodiscard]] auto number_of_threads() const -> ProgramId {
        return Detail::call<clingo_propagate_init_number_of_threads>(init_);
    }

    [[nodiscard]] auto undo_mode() const -> PropagatorUndoMode {
        return static_cast<PropagatorUndoMode>(Detail::call<clingo_propagate_init_get_undo_mode>(init_));
    }

    void undo_mode(PropagatorUndoMode mode) const {
        Detail::handle_error(
            clingo_propagate_init_set_undo_mode(init_, static_cast<clingo_propagator_undo_mode_t>(mode)));
    }

    [[nodiscard]] auto add_clause(SolverLiteralSpan literals) const -> bool {
        return Detail::call<clingo_propagate_init_add_clause>(init_, literals.data(), literals.size());
    }

    [[nodiscard]] auto add_literal(bool freeze) const -> SolverLiteral {
        return Detail::call<clingo_propagate_init_add_literal>(init_, freeze);
    }

    void add_minimize(SolverLiteral literal, Weight weight, Weight priority) const {
        Detail::handle_error(clingo_propagate_init_add_minimize(init_, literal, weight, priority));
    }

    void add_watch(SolverLiteral literal, std::optional<ProgramId> thread_id = std::nullopt) const {
        if (thread_id) {
            Detail::handle_error(clingo_propagate_init_add_watch_to_thread(init_, literal, *thread_id));
        } else {
            Detail::handle_error(clingo_propagate_init_add_watch(init_, literal));
        }
    }

    [[nodiscard]] auto add_weight_constraint(SolverLiteral literal, WeightedLiteralSpan literals, Weight bound,
                                             WeightConstraintType type, bool compare_equal) const -> bool {
        return Detail::call<clingo_propagate_init_add_weight_constraint>(init_, literal, literals.data(),
                                                                         literals.size(), bound, type, compare_equal);
    }

    void freeze_literal(SolverLiteral literal) const {
        Detail::handle_error(clingo_propagate_init_freeze_literal(init_, literal));
    }

    [[nodiscard]] auto propagate() const -> bool { return Detail::call<clingo_propagate_init_propagate>(init_); }

    void remove_watch(SolverLiteral literal, std::optional<ProgramId> thread_id) const {
        if (thread_id) {
            Detail::handle_error(clingo_propagate_init_remove_watch_from_thread(init_, literal, *thread_id));
        } else {
            Detail::handle_error(clingo_propagate_init_remove_watch(init_, literal));
        }
    }

    [[nodiscard]] auto solver_literal(ProgramLiteral literal) const -> SolverLiteral {
        return Detail::call<clingo_propagate_init_solver_literal>(init_, literal);
    }

  private:
    clingo_propagate_init_t *init_;
};

class PropagateControl {
  public:
    explicit PropagateControl(clingo_propagate_control_t *ctl) : ctl_{ctl} {}

    [[nodiscard]] auto add_literal() const -> SolverLiteral {
        return Detail::call<clingo_propagate_control_add_literal>(ctl_);
    }

    [[nodiscard]] auto add_clause(SolverLiteralSpan literals, ClauseFlags flags = ClauseFlags::none) const -> bool {
        return Detail::call<clingo_propagate_control_add_clause>(ctl_, literals.data(), literals.size(),
                                                                 static_cast<clingo_clause_type_t>(flags));
    }

    [[nodiscard]] auto add_nogood(SolverLiteralSpan literals, ClauseFlags flags = ClauseFlags::none) const -> bool {
        return add_clause(Detail::transform(literals, [](auto const &lit) { return -lit; }), flags);
    }

    void add_watch(SolverLiteral literal) const {
        Detail::handle_error(clingo_propagate_control_add_watch(ctl_, literal));
    }

    [[nodiscard]] auto has_watch(SolverLiteral literal) const -> bool {
        return Detail::call<clingo_propagate_control_has_watch>(ctl_, literal);
    }

    [[nodiscard]] auto propagate() const -> bool { return Detail::call<clingo_propagate_control_propagate>(ctl_); }

    void remove_watch(SolverLiteral lit) const {
        Detail::handle_error(clingo_propagate_control_remove_watch(ctl_, lit));
    }

    [[nodiscard]] auto assignment() const -> Assignment {
        return Assignment{Detail::call<clingo_propagate_control_assignment>(ctl_)};
    }

    [[nodiscard]] auto thread_id() const -> ProgramId { return Detail::call<clingo_propagate_control_thread_id>(ctl_); }

  private:
    clingo_propagate_control_t *ctl_;
};

class Propagator {
  public:
    Propagator() = default;
    Propagator(Propagator &&other) = delete;
    virtual ~Propagator() = default;

    void init(PropagateInit init) { do_init(init); }

    void propagate(PropagateControl ctl, ProgramLiteralSpan changes) { do_propagate(ctl, changes); }

    void undo(ProgramId thread_id, Assignment assignment, ProgramLiteralSpan changes) {
        do_undo(thread_id, assignment, changes);
    }

    void check(PropagateControl ctl) { do_check(ctl); }

  private:
    virtual void do_init([[maybe_unused]] PropagateInit init) {}
    virtual void do_propagate([[maybe_unused]] PropagateControl ctl, [[maybe_unused]] ProgramLiteralSpan changes) {}
    virtual void do_undo([[maybe_unused]] uint32_t thread_id, [[maybe_unused]] Assignment assignment,
                         [[maybe_unused]] ProgramLiteralSpan changes) {}
    virtual void do_check([[maybe_unused]] PropagateControl ctl) {}
};

class Heuristic : public Propagator {
  public:
    Heuristic() = default;
    Heuristic(Heuristic &&other) = delete;

    auto operator=(Heuristic const &other) -> Heuristic & = delete;
    auto decide(ProgramId thread_id, Assignment assignment, SolverLiteral literal) -> SolverLiteral {
        return do_decide(thread_id, assignment, literal);
    }

  private:
    virtual auto do_decide([[maybe_unused]] uint32_t thread_id, [[maybe_unused]] Assignment assignment,
                           clingo_literal_t literal) -> clingo_literal_t {
        return literal;
    }
};

namespace Detail {

inline static constexpr auto c_propagator = clingo_propagator_t{
    [](clingo_propagate_init_t *init, void *data) -> bool {
        auto &self = *static_cast<Propagator *>(data);
        CLINGO_TRY {
            self.init(PropagateInit{init});
        }
        CLINGO_CATCH;
    },
    [](clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, void *data) -> bool {
        auto &self = *static_cast<Propagator *>(data);
        CLINGO_TRY {
            self.propagate(PropagateControl{control}, SolverLiteralSpan{changes, size});
        }
        CLINGO_CATCH;
    },
    [](clingo_propagate_control_t const *control, clingo_literal_t const *changes, size_t size, void *data) {
        auto &self = *static_cast<Propagator *>(data);
        try {
            uint32_t thread_id = Detail::call<clingo_propagate_control_thread_id>(control);
            clingo_assignment_t const *assignment = Detail::call<clingo_propagate_control_assignment>(control);
            self.undo(thread_id, Assignment{assignment}, SolverLiteralSpan{changes, size});
        } catch (std::exception const &e) {
            printf("panic: %s\n", e.what());
            std::abort();
        }
    },
    [](clingo_propagate_control_t *control, void *data) -> bool {
        auto &self = *static_cast<Propagator *>(data);
        CLINGO_TRY {
            self.check(PropagateControl{control});
        }
        CLINGO_CATCH;
    },
    nullptr,
    [](void *data) { std::ignore = std::unique_ptr<Propagator>(static_cast<Propagator *>(data)); },
};

inline static constexpr auto c_heuristic = clingo_propagator_t{
    c_propagator.init,
    c_propagator.propagate,
    c_propagator.undo,
    c_propagator.check,
    [](clingo_id_t thread_id, clingo_assignment_t const *assignment, clingo_literal_t fallback, void *data,
       clingo_literal_t *decision) -> bool {
        auto &self = *static_cast<Propagator *>(data);
        CLINGO_TRY {
            // NOLINTBEGIN
            *decision = static_cast<Heuristic &>(self).decide(thread_id, Assignment{assignment}, fallback);
            // NOLINTEND
        }
        CLINGO_CATCH;
    },
    c_propagator.free,
};

} // namespace Detail

} // namespace Clingo
