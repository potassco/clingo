
#include <clingo/base.h>

#include "base.hh"
// #include "iterator.hh"
#include "util.hh"

namespace Clingo::Python {

auto Atom::literal() -> clingo_literal_t {
    auto lit = clingo_literal_t{0};
    handle_error(clingo_atom_base_literal(&base_, index_, &lit));
    return lit;
}

auto Atom::symbol() -> Symbol {
    auto sym = clingo_symbol_t{0};
    handle_error(clingo_atom_base_symbol(&base_, index_, &sym));
    return *cpp_cast(&sym);
}

auto Atom::external() -> bool {
    auto ext = false;
    handle_error(clingo_atom_base_is_external(&base_, index_, &ext));
    return ext;
}
auto Atom::fact() -> bool {
    auto fact = false;
    handle_error(clingo_atom_base_is_external(&base_, index_, &fact));
    return fact;
}

auto AtomBase::size() -> size_t {
    size_t size = 0;
    handle_error(clingo_atom_base_size(&base_, &size));
    return size;
}

auto AtomBase::at(size_t index) -> Atom { return Atom{base_, index}; }

auto AtomBase::lookup(Symbol const &symbol) -> Atom {
    static_cast<void>(this);
    static_cast<void>(symbol);
    throw std::logic_error("implement me: lookup");
}

auto Base::size() -> size_t {
    size_t size = 0;
    handle_error(clingo_base_atoms_size(&base_, &size));
    return size;
}

auto Base::at(size_t index) -> std::pair<std::tuple<std::string, size_t, bool>, AtomBase> {
    auto sig = clingo_signature_t{};
    auto atoms = clingo_atom_base_t{};
    handle_error(clingo_base_atoms_at(&base_, index, &sig, &atoms));
    return std::pair{std::tuple{std::string{sig.name}, sig.arity, sig.sign}, AtomBase{atoms}};
}

auto Base::lookup(std::tuple<char const *, size_t, bool> sig) -> AtomBase {
    auto csig = clingo_signature_t{get<0>(sig), get<1>(sig), get<2>(sig)};
    auto atoms = clingo_atom_base_t{};
    handle_error(clingo_base_atoms_find(&base_, &csig, &atoms));
    return AtomBase{atoms};
}

// TODO: terms

void register_base(pybind11::module &m) {
    using namespace Clingo::Python;

    auto base = m.def_submodule("base", doc(R"(
TODO)"));

    py::class_<Atom>(base, "Atom", R"(TODO.)")
        .def_property_readonly("literal", &Atom::literal, "Get the program literal of the atom.")
        .def_property_readonly("symbol", &Atom::symbol, "Get the symbol of the atom.")
        .def_property_readonly("external", &Atom::external, "Whether the atom is external.")
        .def_property_readonly("fact", &Atom::fact, "Whether the atom is a fact.");

    py::class_<AtomBase>(base, "AtomBase", R"(TODO.)")
        .def("__len__", &AtomBase::size, "Get the number of atoms in the base.")
        .def("lookup", &AtomBase::lookup, "TODO: document it")
        .def(
            "__getitem__",
            [](AtomBase &base, size_t index) { return index < base.size() ? base.at(index) : throw py::index_error{}; },
            "TODO: document it");

    py::class_<Base>(base, "Base", R"(TODO.)")
        .def("__len__", &Base::size, "Get the number of atom bases.")
        .def("lookup", &Base::lookup, "TODO: document it")
        .def(
            "__getitem__",
            [](Base &base, size_t index) { return index < base.size() ? base.at(index) : throw py::index_error{}; },
            "TODO: document it");
    /*
    .def(
        "__iter__",
        [](Base &base) {
            return py::make_iterator(RandomAccessIterator{base, 0}, RandomAccessIterator{base, base.size()});
        },
        "TODO: document it");
    */
}

} // namespace Clingo::Python
