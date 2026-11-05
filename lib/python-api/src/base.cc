/*
// Atom

auto Atom::literal() -> clingo_literal_t {
    auto lit = clingo_literal_t{0};
    handle_error(clingo_atom_base_literal(base_, index_, &lit));
    return lit;
}

auto Atom::symbol() -> Symbol {
    auto sym = clingo_symbol_t{0};
    handle_error(clingo_atom_base_symbol(base_, index_, &sym));
    return *cpp_cast(&sym);
}

// AtomBase

auto AtomBase::size() const -> size_t {
    size_t size = 0;
    handle_error(clingo_atom_base_size(base_, &size));
    return size;
}

auto AtomBase::at(size_t index) const -> value_type {
    if (auto atom = Atom{base_, index}; index < size()) {
        return value_type{atom.symbol(), atom};
    }
    throw py::index_error{"index out of range"};
}

auto AtomBase::get(key_type const &symbol, std::optional<mapped_type> def) const -> std::optional<mapped_type> {
    auto index = size_t{0};
    handle_error(clingo_atom_base_find(base_, *c_cast(&symbol), &index));
    return index < size() ? std::make_optional<mapped_type>(base_, index) : def;
}

auto AtomBase::contains(key_type const &symbol) const -> bool {
    auto index = size_t{0};
    handle_error(clingo_atom_base_find(base_, *c_cast(&symbol), &index));
    return index < size();
}

// Term

auto Term::symbol() -> Symbol {
    auto sym = clingo_symbol_t{0};
    handle_error(clingo_term_base_symbol(base_, index_, &sym));
    return *cpp_cast(&sym);
}

auto Term::condition() -> TypeHint<"Sequence[Sequence[int]]"> {
    size_t const *sizes = nullptr;
    clingo_literal_t const *const *lits = nullptr;
    size_t size = 0;
    auto res = std::vector<std::span<clingo_literal_t const>>{};
    handle_error(clingo_term_base_condition(base_, index_, &sizes, &lits, &size));
    for (size_t i = 0; i < size; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        res.emplace_back(lits[i], sizes[i]);
    }
    return py::cast(res);
}

// TermBase

auto TermBase::size() const -> size_t {
    size_t size = 0;
    handle_error(clingo_term_base_size(base_, &size));
    return size;
}

auto TermBase::at(size_t index) const -> value_type {
    if (auto term = Term{*base_, index}; index < size()) {
        return value_type{term.symbol(), term};
    }
    throw py::index_error{"index out of range"};
}

auto TermBase::contains(key_type const &symbol) const -> bool {
    auto index = size_t{0};
    handle_error(clingo_term_base_find(base_, *c_cast(&symbol), &index));
    return index < size();
}

auto TermBase::get(key_type const &symbol, std::optional<mapped_type> def) const -> std::optional<mapped_type> {
    auto index = size_t{0};
    handle_error(clingo_term_base_find(base_, *c_cast(&symbol), &index));
    return index < size() ? std::make_optional<mapped_type>(*base_, index) : def;
}

// TheoryTerm

auto TheoryTerm::type() -> clingo_theory_term_type_e {
    clingo_theory_term_type_t type = 0;
    handle_error(clingo_theory_base_term_type(base_, index_, &type));
    return static_cast<clingo_theory_term_type_e>(type);
}

auto TheoryTerm::number() -> int {
    int num = 0;
    handle_error(clingo_theory_base_term_number(base_, index_, &num));
    return num;
}

auto TheoryTerm::name() -> std::string_view {
    clingo_string_t name;
    handle_error(clingo_theory_base_term_name(base_, index_, &name));
    return {name.data, name.size};
}

auto TheoryTerm::arguments() -> TypeHint<"Sequence[TheoryTerm]"> {
    size_t size = 0;
    clingo_id_t const *args = nullptr;
    handle_error(clingo_theory_base_term_arguments(base_, index_, &args, &size));
    return py::cast(transform_vec(std::span{args, size}, [this](clingo_id_t id) { return TheoryTerm{*base_, id}; }));
}

auto TheoryTerm::str() -> std::string_view {
    auto *bld = string_builder();
    handle_error(clingo_theory_base_term_to_string(base_, index_, bld));
    clingo_string_t str;
    handle_error(clingo_string_builder_string(bld, &str));
    return {str.data, str.size};
}

// TheoryElement

auto TheoryElement::tuple() -> TypeHint<"Sequence[TheoryTerm]"> {
    size_t size = 0;
    clingo_id_t const *tuple = nullptr;
    handle_error(clingo_theory_base_element_tuple(base_, index_, &tuple, &size));
    return py::cast(transform_vec(std::span{tuple, size}, [this](clingo_id_t id) { return TheoryTerm{*base_, id}; }));
}

auto TheoryElement::condition() -> LitSpan {
    size_t size = 0;
    clingo_literal_t const *cond = nullptr;
    handle_error(clingo_theory_base_element_condition(base_, index_, &cond, &size));
    return std::span{cond, size};
}

auto TheoryElement::condition_id() -> std::optional<clingo_literal_t> {
    clingo_literal_t id = 0;
    handle_error(clingo_theory_base_element_condition_id(base_, index_, &id));
    return id != 0 ? std::optional(id) : std::nullopt;
}

auto TheoryElement::str() -> std::string_view {
    auto *bld = string_builder();
    handle_error(clingo_theory_base_element_to_string(base_, index_, bld));
    clingo_string_t str;
    handle_error(clingo_string_builder_string(bld, &str));
    return {str.data, str.size};
}

// TheoryAtom

auto TheoryAtom::name() -> TheoryTerm {
    clingo_id_t id = 0;
    handle_error(clingo_theory_base_atom_term(base_, index_, &id));
    return TheoryTerm{*base_, id};
}

auto TheoryAtom::elements() -> TypeHint<"Sequence[TheoryElement]"> {
    size_t size = 0;
    clingo_id_t const *elems = nullptr;
    handle_error(clingo_theory_base_atom_elements(base_, index_, &elems, &size));
    return py::cast(
        transform_vec(std::span{elems, size}, [this](clingo_id_t id) { return TheoryElement{*base_, id}; }));
}

auto TheoryAtom::literal() -> clingo_literal_t {
    clingo_literal_t lit = 0;
    handle_error(clingo_theory_base_atom_literal(base_, index_, &lit));
    return lit;
}

auto TheoryAtom::guard() -> std::optional<std::pair<std::string_view, TheoryTerm>> {
    auto has_guard = false;
    handle_error(clingo_theory_base_atom_has_guard(base_, index_, &has_guard));
    if (has_guard) {
        clingo_string_t op;
        clingo_id_t term = 0;
        handle_error(clingo_theory_base_atom_guard(base_, index_, &op, &term));
        return std::pair{std::string_view{op.data, op.size}, TheoryTerm{*base_, term}};
    }
    return std::nullopt;
}

auto TheoryAtom::str() -> std::string_view {
    auto *bld = string_builder();
    handle_error(clingo_theory_base_atom_to_string(base_, index_, bld));
    clingo_string_t str;
    handle_error(clingo_string_builder_string(bld, &str));
    return {str.data, str.size};
}

// TheoryBase

auto TheoryBase::at(size_t index) const -> value_type {
    return index < size() ? TheoryAtom{*base_, index} : throw py::index_error{"atom index out of range"};
}

auto TheoryBase::size() const -> size_t {
    auto size = size_t{0};
    handle_error(clingo_theory_base_size(base_, &size));
    return size;
}

// Base

auto Base::is_external(clingo_literal_t literal) const -> bool {
    auto ext = false;
    handle_error(clingo_base_is_external(base_, literal, &ext));
    return ext;
}

auto Base::is_fact(clingo_literal_t literal) const -> bool {
    auto fact = false;
    handle_error(clingo_base_is_fact(base_, literal, &fact));
    return fact;
}

auto Base::is_shown(clingo_literal_t literal) const -> bool {
    auto shown = false;
    handle_error(clingo_base_is_shown(base_, literal, &shown));
    return shown;
}

auto Base::is_projected(clingo_literal_t literal) const -> bool {
    auto projected = false;
    handle_error(clingo_base_is_projected(base_, literal, &projected));
    return projected;
}

auto Base::is_current(clingo_literal_t literal) const -> bool {
    auto current = false;
    handle_error(clingo_base_is_current(base_, literal, &current));
    return current;
}

auto Base::size() const -> size_t {
    size_t size = 0;
    handle_error(clingo_base_atoms_size(base_, &size));
    return size;
}

auto Base::at(size_t index) const -> value_type {
    auto sig = clingo_signature_t{};
    clingo_atom_base_t const *atoms = nullptr;
    if (index < size()) {
        handle_error(clingo_base_atoms_at(base_, index, &sig, &atoms));
        return std::pair{std::tuple{sig.name, sig.arity, sig.is_positive}, AtomBase{atoms}};
    }
    throw py::index_error{"index out of range"};
}

auto Base::contains_short(std::pair<std::string_view, size_t> const &sig) const -> bool {
    return contains({std::get<0>(sig), std::get<1>(sig), true});
}

auto Base::contains_symbol(Symbol const &sym) const -> bool {
    auto sig = sym.signature();
    if (sig && contains(*sig)) {
        auto base = lookup(*sig);
        return base.contains(sym);
    }
    return false;
}

auto Base::contains(key_type const &sig) const -> bool {
    auto csig =
        clingo_signature_t{std::get<0>(sig).data(), std::get<0>(sig).size(), std::get<1>(sig), std::get<2>(sig)};
    auto found = false;
    handle_error(clingo_base_atoms_find(base_, &csig, nullptr, &found));
    return found;
}

auto Base::get(key_type const &sig, std::optional<mapped_type> def) const -> std::optional<mapped_type> {
    auto csig =
        clingo_signature_t{std::get<0>(sig).data(), std::get<0>(sig).size(), std::get<1>(sig), std::get<2>(sig)};
    clingo_atom_base_t const *atoms = nullptr;
    auto found = false;
    handle_error(clingo_base_atoms_find(base_, &csig, &atoms, &found));
    return found ? std::make_optional<AtomBase>(atoms) : def;
}

auto Base::lookup(key_type const &sig) const -> mapped_type {
    auto csig =
        clingo_signature_t{std::get<0>(sig).data(), std::get<0>(sig).size(), std::get<1>(sig), std::get<2>(sig)};
    clingo_atom_base_t const *atoms = nullptr;
    auto found = false;
    handle_error(clingo_base_atoms_find(base_, &csig, &atoms, &found));
    return found ? AtomBase{atoms} : throw py::key_error("key does not exist");
}

auto Base::lookup_short(std::pair<std::string_view, size_t> const &sig) const -> mapped_type {
    return lookup({std::get<0>(sig), std::get<1>(sig), true});
}

auto Base::lookup_symbol(Symbol const &sym) const -> Atom {
    if (auto sig = sym.signature(); sig) {
        if (auto atom = lookup(*sig).get(sym, std::nullopt)) {
            return *atom;
        }
    }
    throw py::key_error("key does not exist");
}

auto Base::terms() const -> TermBase {
    auto const *terms = static_cast<clingo_term_base_t const *>(nullptr);
    handle_error(clingo_base_terms(base_, &terms));
    return TermBase{*terms};
}

auto Base::theory() const -> TheoryBase {
    auto const *base = static_cast<clingo_theory_base_t const *>(nullptr);
    handle_error(clingo_base_theory(base_, &base));
    return TheoryBase{*base};
}

auto convert(Base base, MixedLitSpan const &lits, bool flip, bool disjunctive) -> LitVec {
    auto res = LitVec{};
    res.reserve(lits.size());
    for (auto &&x : lits) {
        std::visit(
            [&]<typename T>(T const &x) {
                if constexpr (std::is_same_v<T, Lit_t>) {
                    res.emplace_back(flip ? -x : x);
                } else {
                    auto const &[sym, positive] = x;
                    auto pos = positive != flip;
                    if (auto dom = base.get(sym.signature().value(), std::nullopt)) {
                        if (auto atom = dom->get(sym, std::nullopt)) {
                            res.emplace_back(pos ? atom->literal() : -atom->literal());
                            return;
                        }
                    }
                    if (pos != disjunctive) {
                        res.emplace_back(1);
                        res.emplace_back(-1);
                    }
                }
            },
            x);
    }
    return res;
}

void register_base(pybind11::module &m) {
    using namespace PyClingo;

    auto base = m.def_submodule("base", R"(
Functions and classes to work with atom and term bases.

# Examples

```python
>>> from clingo.core import Library
>>> from clingo.symbol import Function, Number
>>> from clingo.control import Control
>>> lib = Library()
>>> ctl = Control(lib)
>>> ctl.parse_string("""\
... p(1).
... { p(3) }.
... #external p(1..3).
...
... q(X) :- p(X).
... """)
>>> ctl.ground()
>>> bse = ctl.base
>>> len(bse)
2
>>> p = bse[("p", 1)]
>>> Function(lib, "p", [Number(lib, 2)]) in p
True
>>> Function(lib, "p", [Number(lib, 4)]) in p
False
>>> [sig for sig in bse]
[('p', 1, False), ('q', 1, False)]
>>> [(str(x.symbol), bse.is_fact(x.literal), bse.is_external(x.literal))
...  for x in p.values()]
[('p(1)', True, False), ('p(3)', False, False), ('p(2)', False, True)]
```)"_d);

    make_hashable(py::class_<Atom>(base, "Atom", R"(A class providing information about symbolic atoms.)"))
        .def_property_readonly("literal", &Atom::literal, "Get the program literal of the atom.")
        .def_property_readonly("symbol", &Atom::symbol, "Get the symbol of the atom.");

    make_mapping(py::class_<AtomBase>(base, "AtomBase", R"(
An class providing information about symbolic atoms.

Implements `Mapping[Symbol, Atom]`.
)"_d));

    make_hashable(py::class_<Term>(base, "Term", R"(A class providing information about terms.)"))
        .def_property_readonly("symbol", &Term::symbol, "Get the symbol of the term.")
        .def_property_readonly("condition", &Term::condition, "Get the condition of the term.");

    make_mapping(py::class_<TermBase>(base, "TermBase", R"(
A class providing information about shown terms.

The base is established by the show directives occurring in a program.

Implements `Mapping[Symbol, Term]`.
)"_d));

    py::native_enum<clingo_theory_term_type_e>(base, "TheoryTermType", "enum.IntEnum",
                                               "Enumeration of theory term types.")
        .value("Number", clingo_theory_term_type_number, R"(For numeric theory terms.)")
        .value("Symbol", clingo_theory_term_type_symbol, R"(For symbolic theory terms (simple strings).)")
        .value("Tuple", clingo_theory_term_type_tuple, R"(For tuple theory terms.)")
        .value("List", clingo_theory_term_type_list, R"(For list theory term.)")
        .value("Set", clingo_theory_term_type_set, R"(For set theory terms.)")
        .value("Function", clingo_theory_term_type_function, R"(For function theory terms.)")
        .finalize();

    make_hashable(py::class_<TheoryTerm>(base, "TheoryTerm", R"(A view to inspect a theory term.)"))
        .def("__str__", &TheoryTerm::str, R"(Get a string representation of the term.)")
        .def_property_readonly("type", &TheoryTerm::type, R"(Get the type of the theory term.)")
        .def_property_readonly("number", &TheoryTerm::number, R"(Get the value of a numeric theory term.)")
        .def_property_readonly("name", &TheoryTerm::name, R"(Get the name of a theory symbol or function.)")
        .def_property_readonly("arguments", &TheoryTerm::arguments,
                               R"(Get the arguments of a function, tuple, list, or set theory term.)");

    make_hashable(py::class_<TheoryElement>(base, "TheoryElement", R"(A view to inspect a theory element.)"))
        .def("__str__", &TheoryElement::str, R"(Get a string representation of the element.)")
        .def_property_readonly("tuple", &TheoryElement::tuple, R"(Get the term tuple of a theory element.)")
        .def_property_readonly("condition", &TheoryElement::condition, R"(Get the condition of a theory element.)")
        .def_property_readonly("condition_id", &TheoryElement::condition_id,
                               R"(Get the condition id of a theory element.

True conditions do not have a condition id. A condition id is only valid for
the current solving step. However, they can be mapped to persistent solver
literals using `clingo.propagate.PropagateInit.solver_literal`.
)");

    make_hashable(py::class_<TheoryAtom>(base, "TheoryAtom", R"(A view to inspect a theory atom.)"))
        .def("__str__", &TheoryAtom::str, R"(Get a string representation of the atom.)")
        .def_property_readonly("name", &TheoryAtom::name, R"(Get the name of a theory atom.)")
        .def_property_readonly("elements", &TheoryAtom::elements, R"(Get the elements of a theory atom.)")
        .def_property_readonly("literal", &TheoryAtom::literal,
                               R"(Get the literal of the theory atom (zero for directives).)")
        .def_property_readonly("guard", &TheoryAtom::guard, R"(Get optional guard of a theory atom.)");

    make_sequence(py::class_<TheoryBase>(base, "TheoryBase", R"(
A class  prooviding information about theory atoms.

Implements `Sequence[TheoryAtom]`.
)"_d));

    make_mapping(py::class_<Base>(base, "Base", R"(
A class providing information about symbolic and theory atoms and shown terms.

Implements `Mapping[tuple[str, int, bool], AtomBase]` providing additional
overloads to directly lookup symbols and short signatures (assuming a positive
sign):

- `__getitem__: Callable[[Symbol], Atom]`
- `__contains__: Callable[[Symbol], bool]`
- `__getitem__: Callable[[tuple[str, int]], AtomBase]`
- `__contains__: Callable[[tuple[str, int]], bool]`
)"))
        .def("__getitem__", &Base::lookup_symbol, py::arg("symbol"), R"(Get the atom with the given symbol.)")
        .def("__getitem__", &Base::lookup_short, py::arg("signature"), R"(
Get the atom base with the given (short) signature.

This function provides a shortcut assuming the sign is positive.
)"_d)
        .def("__contains__", &Base::contains_short, py::arg("signature"),
             R"( Check if there is an atom base with the given (short) signature.)")
        .def("__contains__", &Base::contains_symbol, py::arg("symbol"),
             R"(Check if there is an atom with the given symbol.)")
        .def("is_external", &Base::is_external, py::arg("literal"), R"(
Check whether the given program literal corresponds to an external.

Args:
    literal: The literal to check.
Returns:
    Whether the literal is external.
)"_d)
        .def("is_fact", &Base::is_fact, py::arg("literal"), R"(
Check whether the literal is a fact.

Args:
    literal: The literal to check.
Returns:
    Whether the literal is a fact.
)"_d)
        .def("is_shown", &Base::is_shown, py::arg("literal"), R"(
Check whether the literal is shown via a `#show` directive.

Args:
    literal: The literal to check.
Returns:
    Whether the literal is shown.
)"_d)
        .def("is_projected", &Base::is_projected, py::arg("literal"), R"(
Check whether the literal is part of a `#project` directive.

Args:
    literal: The literal to check.
Returns:
    Whether the literal is subject to projection.
)"_d)
        .def("is_current", &Base::is_current, py::arg("literal"), R"(
Check whether a literal has been introduced in the current step.

Note that all literals introduced before the last solve call are considered
from a previous step.

Args:
    literal: The literal to check.
Returns:
    Whether the literal is subject to projection.
)"_d)
        .def_property_readonly("terms", &Base::terms, "The term base (given by show directives).")
        .def_property_readonly("theory", &Base::theory, "The theory base.");
}

} // namespace PyClingo
*/

