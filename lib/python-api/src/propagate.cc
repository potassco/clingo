#include <clingo/propagate.h>

#include <utility>

#include "base.hh"
#include "iterable.hh"
#include "propagate.hh"
#include "util.hh"

namespace Clingo::Python {

namespace {

auto to_rng(uint32_t size, py::slice const &slc) {
    return py::module::import("builtins").attr("range")(size)[slc];
}

} // namespace

class TrailView {
  public:
    TrailView(clingo_assignment_t const *assignment, py::object rng) : assignment_{assignment}, rng_{std::move(rng)} {}

    auto slice(py::slice const &slc) -> Sequence<clingo_literal_t> {
        return py::cast(TrailView{assignment_, rng_[slc]});
    }

    auto at(uint32_t index) -> clingo_literal_t {
        clingo_literal_t lit = 0;
        auto offset = rng_[py::int_{index}].cast<uint32_t>();
        handle_error(clingo_assignment_trail_at(assignment_, offset, &lit));
        return lit;
    }
    auto size() -> uint32_t { return py::len(rng_); }

  private:
    clingo_assignment_t const *assignment_;
    py::object rng_;
};

class Trail {
  public:
    Trail(clingo_assignment_t const *assignment) : assignment_{assignment} {}

    auto slice(py::slice const &slc) -> Sequence<clingo_literal_t> {
        return py::cast(TrailView{assignment_, to_rng(size(), slc)});
    }

    auto at(uint32_t index) -> clingo_literal_t {
        clingo_literal_t lit = 0;
        handle_error(clingo_assignment_trail_at(assignment_, index, &lit));
        return lit;
    }

    auto size() -> uint32_t {
        uint32_t size = 0;
        handle_error(clingo_assignment_trail_size(assignment_, &size));
        return size;
    }

    auto begin(uint32_t level) -> uint32_t {
        uint32_t offset = 0;
        handle_error(clingo_assignment_trail_begin(assignment_, level, &offset));
        return offset;
    }

    auto end(uint32_t level) -> uint32_t {
        uint32_t offset = 0;
        handle_error(clingo_assignment_trail_end(assignment_, level, &offset));
        return offset;
    }

    auto level(uint32_t level) -> Sequence<clingo_literal_t> { return slice(py::slice(begin(level), end(level), 1)); }

  private:
    clingo_assignment_t const *assignment_;
};

class Assignment {
  public:
    Assignment(clingo_assignment_t const *assignment) : assignment_(assignment) {}

    auto size() -> size_t {
        size_t size = 0;
        handle_error(clingo_assignment_size(assignment_, &size));
        return size;
    }

    auto at(size_t size) -> clingo_literal_t {
        clingo_literal_t lit = 0;
        handle_error(clingo_assignment_at(assignment_, size, &lit));
        return lit;
    }

    auto decision(uint32_t level) -> clingo_literal_t {
        clingo_literal_t lit = 0;
        handle_error(clingo_assignment_decision(assignment_, level, &lit));
        return lit;
    }

    auto decision_level() -> uint32_t {
        uint32_t level = 0;
        handle_error(clingo_assignment_decision_level(assignment_, &level));
        return level;
    }

    auto has_conflict() -> bool {
        auto res = false;
        handle_error(clingo_assignment_has_conflict(assignment_, &res));
        return res;
    }

    auto has_literal(clingo_literal_t lit) -> bool {
        auto res = false;
        handle_error(clingo_assignment_has_literal(assignment_, lit, &res));
        return res;
    }

    auto is_false(clingo_literal_t lit) -> bool {
        auto res = false;
        handle_error(clingo_assignment_is_false(assignment_, lit, &res));
        return res;
    }

    auto is_fixed(clingo_literal_t lit) -> bool {
        auto res = false;
        handle_error(clingo_assignment_is_fixed(assignment_, lit, &res));
        return res;
    }

    auto is_free(clingo_literal_t lit) -> bool {
        clingo_truth_value_t res = 0;
        handle_error(clingo_assignment_truth_value(assignment_, lit, &res));
        return res == clingo_truth_value_free;
    }

    auto is_total() -> bool {
        auto res = false;
        handle_error(clingo_assignment_is_total(assignment_, &res));
        return res;
    }

    auto is_true(clingo_literal_t lit) -> bool {
        auto res = false;
        handle_error(clingo_assignment_is_true(assignment_, lit, &res));
        return res;
    }

    auto level(clingo_literal_t lit) -> uint32_t {
        uint32_t level = 0;
        handle_error(clingo_assignment_level(assignment_, lit, &level));
        return level;
    }

    auto value(clingo_literal_t lit) -> std::optional<bool> {
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

    auto root_level() -> uint32_t {
        uint32_t level = 0;
        handle_error(clingo_assignment_root_level(assignment_, &level));
        return level;
    }

    auto trail() -> Trail { return Trail{assignment_}; }

  private:
    clingo_assignment_t const *assignment_;
};

class PropagateInit {
  public:
    PropagateInit(clingo_propagate_init_t *init) : init_{init} {}

