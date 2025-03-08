#include <clingo/app.h>

#include "util.hh"

namespace Clingo::Python {

void register_app(pybind11::module &m) {
    using namespace Clingo::Python;

    auto app = m.def_submodule("app", R"(
TODO
)"_d);
}

} // namespace Clingo::Python
