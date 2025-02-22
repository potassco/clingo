#include <clingo/backend.h>

#include "backend.hh"
#include "iterable.hh" // IWYU pragma: keep
#include "util.hh"

namespace Clingo::Python {

// Backend

auto Backend::atom(std::optional<Symbol> symbol) -> clingo_atom_t {
    clingo_atom_t atom = 0;
    clingo_backend_add_atom(backend_, symbol ? c_cast(&*symbol) : nullptr, &atom);
    return atom;
}

void Backend::rule(AtomSpan head, LitSpan body, bool choice) {
    handle_error(clingo_backend_rule(backend_, choice, head.data(), head.size(), body.data(), body.size()));
}

void Backend::weight_rule(AtomSpan head, clingo_weight_t lower, WeightLitSpan body, bool choice) {
    handle_error(
        clingo_backend_weight_rule(backend_, choice, head.data(), head.size(), lower, body.data(), body.size()));
}

void Backend::minimize(WeightLitSpan literals, clingo_weight_t priority) {
    handle_error(clingo_backend_minimize(backend_, priority, literals.data(), literals.size()));
}

void Backend::project(AtomSpan atoms) {
    handle_error(clingo_backend_project(backend_, atoms.data(), atoms.size()));
}

void Backend::external(clingo_atom_t atom, clingo_external_type_e type) {
    handle_error(clingo_backend_external(backend_, atom, type));
}

void Backend::assume(LitSpan literals) {
    handle_error(clingo_backend_assume(backend_, literals.data(), literals.size()));
}

void Backend::heuristic(clingo_atom_t atom, clingo_heuristic_type_e type, int bias, unsigned priority,
                        LitSpan condition) {
    handle_error(clingo_backend_heuristic(backend_, atom, type, bias, priority, condition.data(), condition.size()));
}

void Backend::edge(int node_u, int node_v, LitSpan condition) {
    handle_error(clingo_backend_acyc_edge(backend_, node_u, node_v, condition.data(), condition.size()));
}

// BackendManager

auto BackendManager::enter() -> Backend {
    handle_error(clingo_control_backend(ctl_, &backend_));
    return Backend{backend_};
}

void BackendManager::exit([[maybe_unused]] std::optional<pybind11::type> const &type,
                          [[maybe_unused]] std::optional<pybind11::object> const &value,
                          [[maybe_unused]] std::optional<pybind11::object> const &traceback) {
    if (backend_ != nullptr) {
        handle_error(clingo_backend_close(backend_));
        backend_ = nullptr;
    }
}

void register_backend(pybind11::module &m) {
    using namespace Clingo::Python;

    auto backend = m.def_submodule("backend", R"(
Functions and classes to observe or add ground statements.

Examples
--------
The first example shows how to add a fact to a program:

```python
>>> from clingo.symbol import Function
>>> from clingo.control import Control
>>>
>>> ctl = Control()
>>>
>>> sym = Function("a")
>>> with ctl.backend as bck:
...     atm_a = bck.atom(sym)
...     bck.rule([atm_a])
...
>>> ctl.base[sym].is_fact
True
>>>
>>> print(ctl.solve(on_model=print))
a
SAT
```

The next example shows how to add theory atoms to a program:
)"_d);

    py::enum_<clingo_heuristic_type_e>(backend, "HeuristicType", R"doc(Available heuristic types.)doc")
        .value("Level", clingo_heuristic_type_level, R"doc(The level modifier.)doc")
        .value("Factor", clingo_heuristic_type_factor, R"doc(The factor modifier.)doc")
        .value("True_", clingo_heuristic_type_true, R"doc(The true modifier.)doc")
        .value("False_", clingo_heuristic_type_false, R"doc(The false modifier.)doc")
        .value("Init", clingo_heuristic_type_init, R"doc(The init modifier.)doc")
        .value("Sign", clingo_heuristic_type_sign, R"doc(The sign modifier.)doc");

    py::enum_<clingo_external_type_e>(backend, "ExternalType", R"doc(Available external types.)doc")
        .value("True_", clingo_external_type_true)
        .value("False_", clingo_external_type_false)
        .value("Free", clingo_external_type_free)
        .value("Release", clingo_external_type_release);

    py::class_<Backend>(backend, "Backend", R"(
Provides an interface to extend a logic program.

The Backend class allows for extending logic programs through a low-level
interface. It provides methods to add various types of rules, set external
atoms, specify heuristics, and define optimization statements.

See Also:
    clingo.control.Control.backend
)"_d)
        .def("atom", &Backend::atom, py::arg("symbol") = std::nullopt, R"(
Return a fresh program atom or the atom associated with the given symbol.

If the given symbol does not exist in the atom base, it is added first. Such
atoms will be used in subequents calls to ground for instantiation.

Args:
    symbol: The symbol associated with the atom.

Returns: The program atom representing the atom.
)"_d)
        .def("rule", &Backend::rule, py::arg("head"), py::arg("body") = LitSpan{}, py::arg("choice") = false, R"(
Add a rule to the program.

The rule can be a disjunctive rule or a choice rule, depending on the `choice`
parameter. The head and body of the rule are specified as sequences of
literals.

When `choice` is false, different types of rules are created based on the
length of the head:
- An empty head forms an integrity constraint.
- A single literal in the head forms a normal rule.
- Multiple literals in the head form a disjunctive rule.

Args:
	head: Sequence of literals in the rule head.
	body: Sequence of literals in the rule body (default: []).
	choice: If True, adds a choice rule; otherwise, a disjunctive rule (default: False).
)"_d)
        .def("weight_rule", &Backend::weight_rule, py::arg("head"), py::arg("lower_bound"), py::arg("body"),
             py::arg("choice") = false, R"(
Add a weight rule to the program.

Adds a weight rule, where the body is a weight constraint. The head is either a
disjunction ro choice (see Backend.rule for more details).

A weight constraint is satisfied if the sum of weights of the true literals
meets or exceeds the lower bound.

Args:
	head: Sequence of literals in the rule head.
	lower_bound: The lower bound of the weight constraint.
	body: Sequence of (literal, weight) tuples forming the weight constraint.
	choice: If True, adds a choice rule; otherwise, a disjunctive rule (default: False).
)"_d)
        .def("assume", &Backend::assume, py::arg("literals"), R"(
Add an assumption directive to the solver.

Specifies literals to be assumed true or false for the next solving call.
Positive literals are treated as true, while negative literals are treated
as false. These assumptions apply only to the next solve call.

Args:
    literals: Sequence of program literals to assume.
)"_d)
        .def("edge", &Backend::edge, py::arg("node_u"), py::arg("node_v"), py::arg("condtition"), R"(
Add an edge directive.

Adds an edge from node_u to node_v to the graph. The edge is subject to the
specified condition.

Args:
    node_u: The start node of the edge.
    node_v: The end node of the edge.
    condition: Sequence of literals representing the edge condition.
)"_d)
        .def("external", &Backend::external, py::arg("atom"), py::arg("type"), R"(
Add an external directive.

Declares an atom as external and sets its truth value according to the
specified type. External atoms can be used as assumptions or for incremental
solving. The special value `ExternalType.Release` can be used to permanently

set an external atom to false.

Args:
    atom: The external atom (must be a positive literal).
    type: The type determining the truth value of the atom.
)"_d)
        .def("heuristic", &Backend::heuristic, py::arg("atom"), py::arg("type"), py::arg("weight"),
             py::arg("priority") = 0, py::arg("condition") = LitSpan{}, R"(
Add a heuristic directive for an atom.

Adds a heuristic directive for the given atom, influencing the solver's search
process. The directive's effect depends on its type, weight, and priority. The
condition, if provided, determines when the heuristic should be applied.

Args:
	atom: The atom to which the heuristic applies.
	type: The type of the heuristic.
	weight: The weight of the heuristic.
	priority: The priority of the heuristic (default: 0).
	condition: Sequence of literals representing the condition (default: []).
)"_d)
        .def("minimize", &Backend::minimize, py::arg("literals"), py::arg("priority") = 0, R"(
Add a minimize constraint to the program.

Adds a minimize constraint, which instructs the solver to minimize the sum of
weights of true literals in the given sequence. Multiple minimize constraints
with different priorities can be added, with lower priority values considered
more important.

Args:
	literals: Sequence of (literal, weight) tuples to minimize.
	priority: Priority of the constraint (default: 0).
)"_d)
        .def("project", &Backend::project, py::arg("atoms"), R"(
Add a projection directive to the program.

Specifies which atoms should be considered in stable models. When projection
is used, stable models are treated as equivalent if they agree on the truth
values of projected atoms, regardless of other atoms in the model.

Args:
	atoms: Sequence of atoms to project on.
)"_d);

    py::class_<BackendManager>(backend, "BackendManager", R"(
A context manager to initialize and finalize a backend.
)"_d)
        .def("__enter__", &BackendManager::enter, "Initialize backend the backend.")
        .def("__exit__", &BackendManager::exit, py::arg("type"), py::arg("value"), py::arg("traceback"),
             "Finalize the backend.");
}

} // namespace Clingo::Python