#define WITH_DOC_STRING
#define PY_SSIZE_T_CLEAN

#include "base.hh"

#include "symbol.hh"
#include "util.hh"

#include <array>
#include <cassert>
#include <cstring>
#include <limits>
#include <utility>

namespace PyClingo {

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables,cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-type-const-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)

namespace {

using PyAtomObject = struct {
    PyObject_HEAD const clingo_atom_base_t *base;
    size_t index;
};

using PyAtomBaseObject = struct {
    PyObject_HEAD const clingo_atom_base_t *base;
};

using PyTermObject = struct {
    PyObject_HEAD const clingo_term_base_t *base;
    size_t index;
};

using PyTermBaseObject = struct {
    PyObject_HEAD const clingo_term_base_t *base;
};

using PyTheoryTermObject = struct {
    PyObject_HEAD const clingo_theory_base_t *base;
    size_t index;
};

using PyTheoryElementObject = struct {
    PyObject_HEAD const clingo_theory_base_t *base;
    size_t index;
};

using PyTheoryAtomObject = struct {
    PyObject_HEAD const clingo_theory_base_t *base;
    size_t index;
};

using PyTheoryBaseObject = struct {
    PyObject_HEAD const clingo_theory_base_t *base;
};

using PyBaseObject = struct {
    PyObject_HEAD const clingo_base_t *base;
};

struct BaseModuleState {
    PyObject *atom_type{nullptr};
    PyObject *atom_base_type{nullptr};
    PyObject *term_type{nullptr};
    PyObject *term_base_type{nullptr};
    PyObject *theory_term_type{nullptr};
    PyObject *theory_element_type{nullptr};
    PyObject *theory_atom_type{nullptr};
    PyObject *theory_base_type{nullptr};
    PyObject *base_type{nullptr};
    PyObject *theory_term_type_enum{nullptr};
};

auto get_state(PyObject *module) -> BaseModuleState * {
    auto *state = static_cast<BaseModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "base module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_state_from_self(PyObject *self) -> BaseModuleState * {
    auto *mod = PyType_GetModule(Py_TYPE(self));
    if (mod == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "failed to resolve base module");
        return nullptr;
    }
    return get_state(mod);
}

