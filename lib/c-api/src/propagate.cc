#include <clingo/propagate.h>

#include <clasp/clause.h>
#include <clasp/weight_constraint.h>

#include <potassco/clingo.h>

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

namespace {

//! Class to initialize a propagator.
class PropagateInit {
  public:
    using Lit_t = Potassco::Lit_t;

    static_assert(static_cast<clingo_propagator_check_mode_t>(Clasp::ClingoPropagatorCheckType::fixpoint) ==
                  clingo_propagator_check_mode_fixpoint);
    static_assert(static_cast<clingo_propagator_check_mode_t>(Clasp::ClingoPropagatorCheckType::both) ==
                  clingo_propagator_check_mode_both);
    static_assert(static_cast<clingo_propagator_check_mode_t>(Clasp::ClingoPropagatorCheckType::no) ==
                  clingo_propagator_check_mode_none);
    static_assert(static_cast<clingo_propagator_check_mode_t>(Clasp::ClingoPropagatorCheckType::total) ==
                  clingo_propagator_check_mode_total);

    static_assert(static_cast<clingo_propagator_undo_mode_t>(Clasp::ClingoPropagatorUndoType::always) ==
                  clingo_propagator_undo_mode_always);
    static_assert(static_cast<clingo_propagator_undo_mode_t>(Clasp::ClingoPropagatorUndoType::def) ==
                  clingo_propagator_undo_mode_default);

    PropagateInit(Clingo::Control::Solver &slv, Clasp::ClingoPropagatorInit &init)
        : slv_{&slv}, init_{&init}, assignment_{*facade_().ctx.master()}, cc_{facade_().ctx.master()} {
        init_->enableHistory(false);
    }
    [[nodiscard]] auto base() const -> Clingo::Control::BaseView const & { return *slv_; }
    [[nodiscard]] auto map_lit(Lit_t lit) const -> Potassco::Lit_t {
        const auto &prg = slv_->clasp_program();
        return Clasp::encodeLit(prg.getLiteral(lit, Clasp::Asp::MapLit::refined));
    }
    [[nodiscard]] auto threads() const -> uint32_t { return facade_().ctx.concurrency(); }
    void add_watch(Lit_t lit) { init_->addWatch(Clasp::decodeLit(lit)); }
    void add_watch(uint32_t thread_id, Lit_t lit) { init_->addWatch(thread_id, Clasp::decodeLit(lit)); }
    void remove_watch(Lit_t lit) { init_->removeWatch(Clasp::decodeLit(lit)); }
    void remove_watch(uint32_t thread_id, Lit_t lit) { init_->removeWatch(thread_id, Clasp::decodeLit(lit)); }
    void freeze_literal(Lit_t lit) { init_->freezeLit(Clasp::decodeLit(lit)); }
    void enable_history(bool enable) { init_->enableHistory(enable); };
    [[nodiscard]] auto add_literal(bool freeze) -> Potassco::Lit_t {
        auto &ctx = facade_().ctx;
        auto var = ctx.addVar(Clasp::VarType::atom);
        if (freeze) {
            ctx.setFrozen(var, true);
        }
        return Clasp::encodeLit(Clasp::Literal(var, false));
    }
    [[nodiscard]] auto add_clause(Potassco::LitSpan lits) -> bool {
        auto &ctx = facade_().ctx;
        if (ctx.master()->hasConflict()) {
            return false;
        }
        cc_.start();
        for (const auto &lit : lits) {
            cc_.add(Clasp::decodeLit(lit));
        }
        return cc_.end(Clasp::ClauseCreator::clause_force_simplify).ok();
    }
    [[nodiscard]] auto add_weight_constraint(Potassco::Lit_t lit, Potassco::WeightLitSpan lits,
                                             Potassco::Weight_t bound, clingo_weight_constraint_type_t type, bool eq)
        -> bool {
        auto &ctx = facade_().ctx;
        auto &master = *ctx.master();
        if (master.hasConflict()) {
            return false;
        }
        Clasp::WeightLitVec clits;
        clits.reserve(lits.size());
        for (const auto &x : lits) {
            clits.push_back({Clasp::decodeLit(x.lit), x.weight});
        }
        uint32_t flags = 0;
        if (eq) {
            flags |= Clasp::WeightConstraint::create_eq_bound;
        }
        switch (type) {
            case clingo_weight_constraint_type_implication_left: {
                flags |= Clasp::WeightConstraint::create_only_bfb;
                break;
            }
            case clingo_weight_constraint_type_implication_right: {
                flags |= Clasp::WeightConstraint::create_only_btb;
                break;
            }
            default: {
                break;
            }
        }
        // for bug in clang-analyzer
        // NOLINTBEGIN
        return Clasp::WeightConstraint::create(*ctx.master(), Clasp::decodeLit(lit), clits, bound,
                                               static_cast<Clasp::WeightConstraint::CreateFlag>(flags))
            .ok();
        // NOLINTEND
    }
    void add_minimize(Potassco::Lit_t literal, Potassco::Weight_t weight, Potassco::Weight_t priority) {
        auto &ctx = facade_().ctx;
        if (ctx.master()->hasConflict()) {
            return;
        }
        ctx.addMinimize({Clasp::decodeLit(literal), weight}, priority);
    }
    [[nodiscard]] auto propagate() -> bool {
        auto &ctx = facade_().ctx;
        if (ctx.master()->hasConflict()) {
            return false;
        }
        return ctx.master()->propagate();
    }
    void set_check_mode(clingo_propagator_check_mode_t mode) {
        init_->enableClingoPropagatorCheck(static_cast<Clasp::ClingoPropagatorCheckType>(mode));
    }
    void set_undo_mode(clingo_propagator_undo_mode_t mode) {
        init_->enableClingoPropagatorUndo(static_cast<Clasp::ClingoPropagatorUndoType>(mode));
    }
    [[nodiscard]] auto assignment() const -> Potassco::AbstractAssignment const & { return assignment_; }
    [[nodiscard]] auto check_mode() const -> clingo_propagator_check_mode_t {
        return static_cast<clingo_propagator_undo_mode_t>(init_->checkMode());
    }
    [[nodiscard]] auto undo_mode() const -> clingo_propagator_undo_mode_t {
        return static_cast<clingo_propagator_undo_mode_t>(init_->undoMode());
    }

