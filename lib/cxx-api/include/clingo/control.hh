#pragma once

#include <clingo/backend.hh>
#include <clingo/base.hh>
#include <clingo/config.hh>
#include <clingo/core.hh>
#include <clingo/observe.hh>
#include <clingo/propagate.hh>
#include <clingo/solve.hh>
#include <clingo/stats.hh>
#include <clingo/symbol.hh>

#include <clingo/control.h>

#include <cassert>
#include <optional>
#include <span>

namespace Clingo {

namespace AST {

class Program;
auto c_cast(Program const &x) -> clingo_program_t *;

} // namespace AST

namespace Detail {

void join(clingo_control_t *ctx, clingo_program_t const *prg);

} // namespace Detail

struct Part {
    Part(std::string name, SymbolVector params = {}) : name{std::move(name)}, params(std::move(params)) {
        assert(!this->name.empty());
    }

    std::string name;
    SymbolVector params;
};
using PartSpan = std::span<Part const>;
using PartList = std::initializer_list<Part>;
using PartVector = std::vector<Part>;

class ConstMap {
  public:
    using key_type = std::string_view;
    using mapped_type = Symbol;
    using value_type = std::pair<key_type, mapped_type>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<ConstMap>;

    explicit ConstMap(clingo_const_map_t const *map) : map_{map} {}

    [[nodiscard]] auto contains(key_type name) const -> bool {
        return Detail::call<clingo_const_map_find>(map_, name.data(), name.size(), nullptr);
    }

    [[nodiscard]] auto operator[](key_type name) const -> mapped_type {
        clingo_symbol_t sym = 0;
        bool found = false;
        Detail::handle_error(clingo_const_map_find(map_, name.data(), name.size(), &sym, &found));
        return found ? Symbol{sym, true} : throw std::out_of_range{"key not found"};
    }

    [[nodiscard]] auto at(size_t index) const -> value_type {
        clingo_string_t name;
        clingo_symbol_t sym = 0;
        Detail::handle_error(clingo_const_map_at(map_, index, &name, &sym));
        return {{name.data, name.size}, Symbol{sym, true}};
    }

    [[nodiscard]] auto size() const -> size_type { return Detail::call<clingo_const_map_size>(map_); }

    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }

    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    clingo_const_map_t const *map_;
};

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
        ctl_.reset(Detail::call<clingo_control_new>(c_cast(lib), cstrs.data(), arguments.size()), false);
    }

    explicit Control(clingo_control_t *rep, bool acquire) : ctl_{rep, acquire} {}

    [[nodiscard]] friend auto c_cast(Control const &ctl) -> clingo_control_t * { return ctl.ctl_.get(); }

    [[nodiscard]] auto mode() const -> ControlMode {
        return static_cast<ControlMode>(Detail::call<clingo_control_mode>(ctl_.get()));
    }

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

    void ground(std::initializer_list<Part> parts, Context ctx = nullptr) const {
        ground(PartSpan{parts}, std::move(ctx));
    }

    [[nodiscard]] auto base() const -> Base { return Base{Detail::call<clingo_control_base>(ctl_.get())}; }

    [[nodiscard]] auto stats() const -> ConstStats {
        auto const *stats = Detail::call<clingo_control_stats>(ctl_.get());
        auto key = Detail::call<clingo_stats_root>(stats);
        return ConstStats{stats, key};
    }

    [[nodiscard]] auto solve(SolveEventHandler &handler, ProgramLiteralSpan const &assumptions = {},
                             SolveFlags flags = SolveFlags::empty) const -> SolveHandle {
        return solve_(&handler, assumptions, flags);
    }

    [[nodiscard]] auto solve(ProgramLiteralSpan const &assumptions = {}, SolveFlags flags = SolveFlags::yield) const
        -> SolveHandle {
        return solve_(nullptr, assumptions, flags);
    }

    void main() const { Detail::handle_error(clingo_control_main(ctl_.get())); }

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
        auto [data, size] = Detail::call<clingo_control_buffer>(ctl_.get());
        return {data, size};
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

    void parts(PartList parts) const { this->parts(std::span{parts.begin(), parts.end()}); }

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

    [[nodiscard]] auto config() const -> Config {
        auto *config = Detail::call<clingo_control_config>(ctl_.get());
        auto key = Detail::call<clingo_config_root>(config);
        return Config{config, key};
    }

    [[nodiscard]] auto const_map() const -> ConstMap {
        return ConstMap{Detail::call<clingo_control_const_map>(ctl_.get())};
    }

    void observe(Observer &obs, bool preprocess = true) const { obs.observe(ctl_.get(), preprocess); }

    [[nodiscard]] auto backend() const -> ProgramBackend {
        return ProgramBackend{Detail::call<clingo_control_backend>(ctl_.get())};
    }

    template <std::derived_from<Propagator> T> auto register_propagator(std::unique_ptr<T> propagator) const -> T & {
        assert(propagator != nullptr);
        auto &res = *propagator;
        Detail::handle_error(clingo_control_register_propagator(
            ctl_.get(), std::is_base_of_v<Heuristic, T> ? &Detail::c_heuristic : &Detail::c_propagator,
            propagator.release()));
        return res;
    }

    void join(AST::Program const &prg) const { Detail::join(ctl_.get(), c_cast(prg)); }

  private:
    friend class Detail::intrusive_handle<Control, clingo_control_t>;

    static auto ctx_([[maybe_unused]] clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *location,
                     char const *name, size_t name_size, clingo_symbol_t const *arguments, size_t arguments_size,
                     void *data, clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) -> bool {
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

    [[nodiscard]] auto solve_(SolveEventHandler *handler, ProgramLiteralSpan const &assumptions, SolveFlags flags) const
        -> SolveHandle {
        clingo_solve_handle_t *res = nullptr;
        Detail::handle_error(clingo_control_solve(ctl_.get(), static_cast<clingo_solve_mode_bitset_t>(flags),
                                                  assumptions.data(), assumptions.size(),
                                                  handler != nullptr ? &Detail::c_solve_event_handler : nullptr,
                                                  handler != nullptr ? handler : nullptr, &res));
        return SolveHandle{res};
    }

    Detail::intrusive_handle<Control, clingo_control_t> ctl_;
};

} // namespace Clingo