// Atom

/*
class Atom {
  public:
    Atom(clingo_atom_base_t const *base, size_t index) : base_{base}, index_{index} {}

    auto literal() -> clingo_literal_t;
    auto symbol() -> Symbol;

    [[nodiscard]] auto hash() const { return hash_combine(index_, hash_value(base_)); }
    friend auto operator==(Atom const &a, Atom const &b) -> bool { return a.index_ == b.index_; }
    friend auto operator!=(Atom const &a, Atom const &b) -> bool = default;

  private:
    clingo_atom_base_t const *base_;
    size_t index_;
};
*/

auto py_atom_new(PyTypeObject *type, const clingo_atom_base_t *base, size_t index) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *a = reinterpret_cast<PyAtomObject *>(obj);
    a->base = base;
    a->index = index;
    return obj;
}

auto py_atom_hash(PyObject *self) -> Py_hash_t {
    auto *a = reinterpret_cast<PyAtomObject *>(self);
    return static_cast<Py_hash_t>(hash_combine(a->index, std::hash<void const *>{}(a->base)));
}

auto py_atom_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    if (!PyObject_TypeCheck(b, Py_TYPE(a))) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto *x = reinterpret_cast<PyAtomObject *>(a);
    auto *y = reinterpret_cast<PyAtomObject *>(b);
    bool eq = x->base == y->base && x->index == y->index;
    bool res = false;
    if (op == Py_EQ) {
        res = eq;
    } else if (op == Py_NE) {
        res = !eq;
    } else {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_atom_get_literal(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *a = reinterpret_cast<PyAtomObject *>(self); // NOLINT
    clingo_literal_t lit = 0;
    if (!clingo_atom_base_literal(a->base, a->index, &lit)) {
        return py_handle_error();
    }
    return PyLong_FromLong(lit);
}

auto py_atom_get_symbol(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *a = reinterpret_cast<PyAtomObject *>(self); // NOLINT
    clingo_symbol_t sym = 0;
    if (!clingo_atom_base_symbol(a->base, a->index, &sym)) {
        return py_handle_error();
    }
    return py_symbol_from_clingo_object(sym, true);
}

auto py_atom_getset = std::array{
    PyGetSetDef{"literal", py_atom_get_literal, nullptr, PyDoc_STR("Program literal."), nullptr},
    PyGetSetDef{"symbol", py_atom_get_symbol, nullptr, PyDoc_STR("Atom symbol."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_atom_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("View over a symbolic atom."))},
    PyType_Slot{Py_tp_hash, reinterpret_cast<void *>(py_atom_hash)},
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(py_atom_richcompare)},
    PyType_Slot{Py_tp_getset, py_atom_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_atom_spec = PyType_Spec{
    "clingo.base.Atom",   sizeof(PyAtomObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_atom_slots.data(),
};

// AtomBase

/*
class AtomBase {
  public:
    using key_type = Symbol;
    using mapped_type = Atom;
    using value_type = std::pair<key_type, mapped_type>;

    explicit AtomBase(clingo_atom_base_t const *base) : base_{base} {}

    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto at(size_t index) const -> value_type;
    [[nodiscard]] auto contains(key_type const &symbol) const -> bool;
    [[nodiscard]] auto get(key_type const &symbol, std::optional<mapped_type> def) const -> std::optional<mapped_type>;

  private:
    clingo_atom_base_t const *base_;
};
*/

auto py_atom_base_new(PyTypeObject *type, const clingo_atom_base_t *base) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *a = reinterpret_cast<PyAtomBaseObject *>(obj);
    a->base = base;
    return obj;
}

auto py_atom_base_len(PyObject *self) -> Py_ssize_t {
    auto *a = reinterpret_cast<PyAtomBaseObject *>(self);
    size_t n = 0;
    if (!clingo_atom_base_size(a->base, &n)) {
        py_handle_error();
        return -1;
    }
    if (n > static_cast<size_t>(std::numeric_limits<Py_ssize_t>::max())) {
        PyErr_SetString(PyExc_OverflowError, "size does not fit in Py_ssize_t");
        return -1;
    }
    return static_cast<Py_ssize_t>(n);
}

auto atom_base_lookup(PyObject *self, PyObject *key, size_t *idx, clingo_symbol_t *sym_out = nullptr) -> int {
    clingo_symbol_t sym = 0;
    size_t index = 0;
    if (auto res = py_symbol_to_clingo_check(key, &sym); res != 1) {
        return res;
    }
    auto *a = reinterpret_cast<PyAtomBaseObject *>(self);
    if (!clingo_atom_base_find(a->base, sym, &index)) {
        py_handle_error();
        return -1;
    }
    if (idx != nullptr) {
        *idx = index;
    }
    if (sym_out != nullptr) {
        *sym_out = sym;
    }
    return 1;
}

auto py_atom_base_contains(PyObject *self, PyObject *key) -> int {
    size_t idx = 0;
    auto n = py_atom_base_len(self);
    if (n < 0) {
        return -1;
    }
    auto res = atom_base_lookup(self, key, &idx);
    if (res < 0) {
        return -1;
    }
    return res == 1 && std::cmp_less(idx, n) ? 1 : 0;
}

auto py_atom_base_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    size_t idx = 0;
    auto n = py_atom_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto res = atom_base_lookup(self, key, &idx);
    if (res < 0) {
        return nullptr;
    }
    if (res == 0) {
        PyErr_SetString(PyExc_TypeError, "key must be a Symbol");
        return nullptr;
    }
    if (std::cmp_greater_equal(idx, n)) {
        PyErr_SetString(PyExc_KeyError, "key not found");
        return nullptr;
    }
    auto *a = reinterpret_cast<PyAtomBaseObject *>(self); // NOLINT
    return py_atom_new(reinterpret_cast<PyTypeObject *>(state->atom_type), a->base, idx);
}

auto py_atom_base_get(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"key", "default", nullptr};
    PyObject *key = nullptr;
    PyObject *def = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:get", cast_kw(kwlist), &key, &def) == 0) {
        return nullptr;
    }

    size_t idx = 0;
    auto n = py_atom_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto res = atom_base_lookup(self, key, &idx);
    if (res < 0) {
        return nullptr;
    }
    if (res == 0) {
        PyErr_SetString(PyExc_TypeError, "key must be a Symbol");
        return nullptr;
    }
    if (std::cmp_greater_equal(idx, n)) {
        Py_INCREF(def);
        return def;
    }
    return py_atom_base_subscript(self, key);
}

auto py_atom_base_keys(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto n = py_atom_base_len(self);
    if (n < 0) {
        return nullptr;
    }

    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(n));
    if (list == nullptr) {
        return nullptr;
    }

    auto *a = reinterpret_cast<PyAtomBaseObject *>(self); // NOLINT
    for (Py_ssize_t i = 0; i < n; ++i) {
        clingo_symbol_t sym = 0;
        if (!clingo_atom_base_symbol(a->base, static_cast<size_t>(i), &sym)) {
            return py_handle_error();
        }
        auto *obj = py_symbol_from_clingo_object(sym, true);
        if (obj == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(list.get(), i, obj);
    }

    return PyObject_GetIter(list.get());
}

auto py_atom_base_iter(PyObject *self) -> PyObject * {
    return py_atom_base_keys(self, nullptr);
}

auto py_atom_base_values(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto n = py_atom_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(n));
    if (list == nullptr) {
        return nullptr;
    }

    auto *a = reinterpret_cast<PyAtomBaseObject *>(self); // NOLINT
    for (Py_ssize_t i = 0; i < n; ++i) {
        auto *obj = py_atom_new(reinterpret_cast<PyTypeObject *>(state->atom_type), a->base, static_cast<size_t>(i));
        if (obj == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(list.get(), i, obj);
    }

    return PyObject_GetIter(list.get());
}

auto py_atom_base_items(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto n = py_atom_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(n));
    if (list == nullptr) {
        return nullptr;
    }

    auto *a = reinterpret_cast<PyAtomBaseObject *>(self); // NOLINT
    for (Py_ssize_t i = 0; i < n; ++i) {
        clingo_symbol_t sym = 0;
        if (!clingo_atom_base_symbol(a->base, static_cast<size_t>(i), &sym)) {
            return py_handle_error();
        }
        auto key = Guard<PyObject *, Py_DECREF>(py_symbol_from_clingo_object(sym, true));
        auto val = Guard<PyObject *, Py_DECREF>(
            py_atom_new(reinterpret_cast<PyTypeObject *>(state->atom_type), a->base, static_cast<size_t>(i)));
        if (key.get() == nullptr || val.get() == nullptr) {
            return nullptr;
        }
        auto item = Guard<PyObject *, Py_DECREF>(PyTuple_Pack(2, key.get(), val.get()));
        if (item.get() == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(list.get(), i, item.release());
    }

    return PyObject_GetIter(list.get());
}

auto py_atom_base_methods = std::array{
    PyMethodDef{"get", py_c_function_cast(py_atom_base_get), METH_VARARGS | METH_KEYWORDS,
                PyDoc_STR("Get value or default.")}, // NOLINT
    PyMethodDef{"keys", py_atom_base_keys, METH_NOARGS, PyDoc_STR("Iterate keys.")},
    PyMethodDef{"values", py_atom_base_values, METH_NOARGS, PyDoc_STR("Iterate values.")},
    PyMethodDef{"items", py_atom_base_items, METH_NOARGS, PyDoc_STR("Iterate items.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_atom_base_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Mapping Symbol -> Atom."))}, // NOLINT
    PyType_Slot{Py_tp_methods, py_atom_base_methods.data()},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_atom_base_len)},          // NOLINT
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_atom_base_subscript)}, // NOLINT
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(py_atom_base_contains)},   // NOLINT
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_atom_base_iter)},           // NOLINT
    PyType_Slot{0, nullptr},
};

