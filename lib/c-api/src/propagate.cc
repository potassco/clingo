#include <clingo/propagate.h>

#include <potassco/clingo.h>

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

using namespace CppClingo::CAPI;

using PotasscoCheckMode = Potassco::PropagatorCheckMode;
using PotasscoUndoMode = Potassco::PropagatorUndoMode;

struct clingo_propagate_init {
    static_assert(static_cast<clingo_propagator_check_mode_t>(PotasscoCheckMode::fixpoint) ==
                  clingo_propagator_check_mode_fixpoint);
    static_assert(static_cast<clingo_propagator_check_mode_t>(PotasscoCheckMode::both) ==
                  clingo_propagator_check_mode_both);
    static_assert(static_cast<clingo_propagator_check_mode_t>(PotasscoCheckMode::no) ==
                  clingo_propagator_check_mode_none);
    static_assert(static_cast<clingo_propagator_check_mode_t>(PotasscoCheckMode::total) ==
                  clingo_propagator_check_mode_total);
    static_assert(static_cast<clingo_propagator_undo_mode_t>(PotasscoUndoMode::always) ==
                  clingo_propagator_undo_mode_always);
    static_assert(static_cast<clingo_propagator_undo_mode_t>(PotasscoUndoMode::def) ==
                  clingo_propagator_undo_mode_default);
    static_assert(clingo_weight_constraint_type_implication_left < 0);
    static_assert(clingo_weight_constraint_type_implication_right > 0);
    static_assert(clingo_weight_constraint_type_equivalence == 0);

    clingo_control_t *ctl;
    Potassco::AbstractPropagator::Init *init;
};

namespace CppClingo::CAPI {
namespace {

auto cpp_cast(clingo_assignment_t const *assignment) -> Potassco::AbstractAssignment const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::AbstractAssignment const *>(assignment);
}

auto c_cast(Potassco::AbstractAssignment const *assignment) -> clingo_assignment_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_assignment_t const *>(assignment);
}

auto cpp_cast(clingo_propagate_control_t const *control) -> Potassco::AbstractPropagator::Control const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::AbstractPropagator::Control const *>(control);
}

auto cpp_cast(clingo_propagate_control_t *control) -> Potassco::AbstractPropagator::Control * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::AbstractPropagator::Control *>(control);
}

auto c_cast(Potassco::AbstractPropagator::Control *control) -> clingo_propagate_control_t * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_propagate_control_t *>(control);
}

auto map(Potassco::TruthValue value) -> clingo_truth_value_t {
    static_assert(static_cast<clingo_truth_value_t>(Potassco::TruthValue::true_) == clingo_truth_value_true);
    static_assert(static_cast<clingo_truth_value_t>(Potassco::TruthValue::false_) == clingo_truth_value_false);
    static_assert(static_cast<clingo_truth_value_t>(Potassco::TruthValue::free) == clingo_truth_value_free);
    return static_cast<clingo_truth_value_t>(value);
}

class ClingoPropagator : public CppClingo::Control::Propagator {
  public:
    ClingoPropagator(clingo_control_t *ctl, clingo_propagator_t prop, void *data)
        : ctl_{ctl}, prop_(prop), data_(data) {}
    ~ClingoPropagator() override {
        if (prop_.free != nullptr) {
            prop_.free(data_);
        }
    }

    void init(const Potassco::AbstractAssignment &assignment, Init &init) override {
        if (prop_.init != nullptr) {
            auto cinit = clingo_propagate_init{ctl_, &init};
            handle_error(prop_.init(c_cast(&assignment), &cinit, data_));
        }
    }

    void attach(const Potassco::AbstractAssignment &assignment, Control &solver) override {
        if (prop_.attach != nullptr) {
            handle_error(prop_.attach(c_cast(&assignment), c_cast(&solver), data_));
        }
    }

