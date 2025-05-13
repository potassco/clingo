#include <clingo/backend.h>
#include <clingo/observe.h>

#include "backend.hh"
#include "base.hh"
#include "iterable.hh" // IWYU pragma: keep
#include "util.hh"

namespace PyClingo {

// Backend

void Observer::init_program(bool incremental) {
    PYBIND11_OVERRIDE_NAME(void, Observer, "init_program", no_op, incremental);
}

void Observer::begin_step() {
    PYBIND11_OVERRIDE_NAME(void, Observer, "begin_step", no_op);
}

void Observer::end_step(Base base) {
    PYBIND11_OVERRIDE_NAME(void, Observer, "end_step", no_op, base);
}

void Observer::rule(AtomSpan head, LitSpan body, bool choice) {
    PYBIND11_OVERRIDE_NAME(void, Observer, "rule", no_op, head, body, choice);
}

void Observer::weight_rule(AtomSpan head, clingo_weight_t lower, WeightLitSpan body, bool choice) {
    PYBIND11_OVERRIDE_NAME(void, Observer, "weight_rule", no_op, head, lower, body, choice);
}

void Observer::minimize(WeightLitSpan literals, clingo_weight_t priority) {
    PYBIND11_OVERRIDE_NAME(void, Observer, "minimize", no_op, literals, priority);
}

void Observer::project(AtomSpan atoms) {
    PYBIND11_OVERRIDE_NAME(void, Observer, "project", no_op, atoms);
}

void Observer::external(clingo_atom_t atom, clingo_external_type_e type) {
    PYBIND11_OVERRIDE_NAME(void, Observer, "external", no_op, atom, type);
}

void Observer::assume(LitSpan literals) {
    PYBIND11_OVERRIDE_NAME(void, Observer, "assume", no_op, literals);
}

void Observer::heuristic(clingo_atom_t atom, clingo_heuristic_type_e type, int bias, unsigned priority,
                         LitSpan condition) {
    PYBIND11_OVERRIDE_NAME(void, Observer, "heuristic", no_op, atom, type, bias, priority, condition);
}

void Observer::edge(int node_u, int node_v, LitSpan condition) {
    PYBIND11_OVERRIDE_NAME(void, Observer, "edge", no_op, node_u, node_v, condition);
}

void Observer::observe(clingo_control_t *ctl, bool preprocess) {
    static constexpr auto g_obs = clingo_observer_t{
        [](bool incremental, void *data) -> bool {
            CLINGO_TRY {
                static_cast<Observer *>(data)->init_program(incremental);
            }
            CLINGO_CATCH;
        },
        [](void *data) -> bool {
            CLINGO_TRY {
                static_cast<Observer *>(data)->begin_step();
            }
            CLINGO_CATCH;
        },
        [](clingo_base_t const *base, void *data) -> bool {
            CLINGO_TRY {
                static_cast<Observer *>(data)->end_step(Base{base});
            }
            CLINGO_CATCH;
        },
        [](bool choice, clingo_atom_t const *head, size_t head_size, clingo_literal_t const *body, size_t body_size,
           void *data) -> bool {
            CLINGO_TRY {
                static_cast<Observer *>(data)->rule(std::span{head, head_size}, std::span{body, body_size}, choice);
            }
            CLINGO_CATCH;
        },
        [](bool choice, clingo_atom_t const *head, size_t head_size, clingo_weight_t lower,
           clingo_weighted_literal_t const *body, size_t body_size, void *data) -> bool {
            CLINGO_TRY {
                static_cast<Observer *>(data)->weight_rule(std::span{head, head_size}, lower,
                                                           std::span{body, body_size}, choice);
            }
            CLINGO_CATCH;
        },
        [](clingo_weight_t priority, clingo_weighted_literal_t const *body, size_t body_size, void *data) -> bool {
            CLINGO_TRY {
                static_cast<Observer *>(data)->minimize(std::span{body, body_size}, priority);
            }
            CLINGO_CATCH;
        },
        [](clingo_atom_t const *atoms, size_t size, void *data) -> bool {
            CLINGO_TRY {
                static_cast<Observer *>(data)->project(std::span{atoms, size});
            }
            CLINGO_CATCH;
        },
        [](clingo_atom_t atom, clingo_external_type_t type, void *data) -> bool {
            CLINGO_TRY {
                static_cast<Observer *>(data)->external(atom, static_cast<clingo_external_type_e>(type));
            }
            CLINGO_CATCH;
        },
        [](clingo_literal_t const *literals, size_t size, void *data) -> bool {
            CLINGO_TRY {
                static_cast<Observer *>(data)->assume(std::span{literals, size});
            }
            CLINGO_CATCH;
        },
        [](clingo_atom_t atom, clingo_heuristic_type_t type, int bias, unsigned priority,
           clingo_literal_t const *condition, size_t size, void *data) -> bool {
            CLINGO_TRY {
                static_cast<Observer *>(data)->heuristic(atom, static_cast<clingo_heuristic_type_e>(type), bias,
                                                         priority, std::span{condition, size});
            }
            CLINGO_CATCH;
        },
        [](int node_u, int node_v, clingo_literal_t const *condition, size_t size, void *data) -> bool {
            CLINGO_TRY {
                static_cast<Observer *>(data)->edge(node_u, node_v, std::span{condition, size});
            }
            CLINGO_CATCH;
        },
    };
    handle_error(clingo_control_observe(ctl, &g_obs, static_cast<void *>(this), preprocess));
}

auto Backend::atom(std::optional<Symbol> symbol) -> clingo_atom_t {
    clingo_atom_t atom = 0;
    handle_error(clingo_backend_add_atom(backend_, symbol ? c_cast(&*symbol) : nullptr, &atom));
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

auto Backend::theory_number(int number) -> clingo_id_t {
    clingo_id_t id = 0;
    handle_error(clingo_backend_theory_term_number(backend_, number, &id));
    return id;
}

auto Backend::theory_string(std::string_view string) -> clingo_id_t {
    clingo_id_t id = 0;
    handle_error(clingo_backend_theory_term_string(backend_, string.data(), string.size(), &id));
    return id;
}

auto Backend::theory_symbol(Symbol symbol) -> clingo_id_t {
    clingo_id_t id = 0;
    handle_error(clingo_backend_theory_term_symbol(backend_, *c_cast(&symbol), &id));
    return id;
}

auto Backend::theory_sequence(clingo_theory_sequence_type_e type, IdSpan elements) -> clingo_id_t {
    clingo_id_t id = 0;
    handle_error(clingo_backend_theory_term_sequence(backend_, type, elements.data(), elements.size(), &id));
    return id;
}

auto Backend::theory_function(std::string_view name, IdSpan elements) -> clingo_id_t {
    clingo_id_t id = 0;
    handle_error(
        clingo_backend_theory_term_function(backend_, name.data(), name.size(), elements.data(), elements.size(), &id));
    return id;
}

auto Backend::theory_element(IdSpan tuple, LitSpan condition) -> clingo_id_t {
    clingo_id_t id = 0;
    handle_error(
        clingo_backend_theory_element(backend_, tuple.data(), tuple.size(), condition.data(), condition.size(), &id));
    return id;
}

auto Backend::theory_atom(std::optional<clingo_atom_t> atom, Symbol name, IdSpan elements,
                          std::optional<std::pair<std::string, clingo_id_t>> const &guard) -> clingo_atom_t {
    clingo_atom_t res = 0;
    auto op = clingo_string_t{guard ? guard->first.data() : nullptr, guard ? guard->first.size() : 0};
    handle_error(clingo_backend_theory_atom(backend_, *c_cast(&name), elements.data(), elements.size(),
                                            guard ? &op : nullptr, guard ? guard->second : 0, atom ? &*atom : nullptr,
                                            &res));
    return res;
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
    using namespace PyClingo;

    auto backend = m.def_submodule("backend", R"(
Functions and classes to observe or add ground statements.

Examples
--------
The first example shows how to add a fact to a program:

```python
>>> from clingo.core import Library
>>> from clingo.symbol import Function
>>> from clingo.control import Control
...
>>> lib = Library()
>>> ctl = Control(lib)
>>> sym = Function(lib, "a")
>>> with ctl.backend as bck:
...     atm_a = bck.atom(sym)
...     bck.rule([atm_a])
>>> ctl.base.is_fact(ctl.base[sym].literal)
True
>>> with ctl.solve(on_model=print) as hnd:
...     hnd.get()
a
SAT
```

The next example shows how to add theory atoms to a program:
```python
>>> from clingo.core import Library
>>> from clingo.symbol import Function
>>> from clingo.control import Control
...
>>> lib = Library()
>>> ctl = Control(lib)
...
>>> with ctl.backend as bck:
...     n = Function(lib, "p")
...     a = bck.theory_string("a")
...     o = bck.theory_number(1)
...     f = bck.theory_function("f", [a, o])
...     e = bck.theory_element([f], [])
...     bck.theory_atom(0, n, [e])
...
>>> print(ctl.base.theory[0])
&p { f(a,1) }
```
)"_d);

    py::enum_<clingo_heuristic_type_e>(backend, "HeuristicType", R"doc(Available heuristic types.)doc")
        .value("Level", clingo_heuristic_type_level, R"doc(The level modifier.)doc")
        .value("Factor", clingo_heuristic_type_factor, R"doc(The factor modifier.)doc")
        .value("True_", clingo_heuristic_type_true, R"doc(The true modifier.)doc")
        .value("False_", clingo_heuristic_type_false, R"doc(The false modifier.)doc")
        .value("Init", clingo_heuristic_type_init, R"doc(The init modifier.)doc")
        .value("Sign", clingo_heuristic_type_sign, R"doc(The sign modifier.)doc");

    py::enum_<clingo_external_type_e>(backend, "ExternalType", R"doc(Available external types.)doc")
        .value("True_", clingo_external_type_true, R"(Make an external atom true.)")
        .value("False_", clingo_external_type_false, R"(Make an external atom false.)")
        .value("Free", clingo_external_type_free, R"(Make an external atom a choice.)")
        .value("Release", clingo_external_type_release, R"(Release an external atom.)");

    py::enum_<clingo_theory_sequence_type_e>(backend, "TheorySequenceType", R"(Available theory sequence types.)")
        .value("Tuple", clingo_theory_sequence_type_tuple, R"(Sequences enclosed in parentheses.)")
        .value("List", clingo_theory_sequence_type_list, R"(Sequences enclosed in brackets.)")
        .value("Set", clingo_theory_sequence_type_set, R"(Sequences enclosed in braces.)");

    py::class_<Observer>(backend, "Observer", R"(
ABC to inspect aspif directives.

Not all functions of the interface have to be implemented and can be omitted if
not needed.

See Also:
    `clingo.control.Control.observe`
)")
        .def(py::init<>())
        .def("init_program", &Observer::init_program, py::arg("incremental"), R"(
The first directive in a program.

The parameter `incremental` indicates whether the program can consist of more
than one step.

Args:
	incremental: Whether the program is incremental.
)"_d)
        .def("begin_step", &Observer::begin_step, R"(
Called at the beginning of a step.
)"_d)
        .def("end_step", &Observer::end_step, py::arg("base"), R"(
Called at the end of a step.
)"_d)
        .def("rule", &Observer::rule, py::arg("head"), py::arg("body"), py::arg("choice"), R"(
Called for rules in the program.

See also `Backend.rule`.

Args:
	head: Sequence of literals in the rule head.
	body: Sequence of literals in the rule body (default: []).
	choice: If True, adds a choice rule; otherwise, a disjunctive rule (default: False).
)"_d)
        .def("weight_rule", &Observer::weight_rule, py::arg("head"), py::arg("lower_bound"), py::arg("body"),
             py::arg("choice"), R"(
Called for weight rules in the program.

See also `Backend.weight_rule`.

Args:
	head: Sequence of literals in the rule head.
	lower_bound: The lower bound of the weight constraint.
	body: Sequence of (literal, weight) tuples forming the weight constraint.
	choice: If True, adds a choice rule; otherwise, a disjunctive rule (default: False).
)"_d)
        .def("assume", &Observer::assume, py::arg("literals"), R"(
Called for assumptions in the solver.

See also `Backend.assume`.

Args:
    literals: Sequence of program literals to assume.
)"_d)
        .def("edge", &Observer::edge, py::arg("node_u"), py::arg("node_v"), py::arg("condition"), R"(
Called for edge directives in the program.

See also `Backend.edge`.

Args:
    node_u: The start node of the edge.
    node_v: The end node of the edge.
    condition: Sequence of literals representing the edge condition.
)"_d)
        .def("external", &Observer::external, py::arg("atom"), py::arg("type"), R"(
Called for external directives in the program.

See also `Backend.external`.

Args:
    atom: The external atom (must be a positive literal).
    type: The type determining the truth value of the atom.
)"_d)
        .def("heuristic", &Observer::heuristic, py::arg("atom"), py::arg("type"), py::arg("weight"),
             py::arg("priority"), py::arg("condition"), R"(
Called for heuristic directives in the program.

See also `Backend.heuristic`.

Args:
	atom: The atom to which the heuristic applies.
	type: The type of the heuristic.
	weight: The weight of the heuristic.
	priority: The priority of the heuristic (default: 0).
	condition: Sequence of literals representing the condition (default: []).
)"_d)
        .def("minimize", &Observer::minimize, py::arg("literals"), py::arg("priority"), R"(
Called for minimize constraints in the program.

See also `Backend.minimize`.

Args:
	literals: Sequence of (literal, weight) tuples to minimize.
	priority: Priority of the constraint (default: 0).
)"_d)
        .def("project", &Observer::project, py::arg("atoms"), R"(
Called for projection directives in the program.

See also `Backend.project`.

Args:
	atoms: Sequence of atoms to project on.
)"_d);

    py::class_<Backend>(backend, "Backend", R"(
Provides an interface to extend a logic program.

This class offers methods to add various types of rules, set external atoms,
specify heuristics, define optimization statements, and extend the underlying
theory. It allows for low-level manipulation of logic programs.

See Also:
	clingo.control.Control.backend
)"_d)
        .def("atom", &Backend::atom, py::arg("symbol") = std::nullopt, R"(
Return a fresh program atom or the atom associated with the given symbol.

If the given symbol does not exist in the atom base, it is added first. These
atoms will be used in subsequent calls to ground for instantiation.

Args:
	symbol: The symbol associated with the atom.

Returns:
	The program atom representing the atom.
)"_d)
        .def("rule", &Backend::rule, py::arg("head"), py::arg("body") = LitSpan{}, py::arg("choice") = false, R"(
Add a rule to the program.

Creates a disjunctive or choice rule based on the `choice` parameter. The
head and body are specified as sequences of literals.

Rule types based on head length (when choice is False):
- Empty head: Integrity constraint
- Single literal head: Normal rule
- Multiple literals head: Disjunctive rule

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
        .def("edge", &Backend::edge, py::arg("node_u"), py::arg("node_v"), py::arg("condition"), R"(
Add an edge directive.

Adds an edge from `node_u` to `node_v` to the graph. The edge is subject to the
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
)"_d)
        .def("theory_number", &Backend::theory_number, py::arg("number"), R"(
Create a numeric theory term.

The function creates a numeric theory term with the given value and return its
unique identifier. This id can be used in other theory-related methods to
reference this term.

Args:
    number: The numeric value of the theory term to create.

Returns:
	The unique id of the created theory term.
)"_d)
        .def("theory_string", &Backend::theory_string, py::arg("string"), R"(
Create a string theory term.

The method creates a string theory term with the given value and returns its
unique identifier. This id can be used in other theory-related methods to
reference this term.

Args:
	string: The string value of the theory term to create.

Returns:
	The unique id of the created string theory term.
)"_d)
        .def("theory_symbol", &Backend::theory_symbol, py::arg("symbol"), R"(
Convert a symbol into a theory term.

The method converts the given symbol into a theory term and returns its unique
identifier. This id can be used in other theory-related methods to reference
this term.

Args:
	symbol: The symbol to create the theory term from.

Returns:
	The unique id of the created theory term.
)"_d)
        .def("theory_sequence", &Backend::theory_sequence, py::arg("type"), py::arg("elements"), R"(
Create a sequence theory term.

The method creates a sequence theory term of the specified type, containing the
given elements, and returns its unique identifier. This id can be used in other
theory-related methods to reference this term.

Args:
	type: The type of the sequence (e.g., tuple, list, set).
	elements: A sequence of ids representing theory terms.

Returns:
	The unique id of the created sequence theory term.
)"_d)
        .def("theory_function", &Backend::theory_function, py::arg("name"), py::arg("arguments"), R"(
Create a function theory term.

The method creates a function theory term with the given name and arguments,
and returns its unique identifier. This id can be used in other theory-related
methods to reference this term.

Args:
	name: The name of the function.
	arguments: A sequence of ids of theory terms.

Returns:
	The unique id of the created function theory term.
)"_d)
        .def("theory_element", &Backend::theory_element, py::arg("tuple"), py::arg("condition"), R"(
Create a theory element.

The method creates a theory element consisting of a tuple and a condition, and
returns its unique identifier. This id can be used in other theory-related
methods to reference this element.

Args:
	tuple: A sequence of ids representing theory terms.
	condition: A sequence of program literals.

Returns:
	The unique id of the created theory element.
)"_d)
        .def("theory_atom", &Backend::theory_atom, py::arg("atom"), py::arg("name"), py::arg("elements"),
             py::arg("guard") = std::nullopt, R"(
Create a theory atom and return its associated program atom.

If the theory atom does not exist yet, the method assigns a program atom based
on these rules:
- If a specific atom value is provided, that value is used.
- If None is given, a fresh program atom is introduced.
- For program directives, value zero is used.

Args:
	atom: The program atom to assign, or None to assign a fresh one.
	name: The name of the theory atom.
	elements: A sequence of ids representing theory elements.
	guard: A tuple containing the guard operator and the id of the guard term, or None if there is no guard.

Returns:
	The program atom associated with the created theory atom.
)"_d);

    py::class_<BackendManager>(backend, "BackendManager", R"(
A context manager to initialize and finalize a backend.
)"_d)
        .def("__enter__", &BackendManager::enter, "Initialize backend the backend.")
        .def("__exit__", &BackendManager::exit, py::arg("type"), py::arg("value"), py::arg("traceback"),
             "Finalize the backend.");
}

} // namespace PyClingo