auto py_atom_base_spec = PyType_Spec{
    "clingo.base.AtomBase",
    sizeof(PyAtomBaseObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_MAPPING | Py_TPFLAGS_SEQUENCE,
    py_atom_base_slots.data(),
};

// Term

/*
class Term {
  public:
    Term(clingo_term_base_t const &base, size_t index) : base_{&base}, index_{index} {}

    auto symbol() -> Symbol;
    auto condition() -> TypeHint<"Sequence[Sequence[int]]">;

    [[nodiscard]] auto hash() const { return hash_combine(index_, hash_value(base_)); }
    friend auto operator==(Term const &a, Term const &b) -> bool { return a.index_ == b.index_; }
    friend auto operator!=(Term const &a, Term const &b) -> bool = default;

  private:
    clingo_term_base_t const *base_;
    size_t index_;
};
*/

auto py_term_new(PyTypeObject *type, const clingo_term_base_t *base, size_t index) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTermObject *>(obj); // NOLINT
    t->base = base;
    t->index = index;
    return obj;
}

auto py_term_hash(PyObject *self) -> Py_hash_t {
    auto *t = reinterpret_cast<PyTermObject *>(self); // NOLINT
    return static_cast<Py_hash_t>(hash_combine(t->index, std::hash<void const *>{}(t->base)));
}

auto py_term_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    if (!PyObject_TypeCheck(b, Py_TYPE(a))) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto *x = reinterpret_cast<PyTermObject *>(a); // NOLINT
    auto *y = reinterpret_cast<PyTermObject *>(b); // NOLINT
    bool eq = x->base == y->base && x->index == y->index;
    bool res = false;
    if (op == Py_EQ) {
        res = eq;
    } else if (op == Py_NE) {
        res = !eq;
    } else {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_term_get_symbol(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTermObject *>(self); // NOLINT
    clingo_symbol_t sym = 0;
    if (!clingo_term_base_symbol(t->base, t->index, &sym)) {
        return py_handle_error();
    }
    return py_symbol_from_clingo_object(sym, true);
}

auto py_term_get_condition(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTermObject *>(self); // NOLINT
    size_t const *sizes = nullptr;
    clingo_literal_t const *const *lits = nullptr;
    size_t n = 0;
    if (!clingo_term_base_condition(t->base, t->index, &sizes, &lits, &n)) {
        return py_handle_error();
    }

    auto out = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(n)));
    if (out.get() == nullptr) {
        return nullptr;
    }

    for (size_t i = 0; i < n; ++i) {
        auto inner = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(sizes[i])));
        if (inner.get() == nullptr) {
            return nullptr;
        }
        for (size_t j = 0; j < sizes[i]; ++j) {
            auto *v = PyLong_FromLong(lits[i][j]);
            if (v == nullptr) {
                return nullptr;
            }
            PyList_SET_ITEM(inner.get(), static_cast<Py_ssize_t>(j), v);
        }
        PyList_SET_ITEM(out.get(), static_cast<Py_ssize_t>(i), inner.release());
    }
    return out.release();
}

auto py_term_getset = std::array{
    PyGetSetDef{"symbol", py_term_get_symbol, nullptr, PyDoc_STR("Term symbol."), nullptr},
    PyGetSetDef{"condition", py_term_get_condition, nullptr, PyDoc_STR("Term condition."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_term_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("View over a shown term."))}, // NOLINT
    PyType_Slot{Py_tp_hash, reinterpret_cast<void *>(py_term_hash)},                  // NOLINT
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(py_term_richcompare)},    // NOLINT
    PyType_Slot{Py_tp_getset, py_term_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_term_spec = PyType_Spec{
    "clingo.base.Term",   sizeof(PyTermObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_term_slots.data(),
};

// TermBase

/*
class TermBase {
  public:
    using key_type = Symbol;
    using mapped_type = Term;
    using value_type = std::pair<key_type, mapped_type>;

    explicit TermBase(clingo_term_base_t const &base) : base_{&base} {}

    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto at(size_t index) const -> value_type;
    [[nodiscard]] auto contains(key_type const &symbol) const -> bool;
    [[nodiscard]] auto get(key_type const &symbol, std::optional<mapped_type> def) const -> std::optional<mapped_type>;

  private:
    clingo_term_base_t const *base_;
};
*/

auto py_term_base_new(PyTypeObject *type, const clingo_term_base_t *base) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTermBaseObject *>(obj); // NOLINT
    t->base = base;
    return obj;
}

auto py_term_base_len(PyObject *self) -> Py_ssize_t {
    auto *t = reinterpret_cast<PyTermBaseObject *>(self); // NOLINT
    size_t n = 0;
    if (!clingo_term_base_size(t->base, &n)) {
        py_handle_error();
        return -1;
    }
    if (n > static_cast<size_t>(std::numeric_limits<Py_ssize_t>::max())) {
        PyErr_SetString(PyExc_OverflowError, "size does not fit in Py_ssize_t");
        return -1;
    }
    return static_cast<Py_ssize_t>(n);
}

auto term_base_lookup(PyObject *self, PyObject *key, size_t *idx) -> int {
    clingo_symbol_t sym = 0;
    if (auto res = py_symbol_to_clingo_check(key, &sym); res != 1) {
        return res;
    }
    auto *t = reinterpret_cast<PyTermBaseObject *>(self);
    size_t index = 0;
    if (!clingo_term_base_find(t->base, sym, &index)) {
        py_handle_error();
        return -1;
    }
    *idx = index;
    return 1;
}

auto py_term_base_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    size_t idx = 0;
    auto n = py_term_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto res = term_base_lookup(self, key, &idx);
    if (res < 0) {
        return nullptr;
    }
    if (res == 0) {
        PyErr_SetString(PyExc_TypeError, "key must be a Symbol");
        return nullptr;
    }
    if (std::cmp_greater_equal(idx, n)) {
        PyErr_SetString(PyExc_KeyError, "key not found");
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTermBaseObject *>(self); // NOLINT
    return py_term_new(reinterpret_cast<PyTypeObject *>(state->term_type), t->base, idx);
}

auto py_term_base_contains(PyObject *self, PyObject *key) -> int {
    size_t idx = 0;
    auto n = py_term_base_len(self);
    if (n < 0) {
        return -1;
    }
    auto res = term_base_lookup(self, key, &idx);
    if (res < 0) {
        return -1;
    }
    return res == 1 && std::cmp_less(idx, n) ? 1 : 0;
}

auto py_term_base_get(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"key", "default", nullptr};
    PyObject *key = nullptr;
    PyObject *def = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:get", cast_kw(kwlist), &key, &def) == 0) {
        return nullptr;
    }
    auto n = py_term_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    size_t idx = 0;
    auto res = term_base_lookup(self, key, &idx);
    if (res < 0) {
        return nullptr;
    }
    if (res == 0) {
        PyErr_SetString(PyExc_TypeError, "key must be a Symbol");
        return nullptr;
    }
    if (std::cmp_greater_equal(idx, n)) {
        Py_INCREF(def);
        return def;
    }
    return py_term_base_subscript(self, key);
}

auto py_term_base_keys(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto n = py_term_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTermBaseObject *>(self); // NOLINT
    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(n));
    if (list.get() == nullptr) {
        return nullptr;
    }
    for (Py_ssize_t i = 0; i < n; ++i) {
        clingo_symbol_t sym = 0;
        if (!clingo_term_base_symbol(t->base, static_cast<size_t>(i), &sym)) {
            return py_handle_error();
        }
        auto *obj = py_symbol_from_clingo_object(sym, true);
        if (obj == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(list.get(), i, obj);
    }
    return PyObject_GetIter(list.get());
}

auto py_term_base_iter(PyObject *self) -> PyObject * {
    return py_term_base_keys(self, nullptr);
}

auto py_term_base_values(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto n = py_term_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTermBaseObject *>(self); // NOLINT
    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(n));
    if (list.get() == nullptr) {
        return nullptr;
    }
    for (Py_ssize_t i = 0; i < n; ++i) {
        auto *obj = py_term_new(reinterpret_cast<PyTypeObject *>(state->term_type), t->base, static_cast<size_t>(i));
        if (obj == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(list.get(), i, obj);
    }
    return PyObject_GetIter(list.get());
}

auto py_term_base_items(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto n = py_term_base_len(self);
    if (n < 0) {
        return nullptr;
    }

    auto *t = reinterpret_cast<PyTermBaseObject *>(self); // NOLINT
    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(n));
    if (list.get() == nullptr) {
        return nullptr;
    }

    for (Py_ssize_t i = 0; i < n; ++i) {
        clingo_symbol_t sym = 0;
        if (!clingo_term_base_symbol(t->base, static_cast<size_t>(i), &sym)) {
            return py_handle_error();
        }
        auto key = Guard<PyObject *, Py_DECREF>(py_symbol_from_clingo_object(sym, true));
        auto val = Guard<PyObject *, Py_DECREF>(
            py_term_new(reinterpret_cast<PyTypeObject *>(state->term_type), t->base, static_cast<size_t>(i)));
        if (key.get() == nullptr || val.get() == nullptr) {
            return nullptr;
        }
        auto item = Guard<PyObject *, Py_DECREF>(PyTuple_Pack(2, key.get(), val.get()));
        if (item.get() == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(list.get(), i, item.release());
    }

    return PyObject_GetIter(list.get());
}

auto py_term_base_methods = std::array{
    PyMethodDef{"get", py_c_function_cast(py_term_base_get), METH_VARARGS | METH_KEYWORDS,
                PyDoc_STR("Get value or default.")}, // NOLINT
    PyMethodDef{"keys", py_term_base_keys, METH_NOARGS, PyDoc_STR("Iterate keys.")},
    PyMethodDef{"values", py_term_base_values, METH_NOARGS, PyDoc_STR("Iterate values.")},
    PyMethodDef{"items", py_term_base_items, METH_NOARGS, PyDoc_STR("Iterate items.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_term_base_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Mapping Symbol -> Term."))}, // NOLINT
    PyType_Slot{Py_tp_methods, py_term_base_methods.data()},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_term_base_len)},          // NOLINT
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_term_base_subscript)}, // NOLINT
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(py_term_base_contains)},   // NOLINT
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_term_base_iter)},           // NOLINT
    PyType_Slot{0, nullptr},
};

