#include <clingo/propagate.h>

#include <pybind11/native_enum.h>

#include <algorithm>
#include <utility>

#include "base.hh"
#include "iterable.hh"
#include "propagate.hh"
#include "util.hh"

namespace PyClingo {

namespace {

auto to_rng(uint32_t size, py::slice const &slc) {
    return py::module::import("builtins").attr("range")(size)[slc];
}

} // namespace

class TrailView {
  public:
    using value_type = clingo_literal_t;

    TrailView(clingo_assignment_t const *assignment, py::object rng) : assignment_{assignment}, rng_{std::move(rng)} {}

    [[nodiscard]] auto slice(py::slice const &slc) const -> Sequence<clingo_literal_t> {
        return py::cast(TrailView{assignment_, rng_[slc]});
    }

    [[nodiscard]] auto at(uint32_t index) const -> clingo_literal_t {
        clingo_literal_t lit = 0;
        auto offset = rng_[py::int_{index}].cast<uint32_t>();
        handle_error(clingo_assignment_trail_at(assignment_, offset, &lit));
        return lit;
    }
    [[nodiscard]] auto size() const -> uint32_t { return py::len(rng_); }

  private:
    clingo_assignment_t const *assignment_;
    py::object rng_;
};

class Trail {
  public:
    using value_type = clingo_literal_t;

    Trail(clingo_assignment_t const *assignment) : assignment_{assignment} {}

    [[nodiscard]] auto slice(py::slice const &slc) const -> Sequence<clingo_literal_t> {
        return py::cast(TrailView{assignment_, to_rng(size(), slc)});
    }

    [[nodiscard]] auto at(uint32_t index) const -> clingo_literal_t {
        clingo_literal_t lit = 0;
        handle_error(clingo_assignment_trail_at(assignment_, index, &lit));
        return lit;
    }

    [[nodiscard]] auto size() const -> uint32_t {
        uint32_t size = 0;
        handle_error(clingo_assignment_trail_size(assignment_, &size));
        return size;
    }

    [[nodiscard]] auto begin_level(uint32_t level) const -> uint32_t {
        uint32_t offset = 0;
        handle_error(clingo_assignment_trail_begin(assignment_, level, &offset));
        return offset;
    }

    [[nodiscard]] auto end_level(uint32_t level) const -> uint32_t {
        uint32_t offset = 0;
        handle_error(clingo_assignment_trail_end(assignment_, level, &offset));
        return offset;
    }

    [[nodiscard]] auto level(uint32_t level) const -> Sequence<clingo_literal_t> {
        return slice(py::slice(begin_level(level), end_level(level), 1));
    }

  private:
    clingo_assignment_t const *assignment_;
};

class Assignment {
  public:
    using value_type = clingo_literal_t;

    Assignment(clingo_assignment_t const *assignment) : assignment_(assignment) {}

    [[nodiscard]] auto thread_id() const -> clingo_id_t {
        clingo_id_t id = 0;
        handle_error(clingo_assignment_thread_id(assignment_, &id));
        return id;
    }

    [[nodiscard]] auto size() const -> size_t {
        size_t size = 0;
        handle_error(clingo_assignment_size(assignment_, &size));
        return size;
    }

    [[nodiscard]] auto at(size_t size) const -> clingo_literal_t {
        clingo_literal_t lit = 0;
        handle_error(clingo_assignment_at(assignment_, size, &lit));
        return lit;
    }

    [[nodiscard]] auto decision(uint32_t level) const -> clingo_literal_t {
        clingo_literal_t lit = 0;
        handle_error(clingo_assignment_decision(assignment_, level, &lit));
        return lit;
    }

    [[nodiscard]] auto decision_level() const -> uint32_t {
        uint32_t level = 0;
        handle_error(clingo_assignment_decision_level(assignment_, &level));
        return level;
    }

    [[nodiscard]] auto has_conflict() const -> bool {
        auto res = false;
        handle_error(clingo_assignment_has_conflict(assignment_, &res));
        return res;
    }

    [[nodiscard]] auto contains(clingo_literal_t lit) const -> bool {
        auto res = false;
        handle_error(clingo_assignment_has_literal(assignment_, lit, &res));
        return res;
    }

    [[nodiscard]] auto is_false(clingo_literal_t lit) const -> bool {
        auto res = false;
        handle_error(clingo_assignment_is_false(assignment_, lit, &res));
        return res;
    }