    auto base() -> Base {
        clingo_base_t const *base = nullptr;
        handle_error(clingo_propagate_init_base(init_, &base));
        return {base};
    }

  private:
    clingo_propagate_init_t *init_;
};

class PropagateControl {
  public:
    PropagateControl(clingo_propagate_control_t *ctl) : ctl_{ctl} {}

    void add_watch(clingo_literal_t lit) { handle_error(clingo_propagate_control_add_watch(ctl_, lit)); }

  private:
    clingo_propagate_control_t *ctl_;
};

void Propagator::init(PropagateInit &init) {
    PYBIND11_OVERRIDE_NAME(void, Propagator, "init", no_op, init);
}
void Propagator::propagate(PropagateControl &ctl, LitSpan changes) {
    PYBIND11_OVERRIDE_NAME(void, Propagator, "propagate", no_op, ctl, changes);
}
void Propagator::undo(uint32_t thread_id, Assignment &assignment, LitSpan changes) {
    PYBIND11_OVERRIDE_NAME(void, Propagator, "undo", no_op, thread_id, assignment, changes);
}
void Propagator::check(PropagateControl &ctl) {
    PYBIND11_OVERRIDE_NAME(void, Propagator, "check", no_op, ctl);
}
auto Propagator::decide(uint32_t thread_id, Assignment &assignment, clingo_literal_t lit) -> clingo_literal_t {
    PYBIND11_OVERRIDE_NAME(clingo_literal_t, Propagator, "decide", decide_, thread_id, assignment, lit);
}

auto Propagator::decide_([[maybe_unused]] uint32_t thread_id, [[maybe_unused]] Assignment &assignment,
                         [[maybe_unused]] clingo_literal_t lit) -> clingo_literal_t {
    static_cast<void>(this);
    return lit;
}

void register_propagator(clingo_control_t *ctl, PropagatorData &data) {
    // propagator without heuristic
    static constexpr auto c_prop = clingo_propagator_t{
        [](clingo_propagate_init_t *init, void *data) -> clingo_result_t {
            auto &[self, exception] = *static_cast<PropagatorData *>(data);
            CLINGO_TRY {
                auto py_init = PropagateInit{init};
                self->init(py_init);
            }
            CLINGO_CATCH(*exception);
        },
        [](clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size,
           void *data) -> clingo_result_t {
            auto &[self, exception] = *static_cast<PropagatorData *>(data);
            CLINGO_TRY {
                auto py_ctl = PropagateControl{control};
                self->propagate(py_ctl, LitSpan{changes, size});
            }
            CLINGO_CATCH(*exception);
        },
        [](clingo_propagate_control_t const *control, clingo_literal_t const *changes, size_t size, void *data) {
            try {
                auto &[self, exception] = *static_cast<PropagatorData *>(data);
                uint32_t thread_id = 0;
                handle_error(clingo_propagate_control_thread_id(control, &thread_id));
                clingo_assignment_t const *assignment = nullptr;
                handle_error(clingo_propagate_control_assignment(control, &assignment));
                auto py_assignment = Assignment(assignment);
                self->undo(thread_id, py_assignment, LitSpan{changes, size});
            } catch (std::exception const &e) {
                printf("panic: %s\n", e.what());
                std::abort();
            }
        },
        [](clingo_propagate_control_t *control, void *data) -> clingo_result_t {
            auto &[self, exception] = *static_cast<PropagatorData *>(data);
            CLINGO_TRY {
                auto py_ctl = PropagateControl{control};
                self->check(py_ctl);
            }
            CLINGO_CATCH(*exception);
        },
        nullptr,
    };
    // propagator with heuristic
    static constexpr auto c_heu =
        clingo_propagator_t{c_prop.init, c_prop.propagate, c_prop.undo, c_prop.check,
                            [](clingo_id_t thread_id, clingo_assignment_t const *assignment, clingo_literal_t fallback,
                               void *data, clingo_literal_t *decision) -> clingo_result_t {
                                auto &[self, exception] = *static_cast<PropagatorData *>(data);
                                CLINGO_TRY {
                                    auto py_assignment = Assignment{assignment};
                                    *decision = self->decide(thread_id, py_assignment, fallback);
                                }
                                CLINGO_CATCH(*exception);
                            }};
    auto has_heu = pybind11::get_override(static_cast<const Propagator *>(data.first), "decide");
    handle_error(clingo_control_register_propagator(ctl, has_heu ? &c_heu : &c_prop, static_cast<void *>(&data)));
}

void register_propagate(pybind11::module &m) {
    using namespace Clingo::Python;
    auto propagate = m.def_submodule("propagate", R"(
Functions and classes to implement custom propagators.

# Example

```python
>>> from clingo.core import Library
>>> from clingo.symbol import Function
>>> from clingo.propagate import Propagator
>>> from clingo.control import Control
>>>
>>> LIB = Library()
>>>
>>> class AIFFB(Propagator):
...     # add watches for atoms `a` and `b`
...     def init(self, init):
...         # get program literals for atoms `a` and `b`
...         plit_a = init.base[Function(LIB, "a")].literal
...         plit_b = init.base[Function(LIB, "b")].literal
...         # get solver literals for program literals `a` and `b`
...         self.slit_a = init.solver_literal(plit_a)
...         self.slit_b = init.solver_literal(plit_b)
...         # add watches for solver literals `a` and `b`
...         init.add_watch(self.slit_a)
...         init.add_watch(self.slit_b)
...     # propagate solver literals `a` and `b`
...     def propagate(self, ctl, changes):
...         # if `a` is true imply `b`
...         if self.slit_a in changes:
...             assert ctl.assignment.is_true(self.slit_a)
...             ctl.add_clause([-self.slit_a, self.slit_b])
...         # if `b` is true imply `a`
...         if self.slit_b in changes:
...             assert ctl.assignment.is_true(self.slit_b)
...             ctl.add_clause([-self.slit_b, self.slit_a])
...
>>> ctl = Control(LIB, ["0"])
>>> ctl.register_propagator(AIFFB())
>>> ctl.add("1 { a; b }.")
>>> ctl.ground()
>>> print(ctl.solve(on_model=print))
a b
SAT
```
)"_d);

    py::class_<TrailView>(propagate, "_TrailView", R"(
Provides access to a subrange of literals in the solver's trail.

Implements `Sequence[int]` to access the solver literals in the view.
)"_d)
        .def("__len__", &TrailView::size, R"(
Get the number of literals in the view.
)"_d)
        .def("__getitem__", &TrailView::at, py::arg("index"), R"(
Get the literal at the given index.
)"_d)
        .def("__getitem__", &TrailView::slice, py::arg("slice"), R"(
Slice the view.
)"_d);

    py::class_<Trail>(propagate, "Trail", R"(
Provides access to literals in the solver's trail.

The trail represents the sequence of literals assigned during the solving
process. It is structured by decision levels, where each level contains
literals assigned due to implications from the decision literal at that level.

The literals within each level are ordered by implication, reflecting the
logical dependencies between them as determined by the solver's propagation and
learning mechanisms. The decision literal for each level is placed at the
beginning of its respective sequence.

Implements `Sequence[int]` to access the solver literals in the trail.
)"_d)
        .def("__len__", &Trail::size, R"(
Get the number of literals in the trail.
)"_d)
        .def("__getitem__", &Trail::at, py::arg("index"), R"(
Get the literal at the given index.
)"_d)
        .def("__getitem__", &Trail::slice, py::arg("slice"), R"(
Slice the trail.
)"_d)
        .def("level", &Trail::level, py::arg("level"), R"(
Get the literals assigned at the given decision level.

This sequence includes all literals assigned at this level in implication
order, with the decision literal at the beginning of the sequence.

Args:
    level: A decision level.
Returns:
    A sequence of literals.
)"_d)
        .def("begin", &Trail::begin, py::arg("level"), R"(
Get the index of the first literal on the given level.

This also corresponds to the decision literal for that level.

Args:
    level: A decision level.
Returns:
    The index of the decision literal.
)"_d)
        .def("end", &Trail::end, py::arg("level"), R"(
Get the index after the last literal on the given level.

Args:
    level: A decision level.
Returns:
    The index after the last literal.
)"_d);

    py::class_<Assignment>(propagate, "Assignment", R"(
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
)"_d)
        .def("__len__", &Assignment::size, R"(
Get the number of literals in the assignment.
)"_d)
        .def("__getitem__", &Assignment::at, py::arg("index"), R"(
Get the literal at the given index.
)"_d)
        .def("__contains__", &Assignment::has_literal, py::arg("literal"), R"(
Determine if the given literal is contained in the assignment.

Args:
    literal: The solver literal.
Returns:
    Whether the literal is valid.
)"_d)
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

    py::class_<PropagateInit>(propagate, "PropagateInit", R"(
TODO
)"_d)
        .def_property_readonly("base", &PropagateInit::base, R"(
TODO
)"_d);

    py::class_<PropagateControl>(propagate, "PropagateControl", R"(
TODO
)"_d)
        .def("add_watch", &PropagateControl::add_watch, py::arg("literal"), R"(
TODO
)"_d);

    py::class_<Propagator>(propagate, "Propagator", R"(
TODO
)")
        .def(py::init<>())
        .def("init", &Propagator::init, py::arg("init"), R"(
TODO
)"_d)
        .def("propagate", &Propagator::propagate, py::arg("control"), py::arg("changes"), R"(
TODO
)"_d)
        .def("undo", &Propagator::undo, py::arg("thread_id"), py::arg("assignment"), py::arg("changes"), R"(
TODO
)"_d)
        .def("check", &Propagator::check, py::arg("control"), R"(
TODO
)"_d)
        .def("decide", &Propagator::decide, py::arg("thread_id"), py::arg("assignment"), py::arg("fallback"), R"(
TODO
)"_d);
}

} // namespace Clingo::Python
