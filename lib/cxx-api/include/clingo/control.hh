#pragma once

#include <clingo/backend.hh>
#include <clingo/base.hh>
#include <clingo/config.hh>
#include <clingo/core.hh>
#include <clingo/ground.hh>
#include <clingo/observe.hh>
#include <clingo/profile.hh>
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

//! @addtogroup cpp_control
//! Functions to control the grounding and solving process.
//!
//! @{

//! Class to providing a view on the const directives in a logic program.
//!
//! The class models a map from parameter names to their values.
class ConstMap {
  public:
    //! The key type.
    using key_type = std::string_view;
    //! The mapped type.
    using mapped_type = Symbol;
    //! The value type.
    using value_type = std::pair<key_type, mapped_type>;
    //! The size type.
    using size_type = std::size_t;
    //! The difference type.
    using difference_type = std::ptrdiff_t;
    //! The reference type.
    using reference = value_type;
    //! The pointer type.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type.
    using iterator = Detail::RandomAccessIterator<ConstMap>;

    //! Construct from the underlying C representation.
    //!
    //! For internal use.
    explicit ConstMap(clingo_const_map_t const *map) : map_{map} {}

    //! Check if the map contains the given key.
    //!
    //! @param name the key to check
    //! @return whether the key is contained in the map
    [[nodiscard]] auto contains(key_type name) const -> bool {
        return Detail::call<clingo_const_map_find>(map_, name.data(), name.size(), nullptr);
    }

    //! Get the value of for the given key.
    //!
    //! @param name the key to look up
    //! @return the value for the key
    [[nodiscard]] auto operator[](key_type name) const -> mapped_type {
        clingo_symbol_t sym = 0;
        bool found = false;
        Detail::handle_error(clingo_const_map_find(map_, name.data(), name.size(), &sym, &found));
        return found ? Symbol{sym, true} : throw std::out_of_range{"key not found"};
    }

    //! Get the key value pair at the given index.
    //!
    //! @param index the index of the element
    //! @return the key value pair at the index
    [[nodiscard]] auto at(size_t index) const -> value_type {
        clingo_string_t name;
        clingo_symbol_t sym = 0;
        Detail::handle_error(clingo_const_map_at(map_, index, &name, &sym));
        return {{name.data, name.size}, Symbol{sym, true}};
    }

    //! Get the size of the map.
    //!
    //! @return the size of the map
    [[nodiscard]] auto size() const -> size_type { return Detail::call<clingo_const_map_size>(map_); }

    //! Get an iterator to the beginning of the map.
    //!
    //! @return an iterator to the beginning of the map
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }

    //! Get an iterator to the end of the map.
    //!
    //! @return an iterator to the end of the map
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    clingo_const_map_t const *map_;
};

//! Enumeration of the control modes.
//!
//! This controls how the main function of the control object proceeds.
enum class ControlMode : clingo_mode_t {
    parse = clingo_mode_parse,     //!< Parse only.
    rewrite = clingo_mode_rewrite, //!< Parse and rewrite.
    ground = clingo_mode_ground,   //!< Parse, rewrite, ground.
    solve = clingo_mode_solve,     //!< Parse, rewrite, ground, and solve.
};

//! Enumeration of the flags for writing ASPIF files.
enum class WriteAspifFlags : clingo_write_aspif_mode_t {
    none = 0,                                              //!< No flags.
    preamble = clingo_write_aspif_mode_preamble,           //!< Write preamble.
    preamble_auto = clingo_write_aspif_mode_preamble_auto, //!< Write preamble for newly created files.
    append = clingo_write_aspif_mode_append,               //!< Append to an existing file (or create it).
    preprocess = clingo_write_aspif_mode_preprocess,       //!< Whether to preprocess the program before writing.
    symbols = clingo_write_aspif_mode_symbols,             //!< Whether to write symbols in a structured format.
};
//! Enable bitset operations for the WriteAspifFlags enumeration.
CLINGO_ENABLE_BITSET_ENUM(WriteAspifFlags);