    [[nodiscard]] auto is_fixed(clingo_literal_t lit) const -> bool {
        auto res = false;
        handle_error(clingo_assignment_is_fixed(assignment_, lit, &res));
        return res;
    }

    [[nodiscard]] auto is_free(clingo_literal_t lit) const -> bool {
        clingo_truth_value_t res = 0;
        handle_error(clingo_assignment_truth_value(assignment_, lit, &res));
        return res == clingo_truth_value_free;
    }

    [[nodiscard]] auto is_total() const -> bool {
        auto res = false;
        handle_error(clingo_assignment_is_total(assignment_, &res));
        return res;
    }

    [[nodiscard]] auto is_true(clingo_literal_t lit) const -> bool {
        auto res = false;
        handle_error(clingo_assignment_is_true(assignment_, lit, &res));
        return res;
    }

    [[nodiscard]] auto level(clingo_literal_t lit) const -> uint32_t {
        uint32_t level = 0;
        handle_error(clingo_assignment_level(assignment_, lit, &level));
        return level;
    }

    [[nodiscard]] auto value(clingo_literal_t lit) const -> std::optional<bool> {
        clingo_truth_value_t res = 0;
        handle_error(clingo_assignment_truth_value(assignment_, lit, &res));
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

    [[nodiscard]] auto root_level() const -> uint32_t {
        uint32_t level = 0;
        handle_error(clingo_assignment_root_level(assignment_, &level));
        return level;
    }

    [[nodiscard]] auto trail() const -> Trail { return Trail{assignment_}; }

  private:
    clingo_assignment_t const *assignment_;
};

class PropagateControl {
  public:
    PropagateControl(clingo_propagate_control_t *ctl) : ctl_{ctl} {}
    PropagateControl(clingo_propagate_init_t const *init) : ctl_{nullptr} {
        handle_error(clingo_propagate_init_control(init, &ctl_));
    }

    auto add_clause(LitSpan literals, bool tag, bool lock) -> bool {
        clingo_clause_type_t type = 0;
        if (tag) {
            type |= clingo_clause_type_volatile;
        }
        if (lock) {
            type |= clingo_clause_type_static;
        }
        auto res = false;
        handle_error(clingo_propagate_control_add_clause(ctl_, literals.data(), literals.size(), type, &res));
        return res;
    }

    auto add_weight_constraint(clingo_literal_t literal, WeightLitSpan literals, clingo_weight_t bound,
                               clingo_weight_constraint_type_e type) -> bool {
        auto res = false;
        handle_error(clingo_propagate_control_add_weight_constraint(ctl_, literal, literals.data(), literals.size(),
                                                                    bound, type, &res));
        return res;
    }

    auto add_literal(bool freeze = true) -> clingo_literal_t {
        clingo_literal_t lit = 0;
        handle_error(clingo_propagate_control_add_literal(ctl_, freeze, &lit));
        return lit;
    }

    auto add_nogood(LitSpan literals, bool tag, bool lock) -> bool {
        static thread_local auto lits = LitVec{};
        lits.clear();
        lits.reserve(literals.size());
        std::ranges::transform(literals.begin(), literals.end(), std::back_inserter(lits),
                               [](auto const &lit) { return -lit; });
        return add_clause(lits, tag, lock);
    }

    void add_watch(clingo_literal_t lit) { handle_error(clingo_propagate_control_add_watch(ctl_, lit)); }

    auto has_watch(clingo_literal_t lit) -> bool {
        auto res = false;
        handle_error(clingo_propagate_control_has_watch(ctl_, lit, &res));
        return res;
    }

    auto propagate() -> bool {
        auto res = false;
        handle_error(clingo_propagate_control_propagate(ctl_, &res));
        return res;
    }

    void remove_watch(clingo_literal_t lit) { handle_error(clingo_propagate_control_remove_watch(ctl_, lit)); }

  private:
    clingo_propagate_control_t *ctl_;
};

class PropagateInit : public PropagateControl {
  public:
    PropagateInit(clingo_propagate_init_t *init) : PropagateControl(init), init_{init} {}

    auto library() -> PyLibrary {
        clingo_lib_t *lib = nullptr;
        handle_error(clingo_propagate_init_library(init_, &lib));
        return Library::cast(lib);
    }

    auto base() -> Base {
        clingo_base_t const *base = nullptr;
        handle_error(clingo_propagate_init_base(init_, &base));
        return {base};
    }

