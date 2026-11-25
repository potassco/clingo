#include <clingo/util/algorithm.hh>

#include <clingo/control.h>
#include <clingo/script.h>

#include <clasp/cli/clasp_options.h>

#include <potassco/program_opts/program_options.h>
#include <potassco/program_opts/typed_value.h>

#include "ast.hh" // IWYU pragma: keep
#include "control.hh"
#include "lib.hh"
#include "opts.hh"

using namespace CppClingo::CAPI;

namespace CppClingo::CAPI {
namespace {

auto c_cast(CppClingo::Input::ConstMap const *map) {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_const_map_t const *>(map);
}

auto cpp_cast(clingo_const_map_t const *map) {
    // NOLINTNEXTLINE
    return reinterpret_cast<CppClingo::Input::ConstMap const *>(map);
}

} // namespace
} // namespace CppClingo::CAPI

extern "C" auto clingo_control_new(clingo_lib_t *lib, clingo_string_t const *arguments, size_t size,
                                   clingo_control_t **control) -> bool {
    CLINGO_TRY {
        using namespace Potassco::ProgramOptions;
        require(not_null(lib, control) && valid_span(arguments, size));

        auto opts = CppClingo::CAPI::ClingoOptions{lib->log, *lib->store};
        auto slv_cfg = std::make_unique<Clasp::Cli::ClaspCliConfig>();
        auto clasp = std::make_unique<Clasp::ClaspFacade>();

        // parse options
        auto ctx = OptionContext{"<libclingo>"};
        opts.init(ctx);

        auto parse_mode = [&opts](std::string_view str) { return opts.init_app_mode(str); };

        ctx.addOptions("Basic Options") //
            ("mode", parse(parse_mode),
             "Set run mode [default]\n"
             "      %A: {parse|rewrite|default}\n"
             "        parse  : Print parsed program and exit\n"
             "        rewrite: Print rewritten program and exit\n"
             "        default: Ground and solve the program");
        slv_cfg->addOptions(ctx);
        auto pos_parser = [](std::string_view str, std::string &out) {
            if (int value = 0; Potassco::stringTo(str, value) == std::errc{}) {
                out = "models";
                return true;
            }
            return false;
        };
        std::vector<std::string> bargs;
        std::vector<char const *> cargs;
        bargs.reserve(size);
        cargs.reserve(size + 1);
        for (auto const &str : std::span{arguments, size}) {
            require(valid_span(str.data, str.size));
            bargs.emplace_back(std::string_view{str.data, str.size});
            cargs.emplace_back(bargs.back().c_str());
        }
        cargs.emplace_back(nullptr);
        DefaultParseContext pc{ctx};
        parseCommandArray(pc, {cargs.data(), size}, pos_parser);
        opts.validate_options(pc.parsed());
        ctx.assignDefaults(pc.parsed());
        slv_cfg->finalize(pc.parsed(), Clasp::ProblemType::asp, true);

        // setup control
        if (opts.mode() == CppClingo::Control::AppMode::solve) {
            clasp->startAsp(*slv_cfg, !opts.solver_options().single_shot);
        }
        auto slv = std::make_unique<CppClingo::Control::Solver>(*clasp, *slv_cfg, lib->log, *lib->store, lib->scripts,
                                                                opts.solver_options(), nullptr);
        opts.apply(*slv);
        *control = new clingo_control{lib, std::move(slv), std::move(slv_cfg), std::move(clasp)};
    }
    CLINGO_CATCH;
}

extern "C" void clingo_control_acquire(clingo_control_t *control) {
    if (control != nullptr) {
        ++control->ref_count;
    }
}

extern "C" void clingo_control_release(clingo_control_t *control) {
    if (control != nullptr && --control->ref_count == 0) {
        delete control;
    }
}

