#include "clingo.hh"
#include "core.hh" // IWYU pragma: keep
#include "script.hh"

#include <embed.h>

#include <string_view>

extern "C" auto clingo_register_python(clingo_lib_t *lib) -> bool {
    if (!PyClingo::register_python(lib)) {
        return false;
    }
    // NOTE: the module has to be added before the interpreter is initialized,
    // which happens when the first script is executed
    if (Py_IsInitialized() == 0 && PyImport_AppendInittab("clingo", PyClingo::init_clingo) < 0) {
        constexpr auto msg = std::string_view{"adding the clingo module to the python interpreter failed"};
        clingo_set_error(clingo_result_bad_alloc, msg.data(), msg.size());
        return false;
    }
    return true;
}