  private:
    [[nodiscard]] auto facade_() const -> Clasp::ClaspFacade & { return slv_->clasp_facade(); }

    Clingo::Control::Solver *slv_;
    Clasp::ClingoPropagatorInit *init_;
    Clasp::ClingoAssignment assignment_;
    Clasp::ClauseCreator cc_;
};

auto cpp_cast(clingo_propagate_init_t *init) -> PropagateInit * {
    // NOLINTNEXTLINE
    return reinterpret_cast<PropagateInit *>(init);
}

auto cpp_cast(clingo_propagate_init_t const *init) -> PropagateInit const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<PropagateInit const *>(init);
}

auto c_cast(PropagateInit *init) -> clingo_propagate_init_t * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_propagate_init_t *>(init);
}

auto cpp_cast(clingo_assignment_t const *assignment) -> Potassco::AbstractAssignment const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::AbstractAssignment const *>(assignment);
}

auto c_cast(Potassco::AbstractAssignment const *assignment) -> clingo_assignment_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_assignment_t const *>(assignment);
}

auto cpp_cast(clingo_propagate_control_t const *control) -> Potassco::AbstractSolver const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::AbstractSolver const *>(control);
}

auto cpp_cast(clingo_propagate_control_t *control) -> Potassco::AbstractSolver * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::AbstractSolver *>(control);
}

auto c_cast(Potassco::AbstractSolver const *init) -> clingo_propagate_control_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_propagate_control_t const *>(init);
}

auto c_cast(Potassco::AbstractSolver *init) -> clingo_propagate_control_t * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_propagate_control_t *>(init);
}

auto map(Potassco::TruthValue value) -> clingo_truth_value_t {
    static_assert(static_cast<clingo_truth_value_t>(Potassco::TruthValue::true_) == clingo_truth_value_true);
    static_assert(static_cast<clingo_truth_value_t>(Potassco::TruthValue::false_) == clingo_truth_value_false);
    static_assert(static_cast<clingo_truth_value_t>(Potassco::TruthValue::free) == clingo_truth_value_free);
    return static_cast<clingo_truth_value_t>(value);
}

class ClingoPropagator : public Clingo::Control::Propagator {
  public:
    ClingoPropagator(clingo_propagator_t prop, void *data) : prop_(prop), data_(data) {}

    void init(Clingo::Control::Solver &slv, Clasp::ClingoPropagatorInit &init) override {
        if (prop_.init != nullptr) {
            auto cinit = PropagateInit{slv, init};
            handle_error(prop_.init(c_cast(&cinit), data_));
        }
    }