auto py_term_base_spec = PyType_Spec{
    "clingo.base.TermBase",
    sizeof(PyTermBaseObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_MAPPING | Py_TPFLAGS_SEQUENCE,
    py_term_base_slots.data(),
};

// TheoryTerm

/*
class TheoryTerm {
  public:
    TheoryTerm(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}
    auto type() -> clingo_theory_term_type_e;
    auto number() -> int;
    auto name() -> std::string_view;
    auto arguments() -> TypeHint<"Sequence[TheoryTerm]">;
    auto str() -> std::string_view;

    [[nodiscard]] auto hash() const { return hash_combine(index_, hash_value(base_)); }
    friend auto operator==(TheoryTerm const &a, TheoryTerm const &b) -> bool { return a.index_ == b.index_; }
    friend auto operator!=(TheoryTerm const &a, TheoryTerm const &b) -> bool = default;

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};
*/

auto py_theory_term_new(PyTypeObject *type, const clingo_theory_base_t *base, size_t index) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryTermObject *>(obj); // NOLINT
    t->base = base;
    t->index = index;
    return obj;
}

auto py_theory_term_hash(PyObject *self) -> Py_hash_t {
    auto *t = reinterpret_cast<PyTheoryTermObject *>(self); // NOLINT
    return static_cast<Py_hash_t>(hash_combine(t->index, std::hash<void const *>{}(t->base)));
}

auto py_theory_term_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    if (!PyObject_TypeCheck(b, Py_TYPE(a))) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto *x = reinterpret_cast<PyTheoryTermObject *>(a); // NOLINT
    auto *y = reinterpret_cast<PyTheoryTermObject *>(b); // NOLINT
    bool eq = x->base == y->base && x->index == y->index;
    bool res = false;
    if (op == Py_EQ) {
        res = eq;
    } else if (op == Py_NE) {
        res = !eq;
    } else {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_theory_term_str(PyObject *self) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryTermObject *>(self); // NOLINT
    auto bld = Guard<clingo_string_builder_t *, clingo_string_builder_free>{nullptr};
    if (!clingo_string_builder_new(&bld.get())) {
        return py_handle_error();
    }
    if (!clingo_theory_base_term_to_string(t->base, t->index, bld)) {
        return py_handle_error();
    }
    clingo_string_t str;
    if (!clingo_string_builder_string(bld, &str)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
}

auto py_theory_term_get_type(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryTermObject *>(self); // NOLINT
    clingo_theory_term_type_t ty = 0;
    if (!clingo_theory_base_term_type(t->base, t->index, &ty)) {
        return py_handle_error();
    }
    return PyObject_CallFunction(state->theory_term_type_enum, "i", static_cast<int>(ty));
}

auto py_theory_term_get_number(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryTermObject *>(self); // NOLINT
    int num = 0;
    if (!clingo_theory_base_term_number(t->base, t->index, &num)) {
        return py_handle_error();
    }
    return PyLong_FromLong(num);
}

auto py_theory_term_get_name(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryTermObject *>(self); // NOLINT
    clingo_string_t name;
    if (!clingo_theory_base_term_name(t->base, t->index, &name)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(name.data, static_cast<Py_ssize_t>(name.size));
}

auto py_theory_term_get_arguments(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryTermObject *>(self); // NOLINT
    clingo_id_t const *args = nullptr;
    size_t n = 0;
    if (!clingo_theory_base_term_arguments(t->base, t->index, &args, &n)) {
        return py_handle_error();
    }

    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(n)));
    if (list.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < n; ++i) {
        auto *obj = py_theory_term_new(reinterpret_cast<PyTypeObject *>(state->theory_term_type), t->base, args[i]);
        if (obj == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(list.get(), static_cast<Py_ssize_t>(i), obj);
    }
    return list.release();
}

auto py_theory_term_getset = std::array{
    PyGetSetDef{"type", py_theory_term_get_type, nullptr, PyDoc_STR("Theory term type."), nullptr},
    PyGetSetDef{"number", py_theory_term_get_number, nullptr, PyDoc_STR("Numeric value."), nullptr},
    PyGetSetDef{"name", py_theory_term_get_name, nullptr, PyDoc_STR("Name."), nullptr},
    PyGetSetDef{"arguments", py_theory_term_get_arguments, nullptr, PyDoc_STR("Arguments."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_theory_term_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("View over a theory term."))},    // NOLINT
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_theory_term_str)},                 // NOLINT
    PyType_Slot{Py_tp_hash, reinterpret_cast<void *>(py_theory_term_hash)},               // NOLINT
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(py_theory_term_richcompare)}, // NOLINT
    PyType_Slot{Py_tp_getset, py_theory_term_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_theory_term_spec = PyType_Spec{
    "clingo.base.TheoryTerm",    sizeof(PyTheoryTermObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_theory_term_slots.data(),
};

// TheoryElement

/*
class TheoryElement {
  public:
    TheoryElement(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}
    auto tuple() -> TypeHint<"Sequence[TheoryTerm]">;
    auto condition() -> LitSpan;
    auto condition_id() -> std::optional<clingo_literal_t>;
    auto str() -> std::string_view;

    [[nodiscard]] auto hash() const { return hash_combine(index_, hash_value(base_)); }
    friend auto operator==(TheoryElement const &a, TheoryElement const &b) -> bool { return a.index_ == b.index_; }
    friend auto operator!=(TheoryElement const &a, TheoryElement const &b) -> bool = default;

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};
*/

auto py_theory_element_new(PyTypeObject *type, const clingo_theory_base_t *base, size_t index) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryElementObject *>(obj); // NOLINT
    t->base = base;
    t->index = index;
    return obj;
}

auto py_theory_element_hash(PyObject *self) -> Py_hash_t {
    auto *t = reinterpret_cast<PyTheoryElementObject *>(self); // NOLINT
    return static_cast<Py_hash_t>(hash_combine(t->index, std::hash<void const *>{}(t->base)));
}

auto py_theory_element_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    if (!PyObject_TypeCheck(b, Py_TYPE(a))) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto *x = reinterpret_cast<PyTheoryElementObject *>(a); // NOLINT
    auto *y = reinterpret_cast<PyTheoryElementObject *>(b); // NOLINT
    bool eq = x->base == y->base && x->index == y->index;
    bool res = false;
    if (op == Py_EQ) {
        res = eq;
    } else if (op == Py_NE) {
        res = !eq;
    } else {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_theory_element_str(PyObject *self) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryElementObject *>(self); // NOLINT
    auto bld = Guard<clingo_string_builder_t *, clingo_string_builder_free>{nullptr};
    if (!clingo_string_builder_new(&bld.get())) {
        return py_handle_error();
    }
    if (!clingo_theory_base_element_to_string(t->base, t->index, bld)) {
        return py_handle_error();
    }
    clingo_string_t str;
    if (!clingo_string_builder_string(bld, &str)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
}

auto py_theory_element_get_tuple(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryElementObject *>(self); // NOLINT
    clingo_id_t const *vals = nullptr;
    size_t n = 0;
    if (!clingo_theory_base_element_tuple(t->base, t->index, &vals, &n)) {
        return py_handle_error();
    }
    auto out = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(n)));
    if (out.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < n; ++i) {
        auto *obj = py_theory_term_new(reinterpret_cast<PyTypeObject *>(state->theory_term_type), t->base, vals[i]);
        if (obj == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(out.get(), static_cast<Py_ssize_t>(i), obj);
    }
    return out.release();
}

auto py_theory_element_get_condition(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryElementObject *>(self); // NOLINT
    clingo_literal_t const *vals = nullptr;
    size_t n = 0;
    if (!clingo_theory_base_element_condition(t->base, t->index, &vals, &n)) {
        return py_handle_error();
    }
    auto out = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(n)));
    if (out.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < n; ++i) {
        auto *v = PyLong_FromLong(vals[i]);
        if (v == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(out.get(), static_cast<Py_ssize_t>(i), v);
    }
    return out.release();
}

auto py_theory_element_get_condition_id(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryElementObject *>(self); // NOLINT
    clingo_literal_t id = 0;
    if (!clingo_theory_base_element_condition_id(t->base, t->index, &id)) {
        return py_handle_error();
    }
    if (id == 0) {
        Py_RETURN_NONE;
    }
    return PyLong_FromLong(id);
}