    void propagate(const Potassco::AbstractAssignment &assignment, Control &solver,
                   Potassco::LitSpan changes) override {
        if (prop_.propagate != nullptr) {
            handle_error(prop_.propagate(c_cast(&assignment), c_cast(&solver), changes.data(), changes.size(), data_));
        }
    }

    void undo(const Potassco::AbstractAssignment &assignment, Potassco::LitSpan undo) override {
        if (prop_.undo != nullptr) {
            prop_.undo(c_cast(&assignment), undo.data(), undo.size(), data_);
        }
    }

    void check(const Potassco::AbstractAssignment &assignment, Control &solver) override {
        if (prop_.check != nullptr) {
            handle_error(prop_.check(c_cast(&assignment), c_cast(&solver), data_));
        }
    }

    [[nodiscard]] auto hasHeuristic() const -> bool override { return prop_.decide != nullptr; }

    auto decide(const Potassco::AbstractAssignment &assignment, Potassco::Lit_t fallback) -> Potassco::Lit_t override {
        clingo_literal_t decision = 0;
        if (prop_.decide != nullptr) {
            handle_error(prop_.decide(c_cast(&assignment), fallback, data_, &decision));
        }
        return decision;
    }

  private:
    clingo_control_t *ctl_;
    clingo_propagator_t prop_;
    void *data_;
};

} // namespace
} // namespace CppClingo::CAPI

