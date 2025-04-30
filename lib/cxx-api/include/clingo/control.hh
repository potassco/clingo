#pragma once

#include <clingo/base.hh>
#include <clingo/core.hh>
#include <clingo/symbol.hh>

#include <clingo/ast.h>
#include <clingo/control.h>
#include <clingo/observe.h>

#include <cassert>
#include <optional>
#include <span>

namespace Clingo {

using StringSpan = std::span<std::string_view const>;
using StringList = std::initializer_list<std::string_view const>;

struct Part {
    Part(std::string name, SymbolVector params = {}) : name{std::move(name)}, params(std::move(params)) {
        assert(!this->name.empty());
    }
    std::string name;
    SymbolVector params;
};
using PartSpan = std::span<Part const>;

enum class ControlMode : clingo_mode_t {
    parse = clingo_mode_parse,     //!< parse only
    rewrite = clingo_mode_rewrite, //!< parse and rewrite
    ground = clingo_mode_ground,   //!< parse, rewrite, ground
    solve = clingo_mode_solve,     //!< parse, rewrite, ground, and solve
};

enum class WriteAspifFlags : clingo_write_aspif_mode_t {
    none = 0,                                              //!< No flags.
    preamble = clingo_write_aspif_mode_preamble,           //!< Write preamble.
    preamble_auto = clingo_write_aspif_mode_preamble_auto, //!< Write preamble for newly created files.
    append = clingo_write_aspif_mode_append,               //!< Append to an existing file (or create it).
    preprocess = clingo_write_aspif_mode_preprocess,       //!< Whether to preprocess the program before writing.
    symbols = clingo_write_aspif_mode_symbols,             //!< Whether to write symbols in a structured format.
};
CLINGO_ENABLE_BITSET_ENUM(WriteAspifFlags);

class Control {
  public:
    using Context = std::function<SymbolVector(std::string_view, SymbolSpan)>;