auto py_theory_element_getset = std::array{
    PyGetSetDef{"tuple", py_theory_element_get_tuple, nullptr, PyDoc_STR("Term tuple."), nullptr},
    PyGetSetDef{"condition", py_theory_element_get_condition, nullptr, PyDoc_STR("Condition literals."), nullptr},
    PyGetSetDef{
        "condition_id",
        py_theory_element_get_condition_id,
        nullptr,
        PyDoc_STR("Condition id or None."),
        nullptr,
    },
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_theory_element_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("View over a theory element."))},    // NOLINT
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_theory_element_str)},                 // NOLINT
    PyType_Slot{Py_tp_hash, reinterpret_cast<void *>(py_theory_element_hash)},               // NOLINT
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(py_theory_element_richcompare)}, // NOLINT
    PyType_Slot{Py_tp_getset, py_theory_element_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_theory_element_spec = PyType_Spec{
    "clingo.base.TheoryElement",
    sizeof(PyTheoryElementObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_theory_element_slots.data(),
};

// TheoryAtom

/*
class TheoryAtom {
  public:
    TheoryAtom(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}
    auto name() -> TheoryTerm;
    auto elements() -> TypeHint<"Sequence[TheoryElement]">;
    auto literal() -> clingo_literal_t;
    auto guard() -> std::optional<std::pair<std::string_view, TheoryTerm>>;
    auto str() -> std::string_view;

    [[nodiscard]] auto hash() const { return hash_combine(index_, hash_value(base_)); }
    friend auto operator==(TheoryAtom const &a, TheoryAtom const &b) -> bool { return a.index_ == b.index_; }
    friend auto operator!=(TheoryAtom const &a, TheoryAtom const &b) -> bool = default;

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};
*/

auto py_theory_atom_new(PyTypeObject *type, const clingo_theory_base_t *base, size_t index) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(obj); // NOLINT
    t->base = base;
    t->index = index;
    return obj;
}

auto py_theory_atom_hash(PyObject *self) -> Py_hash_t {
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(self); // NOLINT
    return static_cast<Py_hash_t>(hash_combine(t->index, std::hash<void const *>{}(t->base)));
}

auto py_theory_atom_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    if (!PyObject_TypeCheck(b, Py_TYPE(a))) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto *x = reinterpret_cast<PyTheoryAtomObject *>(a); // NOLINT
    auto *y = reinterpret_cast<PyTheoryAtomObject *>(b); // NOLINT
    bool eq = x->base == y->base && x->index == y->index;
    bool res = false;
    if (op == Py_EQ) {
        res = eq;
    } else if (op == Py_NE) {
        res = !eq;
    } else {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_theory_atom_str(PyObject *self) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(self); // NOLINT
    auto bld = Guard<clingo_string_builder_t *, clingo_string_builder_free>{nullptr};
    if (!clingo_string_builder_new(&bld.get())) {
        return py_handle_error();
    }
    if (!clingo_theory_base_atom_to_string(t->base, t->index, bld)) {
        return py_handle_error();
    }
    clingo_string_t str;
    if (!clingo_string_builder_string(bld, &str)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
}

auto py_theory_atom_get_name(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(self); // NOLINT
    clingo_id_t id = 0;
    if (!clingo_theory_base_atom_term(t->base, t->index, &id)) {
        return py_handle_error();
    }
    return py_theory_term_new(reinterpret_cast<PyTypeObject *>(state->theory_term_type), t->base, id);
}

auto py_theory_atom_get_elements(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(self); // NOLINT
    clingo_id_t const *ids = nullptr;
    size_t n = 0;
    if (!clingo_theory_base_atom_elements(t->base, t->index, &ids, &n)) {
        return py_handle_error();
    }
    auto out = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(n)));
    if (out.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < n; ++i) {
        auto *obj =
            py_theory_element_new(reinterpret_cast<PyTypeObject *>(state->theory_element_type), t->base, ids[i]);
        if (obj == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(out.get(), static_cast<Py_ssize_t>(i), obj);
    }
    return out.release();
}

auto py_theory_atom_get_literal(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(self); // NOLINT
    clingo_literal_t lit = 0;
    if (!clingo_theory_base_atom_literal(t->base, t->index, &lit)) {
        return py_handle_error();
    }
    return PyLong_FromLong(lit);
}

auto py_theory_atom_get_guard(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(self); // NOLINT
    bool has_guard = false;
    if (!clingo_theory_base_atom_has_guard(t->base, t->index, &has_guard)) {
        return py_handle_error();
    }
    if (!has_guard) {
        Py_RETURN_NONE;
    }

    clingo_string_t op;
    clingo_id_t term = 0;
    if (!clingo_theory_base_atom_guard(t->base, t->index, &op, &term)) {
        return py_handle_error();
    }

    auto op_obj = Guard<PyObject *, Py_DECREF>(PyUnicode_FromStringAndSize(op.data, static_cast<Py_ssize_t>(op.size)));
    auto term_obj = Guard<PyObject *, Py_DECREF>(
        py_theory_term_new(reinterpret_cast<PyTypeObject *>(state->theory_term_type), t->base, term));
    if (op_obj.get() == nullptr || term_obj.get() == nullptr) {
        return nullptr;
    }
    return PyTuple_Pack(2, op_obj.get(), term_obj.get());
}

auto py_theory_atom_getset = std::array{
    PyGetSetDef{"name", py_theory_atom_get_name, nullptr, PyDoc_STR("Name term."), nullptr},
    PyGetSetDef{"elements", py_theory_atom_get_elements, nullptr, PyDoc_STR("Elements."), nullptr},
    PyGetSetDef{"literal", py_theory_atom_get_literal, nullptr, PyDoc_STR("Literal."), nullptr},
    PyGetSetDef{"guard", py_theory_atom_get_guard, nullptr, PyDoc_STR("Guard or None."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_theory_atom_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("View over a theory atom."))},    // NOLINT
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_theory_atom_str)},                 // NOLINT
    PyType_Slot{Py_tp_hash, reinterpret_cast<void *>(py_theory_atom_hash)},               // NOLINT
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(py_theory_atom_richcompare)}, // NOLINT
    PyType_Slot{Py_tp_getset, py_theory_atom_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_theory_atom_spec = PyType_Spec{
    "clingo.base.TheoryAtom",    sizeof(PyTheoryAtomObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_theory_atom_slots.data(),
};

// TheoryBase (sequence)

/*
class TheoryBase {
  public:
    using value_type = TheoryAtom;

    explicit TheoryBase(clingo_theory_base_t const &base) : base_{&base} {}
    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto at(size_t index) const -> value_type;

  private:
    clingo_theory_base_t const *base_;
};
*/

auto py_theory_base_new(PyTypeObject *type, const clingo_theory_base_t *base) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryBaseObject *>(obj); // NOLINT
    t->base = base;
    return obj;
}

auto py_theory_base_len(PyObject *self) -> Py_ssize_t {
    auto *t = reinterpret_cast<PyTheoryBaseObject *>(self); // NOLINT
    size_t n = 0;
    if (!clingo_theory_base_size(t->base, &n)) {
        py_handle_error();
        return -1;
    }
    if (n > static_cast<size_t>(std::numeric_limits<Py_ssize_t>::max())) {
        PyErr_SetString(PyExc_OverflowError, "size does not fit in Py_ssize_t");
        return -1;
    }
    return static_cast<Py_ssize_t>(n);
}

auto py_theory_base_item(PyObject *self, Py_ssize_t i) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto n = py_theory_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    if (i < 0) {
        i += n;
    }
    if (i < 0 || i >= n) {
        PyErr_SetString(PyExc_IndexError, "atom index out of range");
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryBaseObject *>(self); // NOLINT
    return py_theory_atom_new(reinterpret_cast<PyTypeObject *>(state->theory_atom_type), t->base,
                              static_cast<size_t>(i));
}

auto py_theory_base_contains(PyObject *self, PyObject *value) -> int {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return -1;
    }
    if (!PyObject_TypeCheck(value, reinterpret_cast<PyTypeObject *>(state->theory_atom_type))) {
        return 0;
    }
    auto *a = reinterpret_cast<PyTheoryAtomObject *>(value); // NOLINT
    auto *t = reinterpret_cast<PyTheoryBaseObject *>(self);  // NOLINT
    size_t n = 0;
    if (!clingo_theory_base_size(t->base, &n)) {
        py_handle_error();
        return -1;
    }
    return (a->base == t->base && a->index < n) ? 1 : 0;
}

auto py_theory_base_iter(PyObject *self) -> PyObject * {
    auto n = py_theory_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(n));
    if (list.get() == nullptr) {
        return nullptr;
    }
    for (Py_ssize_t i = 0; i < n; ++i) {
        auto *obj = py_theory_base_item(self, i);
        if (obj == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(list.get(), i, obj);
    }
    return PyObject_GetIter(list.get());
}

auto py_theory_base_methods = std::array{
    PyMethodDef{
        nullptr,
        nullptr,
        0,
        nullptr,
    },
};

auto py_theory_base_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Sequence of theory atoms."))}, // NOLINT
    PyType_Slot{Py_tp_methods, py_theory_base_methods.data()},
    PyType_Slot{Py_sq_length, reinterpret_cast<void *>(py_theory_base_len)},        // NOLINT
    PyType_Slot{Py_sq_item, reinterpret_cast<void *>(py_theory_base_item)},         // NOLINT
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(py_theory_base_contains)}, // NOLINT
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_theory_base_iter)},         // NOLINT
    PyType_Slot{0, nullptr},
};

auto py_theory_base_spec = PyType_Spec{
    "clingo.base.TheoryBase",
    sizeof(PyTheoryBaseObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_MAPPING | Py_TPFLAGS_SEQUENCE,
    py_theory_base_slots.data(),
};

// Base

/*
class Base {
  public:
    using key_type = std::tuple<std::string_view, size_t, bool>;
    using mapped_type = AtomBase;
    using value_type = std::pair<key_type, mapped_type>;

    explicit Base(clingo_base_t const *base) : base_{base} {}

    [[nodiscard]] auto is_external(clingo_literal_t lit) const -> bool;
    [[nodiscard]] auto is_fact(clingo_literal_t lit) const -> bool;
    [[nodiscard]] auto is_shown(clingo_literal_t lit) const -> bool;
    [[nodiscard]] auto is_projected(clingo_literal_t lit) const -> bool;
    [[nodiscard]] auto is_current(clingo_literal_t lit) const -> bool;
    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto at(size_t index) const -> value_type;
    [[nodiscard]] auto contains(key_type const &sig) const -> bool;
    [[nodiscard]] auto contains_short(std::pair<std::string_view, size_t> const &sig) const -> bool;
    [[nodiscard]] auto contains_symbol(Symbol const &sym) const -> bool;
    [[nodiscard]] auto get(key_type const &sig, std::optional<mapped_type> def) const -> std::optional<mapped_type>;
    [[nodiscard]] auto lookup(key_type const &sig) const -> mapped_type;
    [[nodiscard]] auto lookup_short(std::pair<std::string_view, size_t> const &sig) const -> mapped_type;
    [[nodiscard]] auto lookup_symbol(Symbol const &sym) const -> Atom;
    [[nodiscard]] auto terms() const -> TermBase;
    [[nodiscard]] auto theory() const -> TheoryBase;

  private:
    clingo_base_t const *base_;
};
*/

auto py_base_new(PyTypeObject *type, const clingo_base_t *base) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *b = reinterpret_cast<PyBaseObject *>(obj); // NOLINT
    b->base = base;
    return obj;
}

auto py_base_len(PyObject *self) -> Py_ssize_t {
    auto *b = reinterpret_cast<PyBaseObject *>(self); // NOLINT
    size_t n = 0;
    if (!clingo_base_atoms_size(b->base, &n)) {
        py_handle_error();
        return -1;
    }
    if (n > static_cast<size_t>(std::numeric_limits<Py_ssize_t>::max())) {
        PyErr_SetString(PyExc_OverflowError, "size does not fit in Py_ssize_t");
        return -1;
    }
    return static_cast<Py_ssize_t>(n);
}

