#include <clingo/control/solver.hh>

#include "lib.hh"

// NOLINTBEGIN(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-bounds-pointer-arithmetic)

template <class It, class Pred> auto make_vec(It begin, It end, Pred pred) {
    auto p = std::vector<std::invoke_result_t<Pred, typename std::iterator_traits<It>::value_type>>{};
    p.reserve(std::distance(begin, end));
    std::transform(begin, end, std::back_inserter(p), pred);
    return p;
}

struct clingo_control {
    clingo_lib_t *lib;
    Clingo::Control::Solver slv;
};

extern "C" auto clingo_control_new(clingo_lib_t *lib, char const *const *arguments, size_t arguments_size,
                                   clingo_control_t **control) -> bool {
    CLINGO_TRY {
        // for now could use the main stuff
        static_cast<void>(arguments);
        static_cast<void>(arguments_size);
        auto opts = Clingo::Input::RewriteOptions{};
        *control = new clingo_control{
            lib, Clingo::Control::Solver{lib->log, *lib->store, opts, Clingo::Control::OutputMode::text}};
    }
    CLINGO_CATCH(lib);
}

extern "C" void clingo_control_free(clingo_control_t *control) { delete control; }

extern "C" auto clingo_control_parse_files(clingo_control_t *control, char const **files, size_t files_size) -> bool {
    CLINGO_TRY { control->slv.parse(std::vector<std::string_view>{files, files + files_size}); }
    CLINGO_CATCH(control->lib);
}

extern "C" auto clingo_control_parse_string(clingo_control_t *control, char const *program) -> bool {
    CLINGO_TRY { control->slv.parse(program); }
    CLINGO_CATCH(control->lib);
}

extern "C" auto clingo_control_ground(clingo_control_t *control, clingo_part_t const *parts,
                                      size_t parts_size) -> bool {
    CLINGO_TRY {
        auto make_part = [&](auto const &sym) { return Clingo::SharedSymbol::from_rep(sym, true); };
        auto make_parts = [&](auto const &part) {
            return Clingo::Input::ProgramParam{control->lib->store->string(part.name),
                                               make_vec(part.params, part.params + part.size, make_part)};
        };
        std::ignore = control->slv.ground(make_vec(parts, parts + parts_size, make_parts));
    }
    CLINGO_CATCH(control->lib);
}

// NOLINTEND(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-bounds-pointer-arithmetic)