    auto get_check_mode() -> clingo_propagator_check_mode_e {
        clingo_propagator_check_mode_t mode = 0;
        handle_error(clingo_propagate_init_get_check_mode(init_, &mode));
        return static_cast<clingo_propagator_check_mode_e>(mode);
    }

    void set_check_mode(clingo_propagator_check_mode_e mode) {
        handle_error(clingo_propagate_init_set_check_mode(init_, mode));
    }

    auto number_of_threads() -> clingo_id_t {
        clingo_id_t res = 0;
        handle_error(clingo_propagate_init_number_of_threads(init_, &res));
        return res;
    }

    auto get_undo_mode() -> clingo_propagator_undo_mode_e {
        clingo_propagator_check_mode_t mode = 0;
        handle_error(clingo_propagate_init_get_undo_mode(init_, &mode));
        return static_cast<clingo_propagator_undo_mode_e>(mode);
    }

    void set_undo_mode(clingo_propagator_undo_mode_e mode) {
        handle_error(clingo_propagate_init_set_undo_mode(init_, mode));
    }

    void add_minimize(clingo_literal_t literal, clingo_weight_t weight, clingo_weight_t priority) {
        handle_error(clingo_propagate_init_add_minimize(init_, literal, weight, priority));
    }

    void freeze_literal(clingo_literal_t lit) { handle_error(clingo_propagate_init_freeze_literal(init_, lit)); }

    auto solver_literal(clingo_literal_t lit) -> clingo_literal_t {
        clingo_literal_t res = 0;
        handle_error(clingo_propagate_init_solver_literal(init_, lit, &res));
        return res;
    }