auto parse_signature_tuple(PyObject *tuple, clingo_signature_t *sig) -> int {
    auto n = PyTuple_Size(tuple);
    if (n != 2 && n != 3) {
        return 0;
    }

    auto *name_obj = PyTuple_GetItem(tuple, 0);                    // borrowed
    auto *arity_obj = PyTuple_GetItem(tuple, 1);                   // borrowed
    auto *sign_obj = n == 3 ? PyTuple_GetItem(tuple, 2) : Py_True; // borrowed

    if (!PyUnicode_Check(name_obj)) {
        return 0;
    }

    Py_ssize_t name_len = 0;
    const auto *name = PyUnicode_AsUTF8AndSize(name_obj, &name_len);
    if (name == nullptr) {
        return -1;
    }

    if (!PyLong_Check(arity_obj)) {
        return 0;
    }
    auto arity = PyLong_AsSize_t(arity_obj);
    if (PyErr_Occurred() != nullptr) {
        return -1;
    }

    int positive = PyObject_IsTrue(sign_obj);
    if (positive < 0) {
        return -1;
    }

    *sig = clingo_signature_t{name, static_cast<size_t>(name_len), arity, positive != 0};
    return 1;
}

auto parse_base_key(PyObject *key, clingo_signature_t *sig, clingo_symbol_t *sym_opt) -> int {
    if (PyTuple_Check(key) != 0) {
        return parse_signature_tuple(key, sig);
    }

    clingo_symbol_t sym = 0;
    auto is_sym = py_symbol_to_clingo_check(key, &sym);
    if (is_sym < 0) {
        return -1;
    }
    if (is_sym == 0) {
        return 0;
    }
    if (clingo_symbol_type(sym) != clingo_symbol_type_function) {
        return 0;
    }

    clingo_string_t name;
    clingo_symbol_t const *args = nullptr;
    size_t arity = 0;
    bool pos = false;
    if (!clingo_symbol_name(sym, &name) || !clingo_symbol_arguments(sym, &args, &arity) ||
        !clingo_symbol_is_positive(sym, &pos)) {
        py_handle_error();
        return -1;
    }

    *sig = clingo_signature_t{name.data, name.size, arity, pos};
    if (sym_opt != nullptr) {
        *sym_opt = sym;
    }
    return 1;
}

auto py_base_contains(PyObject *self, PyObject *key) -> int {
    auto *b = reinterpret_cast<PyBaseObject *>(self); // NOLINT
    clingo_signature_t sig{};
    clingo_symbol_t sym = 0;
    auto is_key = parse_base_key(key, &sig, &sym);
    if (is_key < 0) {
        return -1;
    }
    if (is_key == 0) {
        return 0;
    }

    clingo_atom_base_t const *atoms = nullptr;
    bool found = false;
    if (!clingo_base_atoms_find(b->base, &sig, &atoms, &found)) {
        py_handle_error();
        return -1;
    }
    if (!found || atoms == nullptr) {
        return 0;
    }

    if (PyTuple_Check(key) != 0) {
        return 1;
    }

    size_t idx = 0;
    if (!clingo_atom_base_find(atoms, sym, &idx)) {
        py_handle_error();
        return -1;
    }
    size_t n = 0;
    if (!clingo_atom_base_size(atoms, &n)) {
        py_handle_error();
        return -1;
    }
    return idx < n ? 1 : 0;
}

auto base_get(PyObject *self, PyObject *key, PyObject *def) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *b = reinterpret_cast<PyBaseObject *>(self); // NOLINT

    clingo_signature_t sig{};
    clingo_symbol_t sym = 0;
    auto is_key = parse_base_key(key, &sig, &sym);
    if (is_key < 0) {
        return nullptr;
    }
    if (is_key == 0) {
        PyErr_SetString(PyExc_TypeError, "key must be a signature tuple or a function symbol");
        return nullptr;
    }

    clingo_atom_base_t const *atoms = nullptr;
    bool found = false;
    if (!clingo_base_atoms_find(b->base, &sig, &atoms, &found)) {
        return py_handle_error();
    }
    if (!found || atoms == nullptr) {
        Py_INCREF(def);
        return def;
    }

    if (PyTuple_Check(key) != 0) {
        return py_atom_base_new(reinterpret_cast<PyTypeObject *>(state->atom_base_type), atoms);
    }

    size_t idx = 0;
    if (!clingo_atom_base_find(atoms, sym, &idx)) {
        return py_handle_error();
    }
    size_t n = 0;
    if (!clingo_atom_base_size(atoms, &n)) {
        return py_handle_error();
    }
    if (idx >= n) {
        Py_INCREF(def);
        return def;
    }
    return py_atom_new(reinterpret_cast<PyTypeObject *>(state->atom_type), atoms, idx);
}

auto py_base_get(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"key", "default", nullptr};
    PyObject *key = nullptr;
    PyObject *def = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:get", cast_kw(kwlist), &key, &def) == 0) {
        return nullptr;
    }
    return base_get(self, key, def);
}

auto py_base_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto *res = base_get(self, key, Py_None);
    if (res == nullptr) {
        return nullptr;
    }
    if (res == Py_None) {
        Py_DECREF(res);
        PyErr_SetString(PyExc_KeyError, "key not found");
        return nullptr;
    }
    return res;
}

auto py_base_keys(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto n = py_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto *b = reinterpret_cast<PyBaseObject *>(self); // NOLINT
    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(n));
    if (list.get() == nullptr) {
        return nullptr;
    }

    for (Py_ssize_t i = 0; i < n; ++i) {
        clingo_signature_t sig{};
        clingo_atom_base_t const *atoms = nullptr;
        if (!clingo_base_atoms_at(b->base, static_cast<size_t>(i), &sig, &atoms)) {
            return py_handle_error();
        }
        auto *k = Py_BuildValue("(s#nO)", sig.name, static_cast<Py_ssize_t>(sig.size),
                                static_cast<Py_ssize_t>(sig.arity), sig.is_positive ? Py_True : Py_False);
        if (k == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(list.get(), i, k);
    }
    return PyObject_GetIter(list.get());
}

auto py_base_iter(PyObject *self) -> PyObject * {
    return py_base_keys(self, nullptr);
}

auto py_base_values(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto n = py_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto *b = reinterpret_cast<PyBaseObject *>(self); // NOLINT
    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(n));
    if (list.get() == nullptr) {
        return nullptr;
    }

    for (Py_ssize_t i = 0; i < n; ++i) {
        clingo_signature_t sig{};
        clingo_atom_base_t const *atoms = nullptr;
        if (!clingo_base_atoms_at(b->base, static_cast<size_t>(i), &sig, &atoms)) {
            return py_handle_error();
        }
        auto *v = py_atom_base_new(reinterpret_cast<PyTypeObject *>(state->atom_base_type), atoms);
        if (v == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(list.get(), i, v);
    }
    return PyObject_GetIter(list.get());
}

auto py_base_items(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto n = py_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto *b = reinterpret_cast<PyBaseObject *>(self); // NOLINT
    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(n));
    if (list.get() == nullptr) {
        return nullptr;
    }

    for (Py_ssize_t i = 0; i < n; ++i) {
        clingo_signature_t sig{};
        clingo_atom_base_t const *atoms = nullptr;
        if (!clingo_base_atoms_at(b->base, static_cast<size_t>(i), &sig, &atoms)) {
            return py_handle_error();
        }

        auto key = Guard<PyObject *, Py_DECREF>(Py_BuildValue("(s#nO)", sig.name, static_cast<Py_ssize_t>(sig.size),
                                                              static_cast<Py_ssize_t>(sig.arity),
                                                              sig.is_positive ? Py_True : Py_False));
        auto val = Guard<PyObject *, Py_DECREF>(
            py_atom_base_new(reinterpret_cast<PyTypeObject *>(state->atom_base_type), atoms));
        if (key.get() == nullptr || val.get() == nullptr) {
            return nullptr;
        }
        auto item = Guard<PyObject *, Py_DECREF>(PyTuple_Pack(2, key.get(), val.get()));
        if (item.get() == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(list.get(), i, item.release());
    }
    return PyObject_GetIter(list.get());
}

auto base_predicate(PyObject *self, PyObject *args, PyObject *kwargs,
                    bool (*fun)(clingo_base_t const *, clingo_literal_t, bool *), char const *name) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"literal", nullptr};
    clingo_literal_t lit = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "i", cast_kw(kwlist), &lit) == 0) {
        return nullptr;
    }
    auto *b = reinterpret_cast<PyBaseObject *>(self); // NOLINT
    bool val = false;
    if (!fun(b->base, lit, &val)) {
        return py_handle_error();
    }
    (void)name;
    if (val) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_base_is_external(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    return base_predicate(self, args, kwargs, clingo_base_is_external, "is_external");
}

auto py_base_is_fact(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    return base_predicate(self, args, kwargs, clingo_base_is_fact, "is_fact");
}

auto py_base_is_shown(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    return base_predicate(self, args, kwargs, clingo_base_is_shown, "is_shown");
}

auto py_base_is_projected(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    return base_predicate(self, args, kwargs, clingo_base_is_projected, "is_projected");
}

auto py_base_is_current(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    return base_predicate(self, args, kwargs, clingo_base_is_current, "is_current");
}

auto py_base_get_terms(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *b = reinterpret_cast<PyBaseObject *>(self); // NOLINT
    clingo_term_base_t const *terms = nullptr;
    if (!clingo_base_terms(b->base, &terms)) {
        return py_handle_error();
    }
    return py_term_base_new(reinterpret_cast<PyTypeObject *>(state->term_base_type), terms);
}

auto py_base_get_theory(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *b = reinterpret_cast<PyBaseObject *>(self); // NOLINT
    clingo_theory_base_t const *theory = nullptr;
    if (!clingo_base_theory(b->base, &theory)) {
        return py_handle_error();
    }
    return py_theory_base_new(reinterpret_cast<PyTypeObject *>(state->theory_base_type), theory);
}