//! Enumeration of the flags for solving a logic program.
enum class SolveFlags : clingo_solve_mode_bitset_t {
    empty = 0,                       //!< Standard event-based solving.
    yield = clingo_solve_mode_yield, //!< Yield models as they are found.
    async = clingo_solve_mode_async, //!< Asynchronously solve in the background.
};
//! Enable bitset operations for the SolveFlags enumeration.
CLINGO_ENABLE_BITSET_ENUM(SolveFlags);

//! Enumeration of the types of statements that can be discarded.
enum class DiscardType {
    minimize = clingo_discard_type_e::minimize, //!< Discard minimize statements.
    project = clingo_discard_type_e::project,   //!< Discard project statements.
};

//! The main control class for grounding and solving logic programs.
//!
//! Control objects are reference counted. For example, care must be taken not
//! to store objects by value in registered propagators to avoid reference
//! cycles.
class Control {
  public:
    //! Callbock for injecting symbols into the grounding process.
    //!
    //! The callback takes the name of the function and the parameters as a
    //! string view and a span of symbols as arguments and returns a vector of
    //! symbols to inject.
    using Context = std::function<SymbolVector(std::string_view, SymbolSpan)>;

    //! Constructs a control object with the given library and arguments.
    //!
    //! @param lib the library to store symbols
    //! @param arguments the command-line arguments to pass to the control object
    explicit Control(Library const &lib, StringList arguments) : Clingo::Control{lib, StringSpan{arguments}} {}