  private:
    clingo_propagate_init_t *init_;
};

void Propagator::init(Assignment &assignment, PropagateInit &init) {
    PYBIND11_OVERRIDE_NAME(void, Propagator, "init", no_op, assignment, init);
}
void Propagator::attach(Assignment &assignment, PropagateControl &ctl) {
    PYBIND11_OVERRIDE_NAME(void, Propagator, "attach", no_op, assignment, ctl);
}
void Propagator::propagate(Assignment &assignment, PropagateControl &ctl, LitSpan changes) {
    PYBIND11_OVERRIDE_NAME(void, Propagator, "propagate", no_op, assignment, ctl, changes);
}
void Propagator::undo(Assignment &assignment, LitSpan changes) {
    PYBIND11_OVERRIDE_NAME(void, Propagator, "undo", no_op, assignment, changes);
}
void Propagator::check(Assignment &assignment, PropagateControl &ctl) {
    PYBIND11_OVERRIDE_NAME(void, Propagator, "check", no_op, assignment, ctl);
}
auto Propagator::decide(Assignment &assignment, clingo_literal_t lit) -> clingo_literal_t {
    PYBIND11_OVERRIDE_NAME(clingo_literal_t, Propagator, "decide", decide_, assignment, lit);
}

auto Propagator::decide_([[maybe_unused]] Assignment &assignment, [[maybe_unused]] clingo_literal_t lit)
    -> clingo_literal_t {
    static_cast<void>(this);
    return lit;
}

void register_propagator(clingo_control_t *ctl, Propagator &prop) {
    // propagator without heuristic
    static constexpr auto c_prop = clingo_propagator_t{
        [](clingo_assignment_t const *assignment, clingo_propagate_init_t *init, void *data) -> bool {
            auto *self = static_cast<Propagator *>(data);
            CLINGO_TRY {
                auto py_init = PropagateInit{init};
                auto py_ass = Assignment{assignment};
                self->init(py_ass, py_init);
            }
            CLINGO_CATCH;
        },
        [](clingo_assignment_t const *assignment, clingo_propagate_control_t *control, void *data) -> bool {
            auto *self = static_cast<Propagator *>(data);
            CLINGO_TRY {
                auto py_ass = Assignment{assignment};
                auto py_ctl = PropagateControl{control};
                self->attach(py_ass, py_ctl);
            }
            CLINGO_CATCH;
        },
        [](clingo_assignment_t const *assignment, clingo_propagate_control_t *control, clingo_literal_t const *changes,
           size_t size, void *data) -> bool {
            auto *self = static_cast<Propagator *>(data);
            CLINGO_TRY {
                auto py_ass = Assignment{assignment};
                auto py_ctl = PropagateControl{control};
                self->propagate(py_ass, py_ctl, LitSpan{changes, size});
            }
            CLINGO_CATCH;
        },
        [](clingo_assignment_t const *assignment, clingo_literal_t const *changes, size_t size, void *data) {
            try {
                auto *self = static_cast<Propagator *>(data);
                auto py_ass = Assignment{assignment};
                self->undo(py_ass, LitSpan{changes, size});
            } catch (std::exception const &e) {
                fprintf(stderr, "panic: %s\n", e.what());
                std::terminate();
            }
        },
        [](clingo_assignment_t const *assignment, clingo_propagate_control_t *control, void *data) -> bool {
            auto *self = static_cast<Propagator *>(data);
            CLINGO_TRY {
                auto py_ass = Assignment{assignment};
                auto py_ctl = PropagateControl{control};
                self->check(py_ass, py_ctl);
            }
            CLINGO_CATCH;
        },
        nullptr,
        nullptr,
    };
    // propagator with heuristic
    static constexpr auto c_heu = clingo_propagator_t{
        c_prop.init,
        c_prop.attach,
        c_prop.propagate,
        c_prop.undo,
        c_prop.check,
        [](clingo_assignment_t const *assignment, clingo_literal_t fallback, void *data,
           clingo_literal_t *decision) -> bool {
            auto *self = static_cast<Propagator *>(data);
            CLINGO_TRY {
                auto py_assignment = Assignment{assignment};
                *decision = self->decide(py_assignment, fallback);
            }
            CLINGO_CATCH;
        },
        nullptr,
    };
    auto has_heu = pybind11::get_override(&prop, "decide");
    handle_error(clingo_control_register_propagator(ctl, has_heu ? &c_heu : &c_prop, static_cast<void *>(&prop)));
}

void register_propagate(pybind11::module &m) {
    using namespace PyClingo;
    auto propagate = m.def_submodule("propagate", R"(
Functions and classes to implement custom propagators.

# Example

```python
>>> from typing import Sequence
>>> from clingo.control import Control
>>> from clingo.core import Library
>>> from clingo.propagate import Assignment, PropagateControl, PropagateInit, Propagator
>>> from clingo.symbol import Function
>>>
>>> LIB = Library()
>>>
>>> class AIFFB(Propagator):
...     slit_a: int
...     slit_b: int
...
...     def __init__(self) -> None:
...         super().__init__()
...         self.slit_a = 0
...         self.slit_b = 0
...
...     # add watches for atoms `a` and `b`
...     def init(self, assignment: Assignment, init: PropagateInit) -> None:
...         # get program literals for atoms `a` and `b`
...         plit_a = init.base[Function(LIB, "a")].literal
...         plit_b = init.base[Function(LIB, "b")].literal
...         # get solver literals for program literals `a` and `b`
...         self.slit_a = init.solver_literal(plit_a)
...         self.slit_b = init.solver_literal(plit_b)
...         # add watches for solver literals `a` and `b`
...         init.add_watch(self.slit_a)
...         init.add_watch(self.slit_b)
...
...     # propagate solver literals `a` and `b`
...     def propagate(self, assignment: Assignment, control: PropagateControl, changes: Sequence[int]) -> None:
...         # if `a` is true imply `b`
...         if self.slit_a in changes:
...             assert assignment.is_true(self.slit_a)
...             control.add_clause([-self.slit_a, self.slit_b])
...         # if `b` is true imply `a`
...         if self.slit_b in changes:
...             assert assignment.is_true(self.slit_b)
...             control.add_clause([-self.slit_b, self.slit_a])
...
>>> ctl = Control(LIB, ["0"])
>>> ctl.register_propagator(AIFFB())
>>> ctl.parse_string("1 { a; b }.")
>>> ctl.ground()
>>> with ctl.solve(on_model=print) as hnd:
>>>     print(hnd.get())
a b
SAT
```
)"_d);

    make_sequence(py::class_<TrailView>(propagate, "_TrailView", R"(
Provides access to a subrange of literals in the solver's trail.

Implements `Sequence[int]` to access the solver literals in the view.
)"_d));

    make_sequence(py::class_<Trail>(propagate, "Trail", R"(
Provides access to literals in the solver's trail.

The trail represents the sequence of literals assigned during the solving
process. It is structured by decision levels, where each level contains
literals assigned due to implications from the decision literal at that level.

The literals within each level are ordered by implication, reflecting the
logical dependencies between them as determined by the solver's propagation and
learning mechanisms. The decision literal for each level is placed at the
beginning of its respective sequence.

Implements `Sequence[int]` to access the solver literals in the trail.
)"_d))
        .def("level", &Trail::level, py::arg("level"), R"(
Get the literals assigned at the given decision level.

This sequence includes all literals assigned at this level in implication
order, with the decision literal at the beginning of the sequence.

Args:
    level: A decision level.
Returns:
    A sequence of literals.
)"_d)
        .def("begin", &Trail::begin_level, py::arg("level"), R"(
Get the index of the first literal on the given level.

This also corresponds to the decision literal for that level.

Args:
    level: A decision level.
Returns:
    The index of the decision literal.
)"_d)
        .def("end", &Trail::end_level, py::arg("level"), R"(
Get the index after the last literal on the given level.

Args:
    level: A decision level.
Returns:
    The index after the last literal.
)"_d);

    py::native_enum<clingo_weight_constraint_type_e>(propagate, "WeightConstraintType", "enum.IntEnum",
                                                     "Enumeration of weight constraint types.")
        .value("Equivalence", clingo_weight_constraint_type_equivalence,
               R"(The weight constraint is equal to its literal.)")
        .value("LeftImplication", clingo_weight_constraint_type_implication_left,
               R"(The weight constraint implies its literal.)")
        .value("RightImplication", clingo_weight_constraint_type_implication_right,
               R"(The literal implies the weight constraint.)")
        .finalize();

    py::native_enum<clingo_propagator_check_mode_e>(propagate, "CheckMode", "enum.IntEnum",
                                                    "Enumeration of check modes.")
        .value("Off", clingo_propagator_check_mode_none, R"(Do not call `Propagator.check()` at all.)")
        .value("Fixpoint", clingo_propagator_check_mode_fixpoint,
               R"(Call `Propagator.check()` on propagation fixpoints.)")
        .value("Total", clingo_propagator_check_mode_total, R"(Call `Propagator.check()` on total assignments.)")
        .value("Both", clingo_propagator_check_mode_both,
               R"(Call `Propagator.check()` on propagation fixpoints and total assignments.)")
        .finalize();

    py::native_enum<clingo_propagator_undo_mode_e>(propagate, "UndoMode", "enum.IntEnum", "Enumeration of undo modes.")
        .value("Default", clingo_propagator_undo_mode_default,
               R"(Call `Propagator.undo()` for decision levels with non-emty changes.)")
        .value("Always", clingo_propagator_undo_mode_always,
               R"(Additionally call `Propagator.undo()` when check has been called.)")
        .finalize();

    make_sequence(py::class_<Assignment>(propagate, "Assignment", R"(
Provides information about the current state of literals in the solver.

It provides methods to inspect and query the assignment, which is essential for
implementing custom propagators.

Key concepts:
- Each literal is either true, false, or unassigned.
- Each assigned literal has a decision level.
- There is exactly one decision literal per level.
- All other literals on the same level are implied by the decision literal and
  literals from earlier levels.
- The current decision level is the highest level at which atoms are assigned.
- The root level is the lowest decision level that can be backtracked to.

Implements `Sequence[int]` to access the solver literals in the assignment.
)"_d))
        .def("decision", &Assignment::decision, py::arg("level"), R"(
Returns the decision literal of the given level.

Each level has exactly one decision literal, which implies other literals on
the same level.

Args:
    level: The decision level.
Returns:
    The decision literal.
)"_d)
        .def("is_false", &Assignment::is_false, py::arg("literal"), R"(
Check if the given literal is false.

Args:
    literal: The solver literal.
Returns:
    Whether the literal is false.
)"_d)
        .def("is_true", &Assignment::is_true, py::arg("literal"), R"(
Check if the given literal is true.

Args:
    literal: The solver literal.
Returns:
    Whether the literal is true.
)"_d)
        .def("is_free", &Assignment::is_free, py::arg("literal"), R"(
Check if the given literal is free.

Args:
    literal: The solver literal.
Returns:
    Whether the literal is free.
)"_d)
        .def("is_fixed", &Assignment::is_fixed, py::arg("literal"), R"(
Checks if the truth value of the literal is fixed.

Args:
    literal: The solver literal.
Returns:
    Whether the literal is fixed.
)"_d)
        .def("level", &Assignment::level, py::arg("literal"), R"(
Returns the decision level of the given literal.

The decision level indicates when the literal was assigned or implied during
the solving process.

Args:
    literal: The solver literal.
Returns:
    The decision level of the literal.
)"_d)
        .def("value", &Assignment::value, py::arg("literal"), R"(
Returns the truth value of the literal, or None if unassigned.

Args:
    literal: The solver literal.
Returns:
    The truth value of the literal.
)"_d)
        .def_property_readonly("thread_id", &Assignment::thread_id, R"(
Get the id of the thread to which the assignment belongs.

Thread ids are consecutive numbers starting with zero.
)"_d)
        .def_property_readonly("decision_level", &Assignment::decision_level, R"(
Get the current decision level.
)"_d)
        .def_property_readonly("has_conflict", &Assignment::has_conflict, R"(
Check if the assignment is conflicting.
)"_d)
        .def_property_readonly("is_total", &Assignment::is_total, R"(
Check if all literals in the assigment are assigned.
)"_d)
        .def_property_readonly("root_level", &Assignment::root_level, R"(
Get the current root level.

The root level is the lowest decision level that can be backtracked to.
Literals on the root level or below can be considered fixed by a propagator.
They include, for example, assumptions.
)"_d)
        .def_property_readonly("trail", &Assignment::trail, R"(
Get the trail of literals.
)"_d);

    py::class_<PropagateControl>(propagate, "PropagateControl", R"(
Class for controlling propagators.

This class provides methods for adding clauses, literals, and nogoods, as well
as managing watches and performing propagation.
)"_d)
        .def("add_clause", &PropagateControl::add_clause, py::arg("literals"), py::arg("tag") = false,
             py::arg("lock") = false, R"(
Add a clause to the solver.

Tagged clauses are deleted after the current solve call finishes while locked
clauses are exempt from the solvers clause deletion strategy.

See `propagate()` for how to handle the case that the function returns false.

Args:
    literals:
        A sequence of solver literals representing the clause.
    tag:
        Whether to tag the clause.
    lock:
        Whether to lock the clause.

Returns:
    Whether the clause could be integrated without conflict.
)"_d)
        .def("add_weight_constraint", &PropagateControl::add_weight_constraint, py::arg("literal"), py::arg("literals"),
             py::arg("bound"),
             py::arg("type") = clingo_weight_constraint_type_e::clingo_weight_constraint_type_equivalence,
             R"(
Add a weight constraint to the solver.

See `add_clause` for how to interpret the return value.

Args:
    literal:
        The literal associated with the constraint.
    literals:
        A sequence of (literal, weight) tuples.
    bound:
        The bound of the weight constraint.
    type:
        The type of the weight constraint.

Returns:
    Whether the weight constraint could be added without conflict.
)"_d)
        .def("add_literal", &PropagateControl::add_literal, py::arg("freeze") = true, R"(
Add a literal to the solver.

If the function is called in the context of a particular solver thread, the literal is only added to
that thread and deleted after the active solve call.

Args:
    freeze:
        Whether to freeze the literal.

Returns:
    A fresh solver literal.
)"_d)
        .def("add_nogood", &PropagateControl::add_nogood, py::arg("literals"), py::arg("tag") = false,
             py::arg("lock") = false, R"(
A shortcut for `add_clause([-literal for literal in literals], tag, lock)`.

Args:
    literals:
        A sequence of solver literals representing the nogood.
    tag:
        Whether to tag the nogood.
    lock:
        Whether to lock the nogood.

Returns:
    Whether the nogood could be integrated without conflict.
)"_d)
        .def("add_watch", &PropagateControl::add_watch, py::arg("literal"), R"(
Add a watch for the given solver literal.

Args:
    literal: The literal to watch.
)"_d)
        .def("has_watch", &PropagateControl::has_watch, py::arg("literal"), R"(
Check if a watch exists for the given solver literal.

Args:
    literal: The literal to check.

Returns:
    True if a watch exists for the literal, False otherwise.
)"_d)
        .def("propagate", &PropagateControl::propagate, R"(
Perform propagation in the solver.

If this function returns False, the propagator must add no further
clauses/literals and immediately return from the corresponding
`Propagator.init()`, `Propagator.propagate()` or `Propagator.check()` call.

Returns:
    True if propagation was successful, False otherwise.
)"_d)
        .def("remove_watch", &PropagateControl::remove_watch, py::arg("literal"), R"(
Remove the watch for the given literal.

This function has no effect if the literal is not watched.

Args:
    literal: The literal to remove the watch for.
)"_d);

    py::class_<PropagateInit, PropagateControl>(propagate, "PropagateInit", R"(
Class for initializing a propagator.

This class extends PropagateControl and additionally provides methods for freezing and looking up literals, adding
constraints, and configuring the propagator's behavior.
)"_d)
        .def("add_minimize", &PropagateInit::add_minimize, py::arg("literal"), py::arg("weight"),
             py::arg("priority") = 0,
             R"(
Add a weighted literal to minimize to the solver.

Args:
    literal:
        The literal to minimize.
    weight:
        The weight of the literal.
    priority:
        The priority of the literal.
)"_d)
        .def("freeze_literal", &PropagateInit::freeze_literal, py::arg("literal"), R"(
Freeze the given literal.

Frozen literals are exempt from simplification. This is important for literals
whose truth values a propagator has to track.

Args:
    literal:
        The literal to freeze.
)"_d)
        .def("solver_literal", &PropagateInit::solver_literal, py::arg("literal"), R"(
Map the given program literal to a solver literal.

Args:
    literal:
        The program literal to map.

Returns:
    The corresponding solver literal.
)"_d)
        .def_property_readonly("library", &PropagateInit::library, R"(
The library object managing symbols.
)"_d)
        .def_property_readonly("base", &PropagateInit::base, R"(
The base object to inspect the grounder's base.
)"_d)
        .def_property("check_mode", &PropagateInit::get_check_mode, &PropagateInit::set_check_mode, R"(
Get/set the check mode for the propagator.
)"_d)
        .def_property_readonly("number_of_threads", &PropagateInit::number_of_threads, R"(
The number of solver threads.
)"_d)
        .def_property("undo_mode", &PropagateInit::get_undo_mode, &PropagateInit::set_undo_mode, R"(
Get/set the undo mode for the propagator.
)"_d);

    py::class_<Propagator>(propagate, "Propagator", R"(
Interface for implementing propagators.

This class defines methods that can be implemented to create custom propagators
for use with the solver. They can be left empty to use their default
implementation.
)")
        .def(py::init<>())
        .def("init", &Propagator::init, py::arg("assignment"), py::arg("init"), R"(
Initialize the propagator.

This method is called once before each solving step. It is used to map
relevant program literals to solver literals, add watches for solver
literals, and initialize the propagator's internal state.

Args:
    assignment:
        The current assignment.
    init:
        The propagate init object for initializing the propagator.
)"_d)
        .def("attach", &Propagator::attach, py::arg("assignment"), py::arg("control"), R"(
Initialize solver thread with given id.

This function is called once for each solving thread before solving of the
current step is started.
It can be used to add thread-specific watches and literals, or
initialize thread-specific data structures.

Args:
    assignment:
        The current assignment.
    control:
        The propagate control object for managing propagation.
)"_d)
        .def("propagate", &Propagator::propagate, py::arg("assignment"), py::arg("control"), py::arg("changes"), R"(
Propagate given set of changes.

This method is called during propagation with a non-empty list of watched literals
that have been assigned truth values. A propagator should add clauses to
propagate literals and to implement its constraints.

Typical propagators add unit-resulting or conflicting constraints only.

Args:
    assignment:
        The current assignment.
    control:
        The propagate control object for managing propagation.
    changes:
        A list of literals that have changed.
)"_d)
        .def("undo", &Propagator::undo, py::arg("assignment"), py::arg("changes"), R"(
Undo previous assignments.

This method is called to undo previous assignments.

See also `PropagateInit.undo_mode`.

Args:
    assignment:
        The current assignment.
    changes:
        The literals whose assignment is undone.
)"_d)
        .def("check", &Propagator::check, py::arg("assignment"), py::arg("control"), R"(
Check if the current assignment is valid.

This method is called on propagation fixpoints or total assignments (see
`PropagateInit.check_mode`). A propagator should add clauses to implement its
constraints here.

Args:
    assignment:
        The current assignment.
    control:
        The propagate control object for managing propagation.
)"_d)
        .def("decide", &Propagator::decide, py::arg("assignment"), py::arg("fallback"), R"(
Make a decision on the next literal to assign.

This method is called to decide on a literal to be assigned next.

Args:
    assignment:
        The current assignment.
    fallback:
        The literal choosen by the solver's heuristic.

Returns:
    The literal to assign or 0 if no decision was made.
)"_d);
}

} // namespace PyClingo
