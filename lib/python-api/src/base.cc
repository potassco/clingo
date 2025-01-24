
#include <clingo/base.h>

#include "base.hh"
#include "util.hh"

namespace Clingo::Python {

auto Base::size() -> size_t {
    size_t size = 0;
    handle_error(clingo_base_atoms_size(&base_, &size));
    return size;
}

void register_base(pybind11::module &m) {
    auto base = m.def_submodule("base", doc(R"(
TODO)"));

    py::class_<Clingo::Python::Base>(base, "Base", R"(TODO.)").def("__len__", &Base::size, doc(R"(
Get the number of atom bases.
)"));
}

} // namespace Clingo::Python