    //! @copydoc Control(Library const &, StringList)
    explicit Control(Library const &lib, StringSpan arguments = {}) {
        auto cstrs = Detail::transform(arguments, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
        ctl_.reset(Detail::call<clingo_control_new>(c_cast(lib), cstrs.data(), arguments.size()), false);
    }

    //! Constructs a control object from an existing C representation.
    //!
    //! For internal use.
    //!
    //! @param rep the C representation of the control object
    //! @param acquire whether to acquire the control object
    explicit Control(clingo_control_t *rep, bool acquire) : ctl_{rep, acquire} {}

    //! Cast to the C representation of the control object.
    //!
    //! @param ctl the control object to cast
    //! @return the C representation of the control object
    [[nodiscard]] friend auto c_cast(Control const &ctl) -> clingo_control_t * { return ctl.ctl_.get(); }

    //! Get the control mode.
    //!
    //! @return the control mode
    [[nodiscard]] auto mode() const -> ControlMode {
        return static_cast<ControlMode>(Detail::call<clingo_control_mode>(ctl_.get()));
    }

    //! Write the current program to an ASPIF file.
    //!
    //! Note that the control object discards previously grounded programs
    //! after calls to solve(). Only the part of the program grounded after the
    //! last call to solve() or at the beginning are written to the file.
    //!
    //! @param path the path to the ASPIF file
    //! @param flags the flags to use when writing the ASPIF file
    void write_aspif(std::string_view path, WriteAspifFlags flags = WriteAspifFlags::none) const {
        Detail::handle_error(clingo_control_write_aspif(ctl_.get(), path.data(), path.size(),
                                                        static_cast<clingo_write_aspif_mode_t>(flags)));
    }

    //! Parse files with the given paths.
    //!
    //! It is also possible to read files in aspif format. However, aspif files
    //! must be read before grounding to avoid redefinition errors. Multiple
    //! aspif files can be given, for example, in the order they have been
    //! output by write_aspif.
    //!
    //! @param files the paths to the files to parse
    void parse_files(StringSpan files) const {
        auto cfiles = Detail::transform(files, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
        Detail::handle_error(clingo_control_parse_files(ctl_.get(), cfiles.data(), cfiles.size()));
    }

    //! Parse files with the given paths.
    //!
    //! It is also possible to read files in aspif format. However, aspif files
    //! must be read before grounding to avoid redefinition errors. Multiple
    //! aspif files can be given, for example, in the order they have been
    //! output by write_aspif.
    //!
    //! @param files the paths to the files to parse
    void parse_files(StringList files) const { parse_files(StringSpan{files.begin(), files.end()}); }

    //! Parse a logic program from a string.
    //!
    //! @param program the logic program to parse
    void parse_string(std::string_view program) const {
        Detail::handle_error(clingo_control_parse_string(ctl_.get(), program.data(), program.size()));
    }

    //! Ground the logic program with the given parameters.
    //!
    //! The given parts determine which program parts are grounded with which
    //! paramaters. If no parts are given, the parts given by the parts
    //! directive are grounded.
    //!
    //! @param parts the parts to ground the control object with
    //! @param ctx the context to use for grounding
    void ground(std::optional<PartSpan> parts = std::nullopt, Context ctx = nullptr) const {
        std::optional<PartVector> default_parts;
        std::vector<clingo_part_t> c_parts;
        if (default_parts = !parts ? this->parts() : std::nullopt; default_parts) {
            parts = default_parts;
        }
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

        constexpr static clingo_ground_event_handler_t handler = clingo_ground_event_handler_t{
            []([[maybe_unused]] char const *name, [[maybe_unused]] size_t name_size,
               [[maybe_unused]] size_t arguments_size, [[maybe_unused]] void *data, bool *result) -> bool {
                CLINGO_TRY {
                    *result = true;
                }
                CLINGO_CATCH;
            },
            []([[maybe_unused]] clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *location, char const *name,
               size_t name_size, clingo_symbol_t const *arguments, size_t arguments_size, void *data,
               clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) -> bool {
                CLINGO_TRY {
                    auto &cb = *static_cast<std::function<SymbolVector(std::string_view, SymbolSpan)> *>(data);
                    auto syms = cb({name, name_size}, {cpp_cast(arguments), arguments_size});
                    auto const *c_syms = c_cast(syms.data());
                    return symbol_callback(c_syms, syms.size(), symbol_callback_data);
                }
                CLINGO_CATCH;
            },
            nullptr,
            nullptr,
        };

        Detail::handle_error(
            clingo_control_ground(ctl_.get(), c_parts.data(), c_parts.size(), ctx ? &handler : nullptr, &ctx));
    }

    //! Ground the control object with the given parts.
    //!
    //! The given parts determine which program parts are grounded with which paramaters. If no parts are given, the
    //! parts given by the parts directive are grounded.
    //!
    //! @param parts the parts to ground the control object with
    //! @param ctx the context to use for grounding
    void ground(std::initializer_list<Part> parts, Context ctx = nullptr) const {
        ground(PartSpan{parts}, std::move(ctx));
    }

    //! Ground the logic program with the given parameters.
    //!
    //! The given parts determine which program parts are grounded with which
    //! paramaters. If no parts are given, the parts given by the parts
    //! directive are grounded.
    //!
    //! A handler implementing the GroundEventHandler interface can be given to
    //! handle grounding events. The handler can be passed by
    //! value/reference/pointer. If passed by value, the handler must be
    //! copyable. If passed by reference/pointer, lifetime must be managed by
    //! the caller.
    //!
    //! @param parts the parts to ground the control object with
    //! @param handler the context to use for grounding
    template <Detail::UserData<GroundEventHandler> Handler = std::nullptr_t>
    [[nodiscard]] auto start_ground(std::optional<PartSpan> parts = std::nullopt, Handler &&handler = nullptr) const
        -> GroundHandle {
        std::optional<PartVector> default_parts;
        std::vector<clingo_part_t> c_parts;
        if (default_parts = !parts ? this->parts() : std::nullopt; default_parts) {
            parts = default_parts;
        }
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
        auto user_data = Detail::make_user_data_manager(std::forward<Handler>(handler));
        using UserData = decltype(user_data);
        clingo_ground_event_handler_t const *c_handler_ptr = nullptr;
        if constexpr (!std::is_null_pointer_v<typename UserData::ValueType>) {
            static constexpr auto c_handler = clingo_ground_event_handler_t{
                [](char const *name, size_t name_size, size_t arguments_size, void *data, bool *result) -> bool {
                    CLINGO_TRY {
                        *result = UserData::cast(data)->callable(std::string_view{name, name_size}, arguments_size);
                    }
                    CLINGO_CATCH;
                },
                []([[maybe_unused]] clingo_lib_t *lib, clingo_location_t const *location, char const *name,
                   size_t name_size, clingo_symbol_t const *arguments, size_t arguments_size, void *data,
                   clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) -> bool {
                    CLINGO_TRY {
                        Location loc{location};
                        auto syms =
                            UserData::cast(data)->call(loc, {name, name_size}, {cpp_cast(arguments), arguments_size});
                        auto const *c_syms = c_cast(syms.data());
                        return symbol_callback(c_syms, syms.size(), symbol_callback_data);
                    }
                    CLINGO_CATCH;
                },
                [](clingo_ground_result_t result, void *data) {
                    UserData::cast(data)->finish(static_cast<GroundResult>(result));
                },
                [](void *data) { UserData::free(data); },
            };
            c_handler_ptr = &c_handler;
        }
        clingo_ground_handle_t *handle = nullptr;
        Detail::handle_error(clingo_control_start_ground(ctl_.get(), c_parts.data(), c_parts.size(),
                                                         user_data ? c_handler_ptr : nullptr, user_data.release(),
                                                         &handle));
        return GroundHandle{handle};
    }

    //! @copydoc start_ground
    template <Detail::UserData<GroundEventHandler> Handler = std::nullptr_t>
    [[nodiscard]] auto start_ground(std::initializer_list<Part> parts, Handler &&handler = nullptr) const
        -> GroundHandle {
        return start_ground(PartSpan{parts}, std::forward<Handler>(handler));
    }

    //! Get the base of the program.
    //!
    //! @return the base of the program
    [[nodiscard]] auto base() const -> Base { return Base{Detail::call<clingo_control_base>(ctl_.get())}; }

    //! Get the statistics of the control object.
    //!
    //! @return the statistics of the control object
    [[nodiscard]] auto stats() const -> ConstStats {
        auto const *stats = Detail::call<clingo_control_stats>(ctl_.get());
        auto key = Detail::call<clingo_stats_root>(stats);
        return ConstStats{stats, key};
    }

    //! Obtain the profiling data of the control object.
    //!
    //! To obtain profiling data, the control object must be created with the
    //! `--profile` option.
    //!
    //! @return a vector of profile nodes representing the profiling data
    auto profile() -> std::vector<ProfileNode> {
        struct Builder {
            static auto internal(size_t depth, char const *key, size_t key_size, bool nested, void *data) -> bool {
                CLINGO_TRY {
                    auto *self = static_cast<Builder *>(data);
                    assert(depth <= self->stack.size());
                    self->stack.resize(depth);
                    auto node = ProfileNodeInternal{std::string{key, key_size}, nested};
                    if (self->stack.empty()) {
                        self->roots.emplace_back(std::move(node));
                        self->stack.emplace_back(&self->roots.back());
                    } else {
                        auto &children = std::get<ProfileNodeInternal>(*self->stack.back()).children;
                        children.emplace_back(std::move(node));
                        self->stack.emplace_back(&children.back());
                    }
                }
                CLINGO_CATCH;
            }
            static auto leaf(size_t depth, clingo_profile_data_t *values, clingo_profile_type_t type, void *data)
                -> bool {
                CLINGO_TRY {
                    auto *self = static_cast<Builder *>(data);
                    assert(depth <= self->stack.size());
                    self->stack.resize(depth);
                    auto node = ProfileNodeLeaf{static_cast<ProfileType>(type), values->matches, values->instances,
                                                values->time_instantiate, values->time_propagate};
                    if (self->stack.empty()) {
                        self->roots.emplace_back(node);
                    } else {
                        std::get<ProfileNodeInternal>(*self->stack.back()).children.emplace_back(node);
                    }
                }
                CLINGO_CATCH;
            }

            std::vector<ProfileNode *> stack;
            std::vector<ProfileNode> roots;
        } builder;

        auto visitor = clingo_profile_visitor_t{&Builder::internal, &Builder::leaf};
        clingo_control_profile(ctl_.get(), &visitor, &builder);
        return std::move(builder.roots);
    }

    //! Solve the grounded program with the given assumptions and flags.
    //!
    //! @param handler the solve event handler to report events to
    //! @param assumptions the assumptions to use for solving
    //! @param flags the flags to use for solving
    //! @return a handle to the solve operation
    [[nodiscard]] auto solve(SolveEventHandler &handler, ProgramLiteralSpan const &assumptions = {},
                             SolveFlags flags = SolveFlags::empty) const -> SolveHandle {
        return solve_(&handler, assumptions, flags);
    }

    //! Solve the grounded program with the given assumptions and flags.
    //!
    //! This function does not take a solve event handler. Instead, the
    //! returned solve handle is configured to yield models as they are found.
    //!
    //! @param assumptions the assumptions to use for solving
    //! @param flags the flags to use for solving
    //! @return a handle to the solve operation
    [[nodiscard]] auto solve(ProgramLiteralSpan const &assumptions = {}, SolveFlags flags = SolveFlags::yield) const
        -> SolveHandle {
        return solve_(nullptr, assumptions, flags);
    }

    //! Run the default ground and solve flow.
    //!
    //! The flow can be configured by specifying a control mode and the parts
    //! to ground.
    void main() const { Detail::handle_error(clingo_control_main(ctl_.get())); }

    //! Interrupt the current solve operation.
    //!
    //! This function is thread-safe and can be called from any thread.
    void interrupt() const { clingo_control_interrupt(ctl_.get()); }

    //! Discard the statements of the given types.
    //!
    //! @param type the types of statements to discard
    void discard(DiscardType type) const {
        Detail::handle_error(clingo_control_discard(ctl_.get(), static_cast<clingo_discard_type_t>(type)));
    }

    //! Get the text buffer of the control object.
    //!
    //! Manually created control objects use this buffer to output statements
    //! to if the control mode has been set to parse, rewrite, or ground.
    //!
    //! @return the contents of the text buffer
    [[nodiscard]] auto buffer() const -> std::string_view {
        auto [data, size] = Detail::call<clingo_control_buffer>(ctl_.get());
        return {data, size};
    }

    //! Get the parts to ground.
    //!
    //! If no parts are set, the next call to ground would ground the base
    //! part.
    //!
    //! @return the parts to ground, or an empty optional if no parts are set
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

    //! Set the parts to ground.
    //!
    //! @param parts the parts to set
    void parts(PartList parts) const { this->parts(std::span{parts.begin(), parts.end()}); }

    //! Set the parts to ground.
    //!
    //! Use std::nullopt to ground the base part.
    //!
    //! @param parts the parts to set
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

    //! Get the configuration of the control object.
    //!
    //! @return the configuration of the control object
    [[nodiscard]] auto config() const -> Config {
        auto *config = Detail::call<clingo_control_config>(ctl_.get());
        auto key = Detail::call<clingo_config_root>(config);
        return Config{config, key};
    }

    //! Get the constant map of the control object.
    //!
    //! @return the constant map of the control object
    [[nodiscard]] auto const_map() const -> ConstMap {
        return ConstMap{Detail::call<clingo_control_const_map>(ctl_.get())};
    }

    //! Inspect the current ground program held by the control object.
    //!
    //! @param obs the observer to use for inspecting the program
    //! @param preprocess whether to preprocess the program before observing
    void observe(Observer &obs, bool preprocess = true) const { obs.observe(ctl_.get(), preprocess); }

    //! Get the backend of the control object.
    //!
    //! @return the backend of the control object
    [[nodiscard]] auto backend() const -> ProgramBackend {
        return ProgramBackend{Detail::call<clingo_control_backend>(ctl_.get())};
    }

    //! Register a propagator with the control object.
    //!
    //! Can be used to register both propagators with and without heuristics.
    //!
    //! @param propagator the propagator to register
    //! @return a reference to the registered propagator
    template <std::derived_from<Propagator> T> auto register_propagator(std::unique_ptr<T> propagator) const -> T & {
        assert(propagator != nullptr);
        auto &res = *propagator;
        Detail::handle_error(clingo_control_register_propagator(
            ctl_.get(), std::is_base_of_v<Heuristic, T> ? &Detail::c_heuristic : &Detail::c_propagator,
            propagator.release()));
        return res;
    }

    //! Join the given non-ground program to the current control object.
    //!
    //! @param prg the program to join
    void join(AST::Program const &prg) const { Detail::join(ctl_.get(), c_cast(prg)); }

  private:
    friend class Detail::intrusive_handle<Control, clingo_control_t>;

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

//! @}

} // namespace Clingo