extern "C" auto clingo_control_add_finalizer(clingo_control_t *control, clingo_finalizer_t finalizer, void *data)
    -> bool {
    CLINGO_TRY {
        require(not_null(control, finalizer));
        control->finalizers.emplace_back(finalizer, data);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_set_owner(clingo_control_t *control, void const *tag, void *owner) -> bool {
    CLINGO_TRY {
        require(not_null(control, tag));
        control->owners.set(tag, owner);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_get_owner(clingo_control_t const *control, void const *tag) -> void * {
    return control->owners.get(tag);
}

extern "C" auto clingo_control_mode(clingo_control_t *control, clingo_mode_t *mode) -> bool {
    CLINGO_TRY {
        require(not_null(control, mode));
        *mode = static_cast<clingo_mode_t>(control->slv->get_mode());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_parse_files(clingo_control_t *control, clingo_string_t const *files, size_t size)
    -> bool {
    CLINGO_TRY {
        require(control != nullptr && valid_span(files, size));
        auto vec = CppClingo::Util::to_vec(std::span{files, size}, [](auto const &x) {
            require(valid_span(x.data, x.size));
            return std::string_view{x.data, x.size};
        });
        control->slv->parse(vec);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_parse_string(clingo_control_t *control, char const *program, size_t size) -> bool {
    CLINGO_TRY {
        require(control != nullptr && valid_span(program, size));
        control->slv->parse({program, size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_join(clingo_control_t *control, clingo_program_t const *program) -> bool {
    CLINGO_TRY {
        require(not_null(control, program));
        control->slv->join(program->program);
    }
    CLINGO_CATCH;
}

extern "C" void clingo_control_interrupt(clingo_control_t *control) {
    control->slv->interrupt();
}

extern "C" auto clingo_control_main(clingo_control_t *control) -> bool {
    CLINGO_TRY {
        require(control != nullptr);
        control->slv->main();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_buffer(clingo_control_t *control, clingo_string_t *result) -> bool {
    CLINGO_TRY {
        require(not_null(control, result));
        auto str = control->slv->buf().view();
        result->data = str.data();
        result->size = str.size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_set_buffer(clingo_control_t *control, char const *value, size_t size) -> bool {
    CLINGO_TRY {
        require(control != nullptr && valid_span(value, size));
        auto &buf = control->slv->buf();
        buf.reset();
        buf << std::string_view{value, size};
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_get_parts(clingo_control_t *control, clingo_part_t const **parts, size_t *size,
                                         bool *has_value) -> bool {
    CLINGO_TRY {
        require(control != nullptr);
        auto const &x = control->slv->get_parts();
        if (has_value != nullptr) {
            *has_value = x.has_value();
        }
        if (x.has_value()) {
            if (parts != nullptr) {
                thread_local std::vector<clingo_part_t> res;
                res.clear();
                for (auto const &part : x->elems()) {
                    res.emplace_back(part.first->data(), part.first->size(), c_cast(part.second.data()),
                                     part.second.size());
                }
                *parts = res.data();
            }
            if (size != nullptr) {
                *size = x->elems().size();
            }
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_set_parts(clingo_control_t *control, clingo_part_t const *parts, size_t size,
                                         bool has_value) -> bool {
    CLINGO_TRY {
        require(control != nullptr);
        if (has_value) {
            control->slv->set_parts(convert(control, parts, size));
        } else {
            control->slv->set_parts(std::nullopt);
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_const_map(clingo_control_t *control, clingo_const_map_t const **map) -> bool {
    CLINGO_TRY {
        require(not_null(control, map));
        *map = c_cast(&control->slv->const_map());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_const_map_find(clingo_const_map_t const *map, char const *name, size_t size,
                                      clingo_symbol_t *symbol, bool *found) -> bool {
    CLINGO_TRY {
        require(map != nullptr && valid_span(name, size));
        auto const *cmap = cpp_cast(map);
        auto it = cmap->find(std::string_view{name, size});
        if (it != cmap->end()) {
            if (found != nullptr) {
                *found = true;
            }
            if (symbol != nullptr) {
                *symbol = *c_cast(&*it->second.second);
            }
        } else if (found != nullptr) {
            *found = false;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_const_map_at(clingo_const_map_t const *map, size_t index, clingo_string_t *name,
                                    clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        require(map != nullptr);
        require_index(index < cpp_cast(map)->size());
        auto const *cmap = cpp_cast(map);
        auto it = cmap->nth(index);
        if (symbol != nullptr) {
            *symbol = *c_cast(&*it->second.second);
        }
        if (name != nullptr) {
            name->data = it->first->data();
            name->size = it->first->size();
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_const_map_size(clingo_const_map_t const *map, size_t *size) -> bool {
    CLINGO_TRY {
        require(not_null(map, size));
        auto const *cmap = cpp_cast(map);
        *size = cmap->size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_discard(clingo_control_t *control, clingo_discard_type_t type) -> bool {
    CLINGO_TRY {
        require(control != nullptr && (type & ~static_cast<clingo_discard_type_t>(clingo_discard_type_minimize |
                                                                                  clingo_discard_type_project)) == 0);
        if ((type & clingo_discard_type_minimize) != 0) {
            control->clasp->asp()->removeMinimize();
        }
        if ((type & clingo_discard_type_project) != 0) {
            control->clasp->asp()->removeProject();
        }
    }
    CLINGO_CATCH;
}
