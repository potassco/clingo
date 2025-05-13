
#include <clingo/base.h>

#include "base.hh"
#include "iterable.hh" // IWYU pragma: keep
#include "util.hh"

namespace PyClingo {

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
    auto atom = Atom{base_, index};
    return index < size() ? value_type{atom.symbol(), atom} : throw py::index_error{"index out of range"};
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
    auto term = Term{*base_, index};
    return index < size() ? value_type{term.symbol(), term} : throw py::index_error{"index out of range"};
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

auto TheoryElement::condition_id() -> clingo_literal_t {
    clingo_literal_t id = 0;
    handle_error(clingo_theory_base_element_condition_id(base_, index_, &id));
    return id;
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
    handle_error(clingo_base_is_fact(base_, literal, &projected));
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

auto convert(Base base, MixedLitSpan const &lits, bool flip) -> LitVec {
    return transform_vec(lits, [&](auto const &x) {
        return std::visit(
            [&]<typename T>(T const &x) {
                if constexpr (std::is_same_v<T, Lit_t>) {
                    return flip ? -x : x;
                } else {
                    auto const &[sym, positive] = x;
                    if (auto atom = base.lookup(sym.signature().value()).get(sym, std::nullopt)) {
                        return (positive != flip) ? atom->literal() : -atom->literal();
                    }
                    throw pybind11::key_error{"key not found"};
                }
            },
            x);
    });
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

    py::enum_<clingo_theory_term_type_e>(base, "TheoryTermType", "Enumeration of theory term types.")
        .value("Number", clingo_theory_term_type_number, R"(For numeric theory terms.)")
        .value("Symbol", clingo_theory_term_type_symbol, R"(For symbolic theory terms (simple strings).)")
        .value("Tuple", clingo_theory_term_type_tuple, R"(For tuple theory terms.)")
        .value("List", clingo_theory_term_type_list, R"(For list theory term.)")
        .value("Set", clingo_theory_term_type_set, R"(For set theory terms.)")
        .value("Function", clingo_theory_term_type_function, R"(For function theory terms.)");

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
                               R"(Get the condition id of a theory element.)");

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
