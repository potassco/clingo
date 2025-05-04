#pragma once

#include <clingo/base.hh>
#include <clingo/config.hh>
#include <clingo/core.hh>
#include <clingo/solve.hh>
#include <clingo/stats.hh>
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
using PartVector = std::vector<Part>;

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

enum class SolveFlags : clingo_solve_mode_bitset_t {
    empty = 0,
    yield = clingo_solve_mode_yield,
    async = clingo_solve_mode_async,
};
CLINGO_ENABLE_BITSET_ENUM(SolveFlags);

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

    [[nodiscard]] auto mode() const -> ControlMode {
        clingo_mode_t mode = 0;
        Detail::handle_error(clingo_control_mode(ctl_.get(), &mode));
        return static_cast<ControlMode>(mode);
    }

    // TODO: wrap program
    void join(clingo_program_t *prg) const { Detail::handle_error(clingo_control_join(ctl_.get(), prg)); }

    void write_aspif(std::string_view path, WriteAspifFlags flags = WriteAspifFlags::none) const {
        Detail::handle_error(clingo_control_write_aspif(ctl_.get(), path.data(), path.size(),
                                                        static_cast<clingo_write_aspif_mode_t>(flags)));
    }

    void parse_files(StringSpan files) const {
        auto cfiles = Detail::transform(files, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
        Detail::handle_error(clingo_control_parse_files(ctl_.get(), cfiles.data(), cfiles.size()));
    }

    void parse_files(StringList files) const { parse_files(StringSpan{files.begin(), files.end()}); }

    void parse_string(std::string_view program) const {
        Detail::handle_error(clingo_control_parse_string(ctl_.get(), program.data(), program.size()));
    }

    void ground(std::optional<PartSpan> parts = std::nullopt, Context ctx = nullptr) const {
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

    [[nodiscard]] auto base() const -> Base {
        clingo_base_t const *base = nullptr;
        clingo_control_base(ctl_.get(), &base);
        return {base};
    }

    [[nodiscard]] auto stats() const -> ConstStats {
        clingo_stats_t const *stats = nullptr;
        Detail::handle_error(clingo_control_stats(ctl_.get(), &stats));
        uint64_t key = 0;
        Detail::handle_error(clingo_stats_root(stats, &key));
        return ConstStats{stats, key};
    }

    [[nodiscard]] auto solve(SolveEventHandler &handler, LiteralSpan const &assumptions = {},
                             SolveFlags flags = SolveFlags::empty) const -> SolveHandle {
        return solve_(&handler, assumptions, flags);
    }

    [[nodiscard]] auto solve(LiteralSpan const &assumptions = {}, SolveFlags flags = SolveFlags::yield) const
        -> SolveHandle {
        return solve_(nullptr, assumptions, flags);
    }

    void main() const { Detail::handle_error(clingo_control_main(ctl_.get()), data_().ptr); }

    void interrupt() const { clingo_control_interrupt(ctl_.get()); }

    void discard(bool minimize, bool project) const {
        clingo_discard_type_t type = 0;
        if (minimize) {
            type |= clingo_discard_type_e::minimize;
        }
        if (project) {
            type |= clingo_discard_type_e::project;
        }
        Detail::handle_error(clingo_control_discard(ctl_.get(), type));
    }

    [[nodiscard]] auto buffer() const -> std::string_view {
        clingo_string_t ret;
        Detail::handle_error(clingo_control_buffer(ctl_.get(), &ret));
        return {ret.data, ret.size};
    }

    [[nodiscard]] auto parts() const -> std::optional<PartVector> {
        clingo_part_t const *parts = nullptr;
        size_t size = 0;
        bool has_value = false;
        Detail::handle_error(clingo_control_get_parts(ctl_.get(), &parts, &size, &has_value));
        if (has_value) {
            return Detail::transform(std::span{parts, size}, [](auto const &part) {
                auto params = std::span{cpp_cast(part.params), part.params_size};
                return Part{{part.name, part.name_size}, {params.begin(), params.end()}};
            });
        }
        return std::nullopt;
    }

    void parts(std::optional<PartSpan> parts) const {
        if (parts) {
            auto cparts = Detail::transform(*parts, [](auto const &part) {
                return clingo_part_t{part.name.data(), part.name.size(), c_cast(part.params.data()),
                                     part.params.size()};
            });
            Detail::handle_error(clingo_control_set_parts(ctl_.get(), cparts.data(), cparts.size(), true));
        } else {
            Detail::handle_error(clingo_control_set_parts(ctl_.get(), nullptr, 0, true));
        }
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

    auto const_map() -> HintConstMap {
        clingo_const_map_t const *map = nullptr;
        handle_error(clingo_control_const_map(ctl_.get(), &map));
        return py::cast(ConstMap{map});
    }

    void register_propagator(Annotation<Propagator> propagator) {
        auto &prop = propagator.cast<Propagator &>();
        user_data().append(std::move(propagator));
        Clingo::Python::register_propagator(ctl_.get(), prop);
    }
    */

  private:
    friend class Detail::ManagedPtr<Control, clingo_control_t>;

    struct Data {
        std::exception_ptr ptr;
    };
    static void free_data_(void *data) { std::ignore = std::unique_ptr<Data>(static_cast<Data *>(data)); }

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

    [[nodiscard]] auto data_() const -> Data & {
        auto *data = static_cast<Data *>(clingo_control_get_user_data(ctl_.get(), Detail::user_data_slot()));
        if (data == nullptr) {
            data = std::make_unique<Data>().release();
            Detail::handle_error(clingo_control_set_user_data(ctl_.get(), Detail::user_data_slot(), data, &free_data_));
        }
        return *data;
    }
    [[nodiscard]] auto solve_(SolveEventHandler *handler, LiteralSpan const &assumptions, SolveFlags flags) const
        -> SolveHandle {
        auto &ptr = data_().ptr;
        auto res = SolveHandle{ptr, handler};
        Detail::handle_error(clingo_control_solve(ctl_.get(), static_cast<clingo_solve_mode_bitset_t>(flags),
                                                  assumptions.data(), assumptions.size(),
                                                  handler != nullptr ? &SolveHandle::c_event_handler_ : nullptr,
                                                  handler != nullptr ? res.data_.get() : nullptr, &res.data_->hnd),
                             ptr);
        return res;
    }
    Detail::ManagedPtr<Control, clingo_control_t> ctl_;
};

} // namespace Clingo
