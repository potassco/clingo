#include <clingo/propagate.h>

#include <potassco/clingo.h>

#include "lib.hh"

namespace {

auto cpp_cast(clingo_assignment_t const *assignment) -> Potassco::AbstractAssignment const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::AbstractAssignment const *>(assignment);
}

auto map(Potassco::TruthValue value) -> clingo_truth_value_t {
    switch (value) {
        case Potassco::TruthValue::false_: {
            return clingo_truth_value_false;
        }
        case Potassco::TruthValue::true_: {
            return clingo_truth_value_true;
        }
        case Potassco::TruthValue::free: {
            return clingo_truth_value_free;
        }
        default: {
            throw std::logic_error{"invalid truth vlaue"};
        }
    }
}

} // namespace

extern "C" auto clingo_assignment_decision_level(clingo_assignment_t const *assignment, uint32_t *level)
    -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || level == nullptr) {
            return clingo_result_invalid;
        }
        *level = cpp_cast(assignment)->level();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_root_level(clingo_assignment_t const *assignment, uint32_t *level)
    -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || level == nullptr) {
            return clingo_result_invalid;
        }
        *level = cpp_cast(assignment)->rootLevel();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_has_literal(clingo_assignment_t const *assignment, clingo_literal_t literal,
                                              bool *is_valid) -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || is_valid == nullptr) {
            return clingo_result_invalid;
        }
        *is_valid = cpp_cast(assignment)->hasLit(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_level(clingo_assignment_t const *assignment, clingo_literal_t literal,
                                        uint32_t *level) -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || level == nullptr) {
            return clingo_result_invalid;
        }
        *level = cpp_cast(assignment)->level(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_decision(clingo_assignment_t const *assignment, uint32_t level,
                                           clingo_literal_t *literal) -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || literal == nullptr) {
            return clingo_result_invalid;
        }
        *literal = cpp_cast(assignment)->decision(level);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_is_fixed(clingo_assignment_t const *assignment, clingo_literal_t literal,
                                           bool *is_fixed) -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || is_fixed == nullptr) {
            return clingo_result_invalid;
        }
        *is_fixed = cpp_cast(assignment)->isFixed(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_is_true(clingo_assignment_t const *assignment, clingo_literal_t literal,
                                          bool *is_true) -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || is_true == nullptr) {
            return clingo_result_invalid;
        }
        *is_true = cpp_cast(assignment)->isTrue(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_is_false(clingo_assignment_t const *assignment, clingo_literal_t literal,
                                           bool *is_false) -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || is_false == nullptr) {
            return clingo_result_invalid;
        }
        *is_false = cpp_cast(assignment)->isFalse(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_truth_value(clingo_assignment_t const *assignment, clingo_literal_t literal,
                                              clingo_truth_value_t *value) -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || value == nullptr) {
            return clingo_result_invalid;
        }
        *value = map(cpp_cast(assignment)->value(literal));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_size(clingo_assignment_t const *assignment, size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || size == nullptr) {
            return clingo_result_invalid;
        }
        *size = cpp_cast(assignment)->size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_at(clingo_assignment_t const *assignment, size_t offset, clingo_literal_t *literal)
    -> clingo_result_t {
    CLINGO_TRY {
        if (offset >= cpp_cast(assignment)->size()) {
            return clingo_result_range;
        }
        *literal = static_cast<clingo_literal_t>(offset + 1);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_is_total(clingo_assignment_t const *assignment, bool *is_total) -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || is_total == nullptr) {
            return clingo_result_invalid;
        }
        *is_total = cpp_cast(assignment)->isTotal();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_trail_size(clingo_assignment_t const *assignment, uint32_t *size) -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || size == nullptr) {
            return clingo_result_invalid;
        }
        *size = cpp_cast(assignment)->trailSize();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_trail_begin(clingo_assignment_t const *assignment, uint32_t level, uint32_t *offset)
    -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || offset == nullptr) {
            return clingo_result_invalid;
        }
        *offset = cpp_cast(assignment)->trailBegin(level);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_trail_end(clingo_assignment_t const *assignment, uint32_t level, uint32_t *offset)
    -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || offset == nullptr) {
            return clingo_result_invalid;
        }
        *offset = cpp_cast(assignment)->trailEnd(level);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_assignment_trail_at(clingo_assignment_t const *assignment, uint32_t offset,
                                           clingo_literal_t *literal) -> clingo_result_t {
    CLINGO_TRY {
        if (assignment == nullptr || literal == nullptr) {
            return clingo_result_invalid;
        }
        *literal = cpp_cast(assignment)->trailAt(offset);
    }
    CLINGO_CATCH;
}

// NOLINTBEGIN

extern "C" auto clingo_propagate_init_solver_literal(clingo_propagate_init_t const *init,
                                                     clingo_literal_t aspif_literal, clingo_literal_t *solver_literal)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_add_watch(clingo_propagate_init_t *init, clingo_literal_t solver_literal)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_add_watch_to_thread(clingo_propagate_init_t *init,
                                                          clingo_literal_t solver_literal, clingo_id_t thread_id)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_remove_watch(clingo_propagate_init_t *init, clingo_literal_t solver_literal)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_remove_watch_from_thread(clingo_propagate_init_t *init,
                                                               clingo_literal_t solver_literal, uint32_t thread_id)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_freeze_literal(clingo_propagate_init_t *init, clingo_literal_t solver_literal)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_base(clingo_propagate_init_t const *init, clingo_base_t const **base)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_number_of_threads(clingo_propagate_init_t const *init, size_t *threads)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_set_check_mode(clingo_propagate_init_t *init, clingo_propagator_check_mode_t mode)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_get_check_mode(clingo_propagate_init_t const *init,
                                                     clingo_propagator_check_mode_t *mode) -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_set_undo_mode(clingo_propagate_init_t *init, clingo_propagator_undo_mode_t mode)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_get_undo_mode(clingo_propagate_init_t const *init,
                                                    clingo_propagator_undo_mode_t *mode) -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_assignment(clingo_propagate_init_t const *init,
                                                 clingo_assignment_t const **assignment) -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_add_literal(clingo_propagate_init_t *init, bool freeze, clingo_literal_t *result)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_add_clause(clingo_propagate_init_t *init, clingo_literal_t const *clause,
                                                 size_t size, bool *result) -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_add_weight_constraint(clingo_propagate_init_t *init, clingo_literal_t literal,
                                                            clingo_weighted_literal_t const *literals, size_t size,
                                                            clingo_weight_t bound, clingo_weight_constraint_type_t type,
                                                            bool compare_equal, bool *result) -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_add_minimize(clingo_propagate_init_t *init, clingo_literal_t literal,
                                                   clingo_weight_t weight, clingo_weight_t priority)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_propagate(clingo_propagate_init_t *init, bool *result) -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_thread_id(clingo_propagate_control_t const *control, clingo_id_t *thread_id)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_assignment(clingo_propagate_control_t const *control,
                                                    clingo_assignment_t const **assignment) -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_add_literal(clingo_propagate_control_t *control, clingo_literal_t *result)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_add_watch(clingo_propagate_control_t *control, clingo_literal_t literal)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_has_watch(clingo_propagate_control_t const *control, clingo_literal_t literal,
                                                   bool *has_watch) -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_remove_watch(clingo_propagate_control_t *control, clingo_literal_t literal)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_add_clause(clingo_propagate_control_t *control, clingo_literal_t const *clause,
                                                    size_t size, clingo_clause_type_t type, bool *result)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_propagate(clingo_propagate_control_t *control, bool *result)
    -> clingo_result_t {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

// NOLINTEND
