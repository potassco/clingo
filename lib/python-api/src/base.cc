
#include <clingo/base.h>

#include "base.hh"
#include "iterator.hh"
#include "util.hh"

namespace Clingo::Python {

auto AtomBase::size() -> size_t {
    size_t size = 0;
    handle_error(clingo_atom_base_size(&base_, &size));
    return size;
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

auto Base::at(std::tuple<char const *, size_t, bool> sig) -> AtomBase {
    auto csig = clingo_signature_t{get<0>(sig), get<1>(sig), get<2>(sig)};
    auto atoms = clingo_atom_base_t{};
    handle_error(clingo_base_atoms_find(&base_, &csig, &atoms));
    return AtomBase{atoms};
}

void register_base(pybind11::module &m) {
    auto base = m.def_submodule("base", doc(R"(
TODO)"));

    py::class_<Clingo::Python::AtomBase>(base, "AtomBase", R"(TODO.)").def("__len__", &AtomBase::size, doc(R"(
Get the number of atoms in the base.
)"));

    py::class_<Clingo::Python::Base>(base, "Base", R"(TODO.)")
        .def("__len__", &Base::size, doc(R"(
Get the number of atom bases.
)"))
        .def(
            "__iter__",
            [](Clingo::Python::Base &base) {
                return py::make_iterator(RandomAccessIterator{base, 0}, RandomAccessIterator{base, base.size()});
            },
            "TODO: document it");
}

} // namespace Clingo::Python
