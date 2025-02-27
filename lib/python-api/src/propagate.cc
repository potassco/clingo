#include <clingo/propagate.h>

#include <utility>

#include "iterable.hh"
#include "propagate.hh"
#include "util.hh"

namespace Clingo::Python {

namespace {

auto to_rng(uint32_t size, py::slice const &slc) {
    return py::module::import("builtins").attr("range")(size)[slc];
}

class TrailView {
  public:
    TrailView(clingo_assignment_t *assignment, py::object rng) : assignment_{assignment}, rng_{std::move(rng)} {}

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
    clingo_assignment_t *assignment_;
    py::object rng_;
};

class Trail {
  public:
    Trail(clingo_assignment_t *assignment) : assignment_{assignment} {}

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
    clingo_assignment_t *assignment_;
};

class Assignment {
  public:
    Assignment(clingo_assignment_t *assignment) : assignment_(assignment) {}

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

    auto root_level() -> uint32_t {
        uint32_t level = 0;
        handle_error(clingo_assignment_root_level(assignment_, &level));
        return level;
    }

    auto trail() -> Trail { return Trail{assignment_}; }

  private:
    clingo_assignment_t *assignment_;
};

} // namespace

void register_propagate(pybind11::module &m) {
    using namespace Clingo::Python;
    auto propagate = m.def_submodule("propagate", R"(
TODO
)"_d);

    py::class_<TrailView>(propagate, "_TrailView", R"(
Provides access to a subrange of literals in the solver's trail.

This class implements the sequence interface.
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

This class implements the sequence interface.
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
Get the literals of the given levels.
)"_d)
        .def("__len__", &Assignment::size, R"(
TODO
)"_d);
}

} // namespace Clingo::Python