auto py_base_getset = std::array{
    PyGetSetDef{"terms", py_base_get_terms, nullptr, PyDoc_STR("Term base."), nullptr},
    PyGetSetDef{"theory", py_base_get_theory, nullptr, PyDoc_STR("Theory base."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_base_methods = std::array{
    PyMethodDef{
        "get",
        py_c_function_cast(py_base_get),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Get value or default."),
    },
    PyMethodDef{
        "keys",
        py_base_keys,
        METH_NOARGS,
        PyDoc_STR("Iterate keys."),
    },
    PyMethodDef{
        "values",
        py_base_values,
        METH_NOARGS,
        PyDoc_STR("Iterate values."),
    },
    PyMethodDef{
        "items",
        py_base_items,
        METH_NOARGS,
        PyDoc_STR("Iterate items."),
    },
    PyMethodDef{
        "is_external",
        py_c_function_cast(py_base_is_external),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check external."),
    },
    PyMethodDef{
        "is_fact",
        py_c_function_cast(py_base_is_fact),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check fact."),
    },
    PyMethodDef{
        "is_shown",
        py_c_function_cast(py_base_is_shown),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check shown."),
    },
    PyMethodDef{
        "is_projected",
        py_c_function_cast(py_base_is_projected),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check projected."),
    },
    PyMethodDef{
        "is_current",
        py_c_function_cast(py_base_is_current),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check current."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_base_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Base view."))}, // NOLINT
    PyType_Slot{Py_tp_methods, py_base_methods.data()},
    PyType_Slot{Py_tp_getset, py_base_getset.data()},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_base_len)},          // NOLINT
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_base_subscript)}, // NOLINT
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(py_base_contains)},   // NOLINT
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_base_iter)},           // NOLINT
    PyType_Slot{0, nullptr},
};

auto py_base_spec = PyType_Spec{
    "clingo.base.Base",
    sizeof(PyBaseObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_MAPPING | Py_TPFLAGS_SEQUENCE,
    py_base_slots.data(),
};

// TheoryTermType enum

using IntEnumMember = std::tuple<char const *, int, char const *>;
constexpr auto theory_term_members = std::array<IntEnumMember, 6>{
    IntEnumMember{"Number", static_cast<int>(clingo_theory_term_type_number), "For numeric theory terms."},
    IntEnumMember{"Symbol", static_cast<int>(clingo_theory_term_type_symbol), "For symbolic theory terms."},
    IntEnumMember{"Tuple", static_cast<int>(clingo_theory_term_type_tuple), "For tuple theory terms."},
    IntEnumMember{"List", static_cast<int>(clingo_theory_term_type_list), "For list theory terms."},
    IntEnumMember{"Set", static_cast<int>(clingo_theory_term_type_set), "For set theory terms."},
    IntEnumMember{"Function", static_cast<int>(clingo_theory_term_type_function), "For function theory terms."},
};

auto create_int_enum(const char *module_name, const char *enum_name, std::span<IntEnumMember const> members,
                     const char *enum_doc) -> PyObject * {
    auto enum_mod = Guard<PyObject *, Py_DECREF>(PyImport_ImportModule("enum"));
    if (enum_mod.get() == nullptr) {
        return nullptr;
    }
    auto int_enum = Guard<PyObject *, Py_DECREF>(PyObject_GetAttrString(enum_mod.get(), "IntEnum"));
    if (int_enum.get() == nullptr) {
        return nullptr;
    }
    auto name = Guard<PyObject *, Py_DECREF>(PyUnicode_FromString(enum_name));
    auto members_dict = Guard<PyObject *, Py_DECREF>(PyDict_New());
    if (name.get() == nullptr || members_dict.get() == nullptr) {
        return nullptr;
    }

    for (auto const &[member_name, member_value, member_doc] : members) {
        auto val = Guard<PyObject *, Py_DECREF>(PyLong_FromLong(member_value));
        if (val.get() == nullptr || PyDict_SetItemString(members_dict.get(), member_name, val.get()) < 0) {
            return nullptr;
        }
        (void)member_doc;
    }

    auto enum_obj = Guard<PyObject *, Py_DECREF>(
        PyObject_CallFunctionObjArgs(int_enum.get(), name.get(), members_dict.get(), nullptr));
    if (enum_obj.get() == nullptr) {
        return nullptr;
    }

    auto mod = Guard<PyObject *, Py_DECREF>(PyUnicode_FromString(module_name));
    auto doc = Guard<PyObject *, Py_DECREF>(PyUnicode_FromString(enum_doc));
    if (mod.get() == nullptr || doc.get() == nullptr) {
        return nullptr;
    }
    if (PyObject_SetAttrString(enum_obj.get(), "__module__", mod.get()) < 0 ||
        PyObject_SetAttrString(enum_obj.get(), "__doc__", doc.get()) < 0 ||
        PyObject_SetAttrString(enum_obj.get(), "__pybind11_native_enum__", Py_True) < 0) {
        return nullptr;
    }

    return enum_obj.release();
}

auto base_module_clear(PyObject *module) -> int {
    auto *state = static_cast<BaseModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_CLEAR(state->atom_type);
    Py_CLEAR(state->atom_base_type);
    Py_CLEAR(state->term_type);
    Py_CLEAR(state->term_base_type);
    Py_CLEAR(state->theory_term_type);
    Py_CLEAR(state->theory_element_type);
    Py_CLEAR(state->theory_atom_type);
    Py_CLEAR(state->theory_base_type);
    Py_CLEAR(state->base_type);
    Py_CLEAR(state->theory_term_type_enum);
    return 0;
}

void base_module_free(void *module) {
    base_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef base_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.base",
    PyDoc_STR("Functions and classes to inspect atom, term, and theory bases."),
    static_cast<Py_ssize_t>(sizeof(BaseModuleState)),
    nullptr,
    nullptr,
    nullptr,
    base_module_clear,
    base_module_free,
};

auto add_type(PyObject *module, const char *name, PyType_Spec *spec, PyObject **store) -> bool {
    auto obj = Guard<PyObject *, Py_DECREF>(PyType_FromModuleAndSpec(module, spec, nullptr));
    if (obj.get() == nullptr || PyModule_AddObjectRef(module, name, obj.get()) < 0) {
        return false;
    }
    *store = obj.release();
    return true;
}

} // namespace

void register_base(py::module &m) {
    auto base_mod = Guard<PyObject *, Py_DECREF>(PyModule_Create(&base_def));
    if (base_mod.get() == nullptr) {
        throw py::error_already_set();
    }

    auto *state = get_state(base_mod.get());
    if (state == nullptr) {
        throw py::error_already_set();
    }

    auto enum_obj = Guard<PyObject *, Py_DECREF>(
        create_int_enum("clingo.base", "TheoryTermType", theory_term_members, "Enumeration of theory term types."));
    if (enum_obj.get() == nullptr || PyModule_AddObjectRef(base_mod.get(), "TheoryTermType", enum_obj.get()) < 0) {
        throw py::error_already_set();
    }
    state->theory_term_type_enum = enum_obj.release();

    if (!add_type(base_mod.get(), "Atom", &py_atom_spec, &state->atom_type) ||
        !add_type(base_mod.get(), "AtomBase", &py_atom_base_spec, &state->atom_base_type) ||
        !add_type(base_mod.get(), "Term", &py_term_spec, &state->term_type) ||
        !add_type(base_mod.get(), "TermBase", &py_term_base_spec, &state->term_base_type) ||
        !add_type(base_mod.get(), "TheoryTerm", &py_theory_term_spec, &state->theory_term_type) ||
        !add_type(base_mod.get(), "TheoryElement", &py_theory_element_spec, &state->theory_element_type) ||
        !add_type(base_mod.get(), "TheoryAtom", &py_theory_atom_spec, &state->theory_atom_type) ||
        !add_type(base_mod.get(), "TheoryBase", &py_theory_base_spec, &state->theory_base_type) ||
        !add_type(base_mod.get(), "Base", &py_base_spec, &state->base_type)) {
        throw py::error_already_set();
    }

    auto *modules = PyImport_GetModuleDict();
    if (modules == nullptr || PyModule_AddObjectRef(m.ptr(), "base", base_mod.get()) < 0 ||
        PyDict_SetItemString(modules, "clingo.base", base_mod.get()) < 0) {
        throw py::error_already_set();
    }
}

auto py_base_type() -> PyTypeObject * {
    auto mod = Guard<PyObject *, Py_DECREF>(PyImport_ImportModule("clingo.base"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto *state = get_state(mod.get());
    if (state == nullptr || state->base_type == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<PyTypeObject *>(state->base_type);
}

auto py_base_from_clingo_object(clingo_base_t const *base) -> PyObject * {
    auto *type = py_base_type();
    if (type == nullptr) {
        return nullptr;
    }
    return py_base_new(type, base);
}

auto convert(TypeHint<"Base"> base, MixedLitSpan const &lits, bool flip, bool disjunctive) -> LitVec {
    auto *base_type = py_base_type();
    if (base_type == nullptr) {
        throw py::error_already_set();
    }
    if (PyObject_TypeCheck(base.ptr(), base_type) == 0) {
        throw py::type_error{"base must be an instance of clingo.base.Base"};
    }
    auto *base_obj = reinterpret_cast<PyBaseObject *>(base.ptr()); // NOLINT
    auto const *c_base = base_obj->base;

    auto res = LitVec{};
    res.reserve(lits.size());
    for (auto &&x : lits) {
        std::visit(
            [&]<typename T>(T const &x) {
                if constexpr (std::is_same_v<T, Lit_t>) {
                    res.emplace_back(flip ? -x : x);
                } else {
                    auto const &[sym, positive] = x;
                    auto pos = positive != flip;

                    auto const [name, arity, sign] = sym.signature().value();
                    auto csig = clingo_signature_t{name.data(), name.size(), arity, sign};

                    clingo_atom_base_t const *dom = nullptr;
                    bool found = false;
                    if (!clingo_base_atoms_find(c_base, &csig, &dom, &found)) {
                        py_handle_error();
                        throw py::error_already_set();
                    }

                    if (found && dom != nullptr) {
                        size_t index = 0;
                        if (!clingo_atom_base_find(dom, *c_cast(&sym), &index)) {
                            py_handle_error();
                            throw py::error_already_set();
                        }

                        size_t size = 0;
                        if (!clingo_atom_base_size(dom, &size)) {
                            py_handle_error();
                            throw py::error_already_set();
                        }

                        if (index < size) {
                            clingo_literal_t lit = 0;
                            if (!clingo_atom_base_literal(dom, index, &lit)) {
                                py_handle_error();
                                throw py::error_already_set();
                            }
                            res.emplace_back(pos ? lit : -lit);
                            return;
                        }
                    }

                    if (pos != disjunctive) {
                        res.emplace_back(1);
                        res.emplace_back(-1);
                    }
                }
            },
            x);
    }
    return res;
}

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables,cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-type-const-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)

} // namespace PyClingo
