#include <clingo/control/solver.hh>

#include "lib.hh"

// NOLINTBEGIN(cppcoreguidelines-owning-memory)

struct clingo_control {
    Clingo::Control::Solver slv;
};

extern "C" auto clingo_control_new(clingo_lib_t *lib, char const *const *arguments, size_t arguments_size,
                                   clingo_control_t **control) -> bool {
    CLINGO_TRY {
        static_cast<void>(arguments);
        static_cast<void>(arguments_size);
        auto opts = Clingo::Input::RewriteOptions{};
        *control =
            new clingo_control{Clingo::Control::Solver{lib->log, *lib->store, opts, Clingo::Control::OutputMode::text}};
    }
    CLINGO_CATCH(lib);
}

extern "C" void clingo_control_free(clingo_control_t *control) { delete control; }

// NOLINTEND(cppcoreguidelines-owning-memory)
