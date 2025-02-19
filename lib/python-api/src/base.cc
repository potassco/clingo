
#include <clingo/base.h>

#include "base.hh"
#include "iterable.hh" // IWYU pragma: keep
#include "util.hh"

namespace Clingo::Python {

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

auto AtomBase::size() -> size_t {
    size_t size = 0;
    handle_error(clingo_atom_base_size(base_, &size));
    return size;
}

auto AtomBase::at(size_t index) -> value_type {
    auto atom = Atom{base_, index};
    return index < size() ? value_type{atom.symbol(), atom} : throw py::index_error{"index out of range"};
}

auto AtomBase::lookup(key_type const &symbol) -> mapped_type {
    auto index = size_t{0};
    handle_error(clingo_atom_base_find(base_, *c_cast(&symbol), &index));
    return index < size() ? Atom{base_, index} : throw py::key_error{"key not found"};
}

auto AtomBase::contains(key_type const &symbol) -> bool {
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

auto Term::condition() -> std::optional<std::span<clingo_literal_t const>> {
    clingo_literal_t *lits = nullptr;
    size_t size = 0;
    handle_error(clingo_term_base_condition(base_, index_, &lits, &size));
    if (lits == nullptr) {
        return std::nullopt;
    }
    return std::make_optional<std::span<clingo_literal_t const>>(lits, size);
}

// TermBase

auto TermBase::size() -> size_t {
    size_t size = 0;
    handle_error(clingo_term_base_size(base_, &size));
    return size;
}

auto TermBase::at(size_t index) -> value_type {
    auto term = Term{*base_, index};
    return index < size() ? value_type{term.symbol(), term} : throw py::index_error{"index out of range"};
}

auto TermBase::contains(key_type const &symbol) -> bool {
    auto index = size_t{0};
    handle_error(clingo_term_base_find(base_, *c_cast(&symbol), &index));
    return index < size();
}

auto TermBase::lookup(key_type const &symbol) -> mapped_type {
    auto index = size_t{0};
    handle_error(clingo_term_base_find(base_, *c_cast(&symbol), &index));
    return index < size() ? Term{*base_, index} : throw py::key_error("key does not exist");
}

// TheoryTerm

auto TheoryTerm::name() -> char const * {
    char const *name = nullptr;
    handle_error(clingo_theory_base_term_name(base_, index_, &name));
    return name;
}

// TheoryElement

auto TheoryElement::tuple() -> TheoryTermVec {
    size_t size = 0;
    clingo_id_t const *tuple = nullptr;
    handle_error(clingo_theory_base_element_tuple(base_, index_, &tuple, &size));
    auto res = TheoryTermVec{};
    res.reserve(size);
    std::ranges::transform(std::span{tuple, size}, std::back_inserter(res),
                           [this](clingo_id_t id) { return TheoryTerm{*base_, id}; });
    return res;
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

// TheoryAtom

auto TheoryAtom::name() -> TheoryTerm {
    clingo_id_t id = 0;
    handle_error(clingo_theory_base_atom_term(base_, index_, &id));
    return TheoryTerm{*base_, id};
}

auto TheoryAtom::elements() -> TheoryElementVec {
    size_t size = 0;
    clingo_id_t const *elems = nullptr;
    handle_error(clingo_theory_base_atom_elements(base_, index_, &elems, &size));
    auto res = TheoryElementVec{};
    res.reserve(size);
    std::ranges::transform(std::span{elems, size}, std::back_inserter(res),
                           [this](clingo_id_t id) { return TheoryElement{*base_, id}; });
    return res;
}

// TheoryBase

auto TheoryBase::at(size_t index) -> value_type {
    return index < size() ? TheoryAtom{*base_, index} : throw std::range_error{"atom index out of range"};
}

auto TheoryBase::size() -> size_t {
    auto size = size_t{0};
    handle_error(clingo_theory_base_size(base_, &size));
    return size;
}

// Base

auto Base::is_external(clingo_literal_t literal) -> bool {
    auto ext = false;
    handle_error(clingo_base_is_external(base_, literal, &ext));
    return ext;
}
auto Base::is_fact(clingo_literal_t literal) -> bool {
    auto fact = false;
    handle_error(clingo_base_is_fact(base_, literal, &fact));
    return fact;
}

auto Base::size() -> size_t {
    size_t size = 0;
    handle_error(clingo_base_atoms_size(base_, &size));
    return size;
}

auto Base::at(size_t index) -> value_type {
    auto sig = clingo_signature_t{};
    clingo_atom_base_t const *atoms = nullptr;
    if (index < size()) {
        handle_error(clingo_base_atoms_at(base_, index, &sig, &atoms));
        return std::pair{std::tuple{sig.name, sig.arity, sig.sign}, AtomBase{atoms}};
    }
    throw py::index_error{"index out of range"};
}

auto Base::contains_short(std::pair<char const *, size_t> const &sig) -> bool {
    return contains({get<0>(sig), get<1>(sig), false});
}

auto Base::contains(key_type const &sig) -> bool {
    auto csig = clingo_signature_t{get<0>(sig), get<1>(sig), get<2>(sig)};
    auto found = false;
    handle_error(clingo_base_atoms_find(base_, &csig, nullptr, &found));
    return found;
}

auto Base::lookup_short(std::pair<char const *, size_t> const &sig) -> mapped_type {
    return lookup({get<0>(sig), get<1>(sig), false});
}

auto Base::lookup(key_type const &sig) -> mapped_type {
    auto csig = clingo_signature_t{get<0>(sig), get<1>(sig), get<2>(sig)};
    clingo_atom_base_t const *atoms = nullptr;
    auto found = false;
    handle_error(clingo_base_atoms_find(base_, &csig, &atoms, &found));
    return found ? AtomBase{atoms} : throw py::key_error("key does not exist");
}

auto Base::terms() -> TermBase {
    auto const *terms = static_cast<clingo_term_base_t const *>(nullptr);
    handle_error(clingo_base_terms(base_, &terms));
    return TermBase{*terms};
}

auto Base::theory() -> TheoryBase {
    auto const *base = static_cast<clingo_theory_base_t const *>(nullptr);
    handle_error(clingo_base_theory(base_, &base));
    return TheoryBase{*base};
}

auto convert(Base base, MixedLitlVec const &lits) -> LitVec {
    LitVec res;
    res.reserve(lits.size());
    std::ranges::transform(lits, std::back_inserter(res), [&](auto const &x) {
        return std::visit(
            [&]<typename T>(T const &x) {
                if constexpr (std::is_same_v<T, Lit_t>) {
                    return x;
                } else {
                    auto const &[sym, positive] = x;
                    auto lit = base.lookup(sym.signature().value()).lookup(sym).literal();
                    return positive ? lit : -lit;
                }
            },
            x);
    });
    return res;
}

void register_base(pybind11::module &m) {
    using namespace Clingo::Python;

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

    py::class_<Atom>(base, "Atom", R"(A class providing information about symbolic atoms.)")
        .def_property_readonly("literal", &Atom::literal, "Get the program literal of the atom.")
        .def_property_readonly("symbol", &Atom::symbol, "Get the symbol of the atom.");

    py::class_<AtomBase>(base, "AtomBase", R"(An atom base mapping symbols to atoms.)")
        .def("__len__", &AtomBase::size, "Get the number of atoms in the base.")
        .def("__getitem__", &AtomBase::lookup, R"( Get the atom with the given symbol.)")
        .def("__contains__", &AtomBase::contains, R"( Check if the base contains an atom with the given symbol.)")
        .def(
            "__iter__", [](AtomBase &base) { return py::make_key_iterator(base.begin(), base.end()); },
            "Get an iterable over the keys in the map.")
        .def(
            "items", [](AtomBase &base) { return py::make_iterator(base.begin(), base.end()); },
            R"(Get an iterator over the items in the map.)")
        .def(
            "values", [](AtomBase &base) { return py::make_value_iterator(base.begin(), base.end()); },
            R"(Get an iterator over the values in the map.)")
        .def(
            "keys", [](AtomBase &base) { return py::make_key_iterator(base.begin(), base.end()); },
            R"(Get get an iterator over the keys in the map.)");

    py::class_<Term>(base, "Term", R"(A class providing information about terms.)")
        .def_property_readonly("symbol", &Term::symbol, "Get the symbol of the term.")
        .def_property_readonly("condition", &Term::condition, "Get the condition of the term.");

    py::class_<TermBase>(base, "TermBase", R"(
A base to inspect show term directives in a program.

The base is established by the show directives occurring in a program.)"_d)
        .def("__len__", &TermBase::size, "Get the number of terms in the base.")
        .def("__getitem__", &TermBase::lookup, R"(Get the term with the given symbol.)")
        .def("__contains__", &TermBase::contains, R"( Check if the base contains a term with the given symbol.)")
        .def(
            "__iter__", [](TermBase &base) { return py::make_key_iterator(base.begin(), base.end()); },
            "Get an iterator over the keys in the map.")
        .def(
            "items", [](TermBase &base) { return py::make_iterator(base.begin(), base.end()); },
            R"(Get an iterator over the items in the map.)")
        .def(
            "values", [](TermBase &base) { return py::make_value_iterator(base.begin(), base.end()); },
            R"(Get an iterator over the values in the map.)")
        .def(
            "keys", [](TermBase &base) { return py::make_key_iterator(base.begin(), base.end()); },
            R"(Get get an iterator over the keys in the map.)");

    py::class_<TheoryTerm>(base, "TheoryTerm", R"(A view to inspect a theory term.)")
        .def_property_readonly("name", &TheoryTerm::name, R"(Get the name of a theory function.)");

    py::class_<TheoryElement>(base, "TheoryElement", R"(A view to inspect a theory element.)")
        .def_property_readonly("tuple", &TheoryElement::tuple, R"(Get the term tuple of a theory element.)")
        .def_property_readonly("condition", &TheoryElement::condition, R"(Get the condition of a theory element.)")
        .def_property_readonly("condition_id", &TheoryElement::condition_id,
                               R"(Get the condition id of a theory element.)");

    py::class_<TheoryAtom>(base, "TheoryAtom", R"(A view to inspect a theory atom.)")
        .def_property_readonly("name", &TheoryAtom::name, R"(Get the name of a theory atom.)")
        .def_property_readonly("elements", &TheoryAtom::elements, R"(Get the elements of a theory atom.)");

    py::class_<TheoryBase>(base, "TheoryBase", R"(
A base to inspect theory atoms.)"_d)
        .def("__len__", &TheoryBase::size, "Get the number of theory atoms in the base.")
        .def("__getitem__", &TheoryBase::at, R"(Get the atom with the given index.)")
        .def(
            "__iter__", [](TheoryBase &base) { return py::make_iterator(base.begin(), base.end()); },
            "Get an iterable over the keys in the map.");

    py::class_<Base>(base, "Base", R"(
The base provides information about atoms and show term directives occuring in a program.

It implements a map from signatures to atom bases.
)")
        .def("__len__", &Base::size, "Get the number of atom bases.")
        .def("__getitem__", &Base::lookup_short, R"(
Get the element with the given (short) signature.

This function provides a shortcut assuming the sign is false.
)"_d)
        .def("__getitem__", &Base::lookup, R"(Get the atom base with the given signature.)")
        .def("__contains__", &Base::contains, R"( Check if there is an atom base with the given signature.)")
        .def("__contains__", &Base::contains_short,
             R"( Check if there is an atom base with the given (short) signature.)")
        .def(
            "__iter__", [](Base &base) { return py::make_key_iterator(base.begin(), base.end()); },
            "Get an iterator over the keys in the map.")
        .def(
            "items", [](Base &base) { return py::make_iterator(base.begin(), base.end()); },
            R"(Get an iterator over the items in the map.)")
        .def(
            "values", [](Base &base) { return py::make_value_iterator(base.begin(), base.end()); },
            R"(Get an iterator over the values in the map.)")
        .def(
            "keys", [](Base &base) { return py::make_key_iterator(base.begin(), base.end()); },
            R"(Get get an iterator over the keys in the map.)")
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
        .def_property_readonly("terms", &Base::terms, "The term base (given by show directives).")
        .def_property_readonly("theory", &Base::theory, "The theory base.");
}

} // namespace Clingo::Python