    void propagate(Potassco::AbstractSolver &solver, Potassco::LitSpan changes) override {
        if (prop_.propagate != nullptr) {
            handle_error(prop_.propagate(c_cast(&solver), changes.data(), changes.size(), data_));
        }
    }

    void undo(Potassco::AbstractSolver const &solver, Potassco::LitSpan undo) override {
        if (prop_.undo != nullptr) {
            prop_.undo(c_cast(&solver), undo.data(), undo.size(), data_);
        }
    }

    void check(Potassco::AbstractSolver &solver) override {
        if (prop_.check != nullptr) {
            handle_error(prop_.check(c_cast(&solver), data_));
        }
    }

    [[nodiscard]] auto hasHeuristic() const -> bool override { return prop_.decide != nullptr; }

    auto decide(Potassco::Id_t solverId, const Potassco::AbstractAssignment &assignment, Potassco::Lit_t fallback)
        -> Potassco::Lit_t override {
        clingo_literal_t decision = 0;
        if (prop_.decide != nullptr) {
            handle_error(prop_.decide(solverId, c_cast(&assignment), fallback, data_, &decision));
        }
        return decision;
    }

  private:
    clingo_propagator_t prop_;
    void *data_;
};

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

extern "C" auto clingo_propagate_init_solver_literal(clingo_propagate_init_t const *init,
                                                     clingo_literal_t aspif_literal, clingo_literal_t *solver_literal)
    -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr || solver_literal == nullptr) {
            return clingo_result_invalid;
        }
        *solver_literal = cpp_cast(init)->map_lit(aspif_literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_add_watch(clingo_propagate_init_t *init, clingo_literal_t solver_literal)
    -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr) {
            return clingo_result_invalid;
        }
        cpp_cast(init)->add_watch(solver_literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_add_watch_to_thread(clingo_propagate_init_t *init,
                                                          clingo_literal_t solver_literal, clingo_id_t thread_id)
    -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr) {
            return clingo_result_invalid;
        }
        cpp_cast(init)->add_watch(thread_id, solver_literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_remove_watch(clingo_propagate_init_t *init, clingo_literal_t solver_literal)
    -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr) {
            return clingo_result_invalid;
        }
        cpp_cast(init)->remove_watch(solver_literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_remove_watch_from_thread(clingo_propagate_init_t *init,
                                                               clingo_literal_t solver_literal, uint32_t thread_id)
    -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr) {
            return clingo_result_invalid;
        }
        cpp_cast(init)->remove_watch(thread_id, solver_literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_freeze_literal(clingo_propagate_init_t *init, clingo_literal_t solver_literal)
    -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr) {
            return clingo_result_invalid;
        }
        cpp_cast(init)->freeze_literal(solver_literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_base(clingo_propagate_init_t const *init, clingo_base_t const **base)
    -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr) {
            return clingo_result_invalid;
        }
        // NOLINTNEXTLINE
        *base = reinterpret_cast<clingo_base_t const *>(&cpp_cast(init)->base());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_number_of_threads(clingo_propagate_init_t const *init, size_t *threads)
    -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr || threads == nullptr) {
            return clingo_result_invalid;
        }
        *threads = cpp_cast(init)->threads();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_set_check_mode(clingo_propagate_init_t *init, clingo_propagator_check_mode_t mode)
    -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr) {
            return clingo_result_invalid;
        }
        cpp_cast(init)->set_check_mode(mode);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_get_check_mode(clingo_propagate_init_t const *init,
                                                     clingo_propagator_check_mode_t *mode) -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr) {
            return clingo_result_invalid;
        }
        *mode = cpp_cast(init)->check_mode();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_set_undo_mode(clingo_propagate_init_t *init, clingo_propagator_undo_mode_t mode)
    -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr) {
            return clingo_result_invalid;
        }
        cpp_cast(init)->set_undo_mode(mode);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_get_undo_mode(clingo_propagate_init_t const *init,
                                                    clingo_propagator_undo_mode_t *mode) -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr) {
            return clingo_result_invalid;
        }
        *mode = cpp_cast(init)->undo_mode();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_assignment(clingo_propagate_init_t const *init,
                                                 clingo_assignment_t const **assignment) -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr) {
            return clingo_result_invalid;
        }
        *assignment = c_cast(&cpp_cast(init)->assignment());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_add_literal(clingo_propagate_init_t *init, bool freeze,
                                                  clingo_literal_t *solver_literal) -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr || solver_literal == nullptr) {
            return clingo_result_invalid;
        }
        *solver_literal = cpp_cast(init)->add_literal(freeze);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_add_clause(clingo_propagate_init_t *init, clingo_literal_t const *literals,
                                                 size_t size, bool *result) -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr || (literals == nullptr && size > 0) || result == nullptr) {
            return clingo_result_invalid;
        }
        *result = cpp_cast(init)->add_clause(std::span{literals, size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_add_weight_constraint(clingo_propagate_init_t *init,
                                                            clingo_literal_t solver_literal,
                                                            clingo_weighted_literal_t const *literals, size_t size,
                                                            clingo_weight_t bound, clingo_weight_constraint_type_t type,
                                                            bool compare_equal, bool *result) -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr || (literals == nullptr && size > 0) || result == nullptr) {
            return clingo_result_invalid;
        }
        *result =
            cpp_cast(init)->add_weight_constraint(solver_literal, map(literals, size), bound, type, compare_equal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_add_minimize(clingo_propagate_init_t *init, clingo_literal_t solver_literal,
                                                   clingo_weight_t weight, clingo_weight_t priority)
    -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr) {
            return clingo_result_invalid;
        }
        cpp_cast(init)->add_minimize(solver_literal, weight, priority);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_init_propagate(clingo_propagate_init_t *init, bool *result) -> clingo_result_t {
    CLINGO_TRY {
        if (init == nullptr || result == nullptr) {
            return clingo_result_invalid;
        }
        *result = cpp_cast(init)->propagate();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_thread_id(clingo_propagate_control_t const *control, clingo_id_t *thread_id)
    -> clingo_result_t {
    CLINGO_TRY {
        if (control == nullptr || thread_id == nullptr) {
            return clingo_result_invalid;
        }
        *thread_id = cpp_cast(control)->id();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_assignment(clingo_propagate_control_t const *control,
                                                    clingo_assignment_t const **assignment) -> clingo_result_t {
    CLINGO_TRY {
        if (control == nullptr || assignment == nullptr) {
            return clingo_result_invalid;
        }
        *assignment = c_cast(&cpp_cast(control)->assignment());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_add_literal(clingo_propagate_control_t *control, clingo_literal_t *result)
    -> clingo_result_t {
    CLINGO_TRY {
        if (control == nullptr || result == nullptr) {
            return clingo_result_invalid;
        }
        *result = cpp_cast(control)->addVariable();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_add_watch(clingo_propagate_control_t *control, clingo_literal_t literal)
    -> clingo_result_t {
    CLINGO_TRY {
        if (control == nullptr) {
            return clingo_result_invalid;
        }
        cpp_cast(control)->addWatch(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_has_watch(clingo_propagate_control_t const *control, clingo_literal_t literal,
                                                   bool *has_watch) -> clingo_result_t {
    CLINGO_TRY {
        if (control == nullptr || has_watch == nullptr) {
            return clingo_result_invalid;
        }
        *has_watch = cpp_cast(control)->hasWatch(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_remove_watch(clingo_propagate_control_t *control, clingo_literal_t literal)
    -> clingo_result_t {
    CLINGO_TRY {
        if (control == nullptr) {
            return clingo_result_invalid;
        }
        cpp_cast(control)->removeWatch(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_propagate_control_add_clause(clingo_propagate_control_t *control,
                                                    clingo_literal_t const *literals, size_t size,
                                                    clingo_clause_type_t type, bool *result) -> clingo_result_t {
    CLINGO_TRY {
        if (control == nullptr || (literals == nullptr && size > 0) || result == nullptr) {
            return clingo_result_invalid;
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

extern "C" auto clingo_propagate_control_propagate(clingo_propagate_control_t *control, bool *result)
    -> clingo_result_t {
    CLINGO_TRY {
        if (control == nullptr || result == nullptr) {
            return clingo_result_invalid;
        }
        *result = cpp_cast(control)->propagate();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_register_propagator(clingo_control_t *control, clingo_propagator_t const *propagator,
                                                   void *data) -> clingo_result_t {
    CLINGO_TRY {
        control->slv->register_propagator(std::make_unique<ClingoPropagator>(*propagator, data));
    }
    CLINGO_CATCH;
}
