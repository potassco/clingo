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

struct AppData;
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
        bool found = false;
        Detail::handle_error(clingo_const_map_find(map_, name.data(), name.size(), nullptr, &found));
        return found;
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

    [[nodiscard]] auto size() const -> size_type {
        size_t size = 0;
        Detail::handle_error(clingo_const_map_size(map_, &size));
        return size;
    }

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

    [[nodiscard]] auto config() const -> Config {
        clingo_config_t *config = nullptr;
        Detail::handle_error(clingo_control_config(ctl_.get(), &config));
        clingo_id_t key = 0;
        Detail::handle_error(clingo_config_root(config, &key));
        return Config{config, key};
    }

    [[nodiscard]] auto const_map() const -> ConstMap {
        clingo_const_map_t const *map = nullptr;
        Detail::handle_error(clingo_control_const_map(ctl_.get(), &map));
        return ConstMap{map};
    }

    void observe(Observer &obs, bool preprocess) const { obs.observe(ctl_.get(), preprocess); }

    [[nodiscard]] auto backend() const -> ProgramBackend {
        clingo_backend_t *bck = nullptr;
        Detail::handle_error(clingo_control_backend(ctl_.get(), &bck));
        return ProgramBackend{bck};
    }

    void register_propagator(std::unique_ptr<Propagator> propagator) const {
        auto &data = data_();
        data.props.emplace_back(std::move(propagator));
        Detail::handle_error(
            clingo_control_register_propagator(ctl_.get(), &Detail::c_propagator, data.props.back().get()));
    }

    void register_propagator(std::unique_ptr<Heuristic> heuristic) const {
        auto &data = data_();
        data.props.emplace_back(std::move(heuristic));
        Detail::handle_error(
            clingo_control_register_propagator(ctl_.get(), &Detail::c_heuristic, data.props.back().get()));
    }

    void join(AST::Program const &prg) const { Detail::join(ctl_.get(), c_cast(prg)); }

  private:
    // NOTE: Putting the user_data function into the Detail namespace would
    // avoid this.
    friend struct Detail::AppData;
    friend class Detail::intrusive_handle<Control, clingo_control_t>;

    struct Data {
        std::vector<std::unique_ptr<Propagator>> props;
    };

    static void free_data_(void *data) { std::ignore = std::unique_ptr<Data>(static_cast<Data *>(data)); }

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

    [[nodiscard]] auto data_() const -> Data & {
        auto *data = static_cast<Data *>(clingo_control_get_user_data(ctl_.get(), Detail::user_data_slot()));
        if (data == nullptr) {
            data = std::make_unique<Data>().release();
            Detail::handle_error(clingo_control_set_user_data(ctl_.get(), Detail::user_data_slot(), data, &free_data_));
        }
        return *data;
    }

    [[nodiscard]] auto solve_(SolveEventHandler *handler, ProgramLiteralSpan const &assumptions, SolveFlags flags) const
        -> SolveHandle {
        auto res = SolveHandle{handler};
        Detail::handle_error(clingo_control_solve(ctl_.get(), static_cast<clingo_solve_mode_bitset_t>(flags),
                                                  assumptions.data(), assumptions.size(),
                                                  handler != nullptr ? &SolveHandle::c_event_handler_ : nullptr,
                                                  handler != nullptr ? res.data_.get() : nullptr, &res.data_->hnd));
        return res;
    }

    Detail::intrusive_handle<Control, clingo_control_t> ctl_;
};

} // namespace Clingo