extern "C" auto clingo_assignment_thread_id(clingo_assignment_t const *assignment, clingo_id_t *id) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || id == nullptr) {
            return fail_arguments();
        }
        *id = cpp_cast(assignment)->solverId();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_decision_level(clingo_assignment_t const *assignment, uint32_t *level) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || level == nullptr) {
            return fail_arguments();
        }
        *level = cpp_cast(assignment)->level();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_root_level(clingo_assignment_t const *assignment, uint32_t *level) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || level == nullptr) {
            return fail_arguments();
        }
        *level = cpp_cast(assignment)->rootLevel();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_has_conflict(clingo_assignment_t const *assignment, bool *is_conflicting) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || is_conflicting == nullptr) {
            return fail_arguments();
        }
        *is_conflicting = cpp_cast(assignment)->hasConflict();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_has_literal(clingo_assignment_t const *assignment, clingo_literal_t literal,
                                              bool *is_valid) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || is_valid == nullptr) {
            return fail_arguments();
        }
        *is_valid = cpp_cast(assignment)->hasLit(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_level(clingo_assignment_t const *assignment, clingo_literal_t literal,
                                        uint32_t *level) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || level == nullptr) {
            return fail_arguments();
        }
        *level = cpp_cast(assignment)->level(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_decision(clingo_assignment_t const *assignment, uint32_t level,
                                           clingo_literal_t *literal) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || literal == nullptr) {
            return fail_arguments();
        }
        *literal = cpp_cast(assignment)->decision(level);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_is_fixed(clingo_assignment_t const *assignment, clingo_literal_t literal,
                                           bool *is_fixed) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || is_fixed == nullptr) {
            return fail_arguments();
        }
        *is_fixed = cpp_cast(assignment)->isFixed(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_is_true(clingo_assignment_t const *assignment, clingo_literal_t literal,
                                          bool *is_true) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || is_true == nullptr) {
            return fail_arguments();
        }
        *is_true = cpp_cast(assignment)->isTrue(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_is_false(clingo_assignment_t const *assignment, clingo_literal_t literal,
                                           bool *is_false) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || is_false == nullptr) {
            return fail_arguments();
        }
        *is_false = cpp_cast(assignment)->isFalse(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_truth_value(clingo_assignment_t const *assignment, clingo_literal_t literal,
                                              clingo_truth_value_t *value) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || value == nullptr) {
            return fail_arguments();
        }
        *value = map(cpp_cast(assignment)->value(literal));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_size(clingo_assignment_t const *assignment, size_t *size) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || size == nullptr) {
            return fail_arguments();
        }
        *size = cpp_cast(assignment)->size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_at(clingo_assignment_t const *assignment, size_t offset, clingo_literal_t *literal)
    -> bool {
    CLINGO_TRY {
        if (offset >= cpp_cast(assignment)->size()) {
            return fail_with(clingo_result_range, "index out of range");
        }
        *literal = static_cast<clingo_literal_t>(offset + 1);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_is_total(clingo_assignment_t const *assignment, bool *is_total) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || is_total == nullptr) {
            return fail_arguments();
        }
        *is_total = cpp_cast(assignment)->isTotal();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_trail_size(clingo_assignment_t const *assignment, uint32_t *size) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || size == nullptr) {
            return fail_arguments();
        }
        *size = cpp_cast(assignment)->trailSize();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_trail_begin(clingo_assignment_t const *assignment, uint32_t level, uint32_t *offset)
    -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || offset == nullptr) {
            return fail_arguments();
        }
        *offset = cpp_cast(assignment)->trailBegin(level);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_trail_end(clingo_assignment_t const *assignment, uint32_t level, uint32_t *offset)
    -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || offset == nullptr) {
            return fail_arguments();
        }
        *offset = cpp_cast(assignment)->trailEnd(level);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_trail_at(clingo_assignment_t const *assignment, uint32_t offset,
                                           clingo_literal_t *literal) -> bool {
    CLINGO_TRY {
        if (assignment == nullptr || literal == nullptr) {
            return fail_arguments();
        }
        *literal = cpp_cast(assignment)->trailAt(offset);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_solver_literal(clingo_propagate_init_t const *init,
                                                     clingo_literal_t aspif_literal, clingo_literal_t *solver_literal)
    -> bool {
    CLINGO_TRY {
        if (init == nullptr || solver_literal == nullptr) {
            return fail_arguments();
        }
        *solver_literal = init->init->solverLiteral(aspif_literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_freeze_literal(clingo_propagate_init_t *init, clingo_literal_t solver_literal)
    -> bool {
    CLINGO_TRY {
        if (init == nullptr) {
            return fail_arguments();
        }
        init->init->freezeVariable(solver_literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_library(clingo_propagate_init_t const *init, clingo_lib_t **lib) -> bool {
    CLINGO_TRY {
        if (init == nullptr || lib == nullptr) {
            return fail_arguments();
        }
        *lib = init->ctl->lib;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_base(clingo_propagate_init_t const *init, clingo_base_t const **base) -> bool {
    CLINGO_TRY {
        if (init == nullptr || base == nullptr) {
            return fail_arguments();
        }
        // NOLINTNEXTLINE
        *base = reinterpret_cast<clingo_base_t const *>(init->ctl->slv);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_control(clingo_propagate_init_t const *init, clingo_propagate_control_t **control)
    -> bool {
    CLINGO_TRY {
        if (init == nullptr || control == nullptr) {
            return fail_arguments();
        }
        *control = c_cast(init->init);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_number_of_threads(clingo_propagate_init_t const *init, clingo_id_t *threads)
    -> bool {
    CLINGO_TRY {
        if (init == nullptr || threads == nullptr) {
            return fail_arguments();
        }
        *threads = init->init->numSolver();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_set_check_mode(clingo_propagate_init_t *init, clingo_propagator_check_mode_t mode)
    -> bool {
    CLINGO_TRY {
        if (init == nullptr) {
            return fail_arguments();
        }
        init->init->setCheckMode(static_cast<PotasscoCheckMode>(mode));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_get_check_mode(clingo_propagate_init_t const *init,
                                                     clingo_propagator_check_mode_t *mode) -> bool {
    CLINGO_TRY {
        if (init == nullptr) {
            return fail_arguments();
        }
        *mode = static_cast<clingo_propagator_check_mode_t>(init->init->checkMode());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_set_undo_mode(clingo_propagate_init_t *init, clingo_propagator_undo_mode_t mode)
    -> bool {
    CLINGO_TRY {
        if (init == nullptr) {
            return fail_arguments();
        }
        init->init->setUndoMode(static_cast<PotasscoUndoMode>(mode));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_get_undo_mode(clingo_propagate_init_t const *init,
                                                    clingo_propagator_undo_mode_t *mode) -> bool {
    CLINGO_TRY {
        if (init == nullptr) {
            return fail_arguments();
        }
        *mode = static_cast<clingo_propagator_undo_mode_t>(init->init->undoMode());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_add_minimize(clingo_propagate_init_t *init, clingo_literal_t solver_literal,
                                                   clingo_weight_t weight, clingo_weight_t priority) -> bool {
    CLINGO_TRY {
        if (init == nullptr) {
            return fail_arguments();
        }
        init->init->addMinimize(priority, {solver_literal, weight});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_add_literal(clingo_propagate_control_t *control, bool freeze,
                                                     clingo_literal_t *result) -> bool {
    CLINGO_TRY {
        if (control == nullptr || result == nullptr) {
            return fail_arguments();
        }
        *result = cpp_cast(control)->addVariable(freeze);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_add_watch(clingo_propagate_control_t *control, clingo_literal_t literal)
    -> bool {
    CLINGO_TRY {
        if (control == nullptr) {
            return fail_arguments();
        }
        cpp_cast(control)->addWatch(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_has_watch(clingo_propagate_control_t const *control, clingo_literal_t literal,
                                                   bool *has_watch) -> bool {
    CLINGO_TRY {
        if (control == nullptr || has_watch == nullptr) {
            return fail_arguments();
        }
        *has_watch = cpp_cast(control)->hasWatch(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_remove_watch(clingo_propagate_control_t *control, clingo_literal_t literal)
    -> bool {
    CLINGO_TRY {
        if (control == nullptr) {
            return fail_arguments();
        }
        cpp_cast(control)->removeWatch(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_add_clause(clingo_propagate_control_t *control,
                                                    clingo_literal_t const *literals, size_t size,
                                                    clingo_clause_type_t type, bool *result) -> bool {
    CLINGO_TRY {
        if (control == nullptr || (literals == nullptr && size > 0) || result == nullptr) {
            return fail_arguments();
        }
        static_assert(static_cast<clingo_clause_type_t>(Potassco::ClauseType::learnt) == clingo_clause_type_learnt);
        static_assert(static_cast<clingo_clause_type_t>(Potassco::ClauseType::locked) == clingo_clause_type_static);
        static_assert(static_cast<clingo_clause_type_t>(Potassco::ClauseType::transient) ==
                      clingo_clause_type_volatile);
        static_assert(static_cast<clingo_clause_type_t>(Potassco::ClauseType::transient_locked) ==
                      clingo_clause_type_volatile_static);
        *result = cpp_cast(control)->addClause(std::span{literals, size}, static_cast<Potassco::ClauseType>(type));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_add_weight_constraint(
    clingo_propagate_control_t *control, clingo_literal_t solver_literal, clingo_weighted_literal_t const *literals,
    size_t size, clingo_weight_t bound, clingo_weight_constraint_type_t type, bool *result) -> bool {
    CLINGO_TRY {
        if (control == nullptr || (literals == nullptr && size > 0) || result == nullptr) {
            return fail_arguments();
        }
        *result = cpp_cast(control)->addWeightConstraint(solver_literal, map(literals, size), bound, type);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_propagate(clingo_propagate_control_t *control, bool *result) -> bool {
    CLINGO_TRY {
        if (control == nullptr || result == nullptr) {
            return fail_arguments();
        }
        *result = cpp_cast(control)->propagate();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_register_propagator(clingo_control_t *control, clingo_propagator_t const *propagator,
                                                   void *data) -> bool {
    CLINGO_TRY {
        if (control == nullptr || propagator == nullptr) {
            return fail_arguments();
        }
        control->slv->register_propagator(std::make_unique<ClingoPropagator>(control, *propagator, data));
    }
    CLINGO_CATCH;
}