    Control(Library const &lib, StringList arguments) : Clingo::Control{lib, StringSpan{arguments}} {}
    Control(Library const &lib, StringSpan arguments = {}) {
        auto cstrs = Detail::transform(arguments, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
        clingo_control_t *ptr = nullptr;
        Detail::handle_error(clingo_control_new(c_cast(lib), cstrs.data(), arguments.size(), &ptr));
        ctl_.reset(ptr, false);
    }
    explicit Control(clingo_control_t *rep, bool acquire) : ctl_{rep, acquire} {}

    [[nodiscard]] friend auto c_cast(Control const &ctl) -> clingo_control_t * { return ctl.ctl_.get(); }

    auto mode() -> ControlMode {
        clingo_mode_t mode = 0;
        Detail::handle_error(clingo_control_mode(ctl_.get(), &mode));
        return static_cast<ControlMode>(mode);
    }

    // TODO: wrap program
    void join(clingo_program_t *prg) { Detail::handle_error(clingo_control_join(ctl_.get(), prg)); }

    void write_aspif(std::string_view path, WriteAspifFlags flags = WriteAspifFlags::none) {
        Detail::handle_error(clingo_control_write_aspif(ctl_.get(), path.data(), path.size(),
                                                        static_cast<clingo_write_aspif_mode_t>(flags)));
    }

    void parse_files(StringSpan files) {
        auto cfiles = Detail::transform(files, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
        Detail::handle_error(clingo_control_parse_files(ctl_.get(), cfiles.data(), cfiles.size()));
    }

    void parse_files(StringList files) { parse_files(StringSpan{files.begin(), files.end()}); }

    void parse_string(std::string_view program) {
        Detail::handle_error(clingo_control_parse_string(ctl_.get(), program.data(), program.size()));
    }

    void ground(std::optional<PartSpan> parts = std::nullopt, Context ctx = nullptr) {
        std::vector<clingo_part_t> c_parts;
        if (parts) {
            c_parts.reserve(parts->size());
            for (auto const &part : *parts) {
                c_parts.emplace_back(part.name.data(), part.name.size(), c_cast(part.params.data()),
                                     part.params.size());
            }
        } else {
            c_parts.reserve(1);
            c_parts.emplace_back("base", 4, nullptr, 0);
        }
        Detail::handle_error(
            clingo_control_ground(ctl_.get(), c_parts.data(), c_parts.size(), ctx ? &ctx_ : nullptr, &ctx));
    }

    auto base() -> Base {
        clingo_base_t const *base = nullptr;
        clingo_control_base(ctl_.get(), &base);
        return {base};
    }

    /*
    void observe(Observer &obs, bool preprocess) {
        obs.observe(ctl_.get(), preprocess);
    }

    auto backend() -> BackendManager {
        return BackendManager{ctl_.get()};
    }

    auto config() -> Config {
        clingo_config_t *config = nullptr;
        handle_error(clingo_control_config(ctl_.get(), &config));
        clingo_id_t key = 0;
        handle_error(clingo_config_root(config, &key));
        return Config{config, key};
    }

    auto stats() -> py::dict {
        clingo_stats_t const *stats = nullptr;
        handle_error(clingo_control_stats(ctl_.get(), &stats));
        uint64_t key = 0;
        handle_error(clingo_stats_root(stats, &key));
        // NOLINTNEXTLINE
        return Stats{const_cast<clingo_stats_t *>(stats), key}.nestify();
    }

    auto solve(MixedLitSpan const &assumptions, std::optional<ModelCallback> on_model,
                        std::optional<StatsCallback> on_stats, bool yield, bool async) -> SSolveHandle {
        auto release = py::gil_scoped_release{};
        auto res = std::make_shared<SolveHandle>(std::move(on_model), std::move(on_stats));
        auto mode = clingo_solve_mode_bitset_t{0};
        if (yield) {
            mode |= clingo_solve_mode_yield;
        }
        if (async) {
            mode |= clingo_solve_mode_async;
        }
        auto ass = convert(base(), assumptions, false);
        handle_error(clingo_control_solve(ctl_.get(), mode, ass.data(), assumptions.size(),
    &SolveHandle::c_event_handler, res.get(), &res->handle()), get_exception_ptr()); return res;
    }

    void main() {
        auto release = py::gil_scoped_release{};
        handle_error(clingo_control_main(ctl_.get()), get_exception_ptr());
    }

    void interrupt() {
        clingo_control_interrupt(ctl_.get());
    }

    void discard(bool minimize, bool project) {
        clingo_discard_type_t type = 0;
        if (minimize) {
            type |= clingo_discard_type_e::minimize;
        }
        if (project) {
            type |= clingo_discard_type_e::project;
        }
        handle_error(clingo_control_discard(ctl_.get(), type));
    }

    auto buffer() -> char const * {
        char const *ret = nullptr;
        handle_error(clingo_control_buffer(ctl_.get(), &ret));
        return ret;
    }

    auto const_map() -> HintConstMap {
        clingo_const_map_t const *map = nullptr;
        handle_error(clingo_control_const_map(ctl_.get(), &map));
        return py::cast(ConstMap{map});
    }

    auto parts() -> std::optional<PartSpan> {
        clingo_part_t const *parts = nullptr;
        size_t size = 0;
        bool has_value = false;
        handle_error(clingo_control_get_parts(ctl_.get(), &parts, &size, &has_value));
        if (!has_value) {
            return std::nullopt;
        }
        return PartSpan{parts, size};
    }

    void set_parts(std::optional<PartSpan> parts) {
        if (parts) {
            handle_error(clingo_control_set_parts(ctl_.get(), parts->data(), parts->size(), true));
        } else {
            handle_error(clingo_control_set_parts(ctl_.get(), nullptr, 0, true));
        }
    }

    void register_propagator(Annotation<Propagator> propagator) {
        auto &prop = propagator.cast<Propagator &>();
        user_data().append(std::move(propagator));
        Clingo::Python::register_propagator(ctl_.get(), prop);
    }
    */

  private:
    friend class Detail::ManagedPtr<Control, clingo_control_t>;

    static auto ctx_([[maybe_unused]] clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *location,
                     char const *name, size_t name_size, clingo_symbol_t const *arguments, size_t arguments_size,
                     void *data, clingo_symbol_callback_t symbol_callback, void *symbol_callback_data)
        -> clingo_result_t {
        CLINGO_TRY {
            auto &cb = *static_cast<std::function<SymbolVector(std::string_view, SymbolSpan)> *>(data);
            auto syms = cb({name, name_size}, {cpp_cast(arguments), arguments_size});
            auto const *c_syms = c_cast(syms.data());
            return symbol_callback(c_syms, syms.size(), symbol_callback_data);
        }
        CLINGO_CATCH;
    }

    static auto acquire(clingo_control_t *ptr) { clingo_control_acquire(ptr); }

    static auto release(clingo_control_t *ptr) { clingo_control_release(ptr); }

    Detail::ManagedPtr<Control, clingo_control_t> ctl_;
};

} // namespace Clingo
