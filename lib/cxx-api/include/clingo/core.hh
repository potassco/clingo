#pragma once

#include <clingo/core.h>
#include <clingo/shared.h>

#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Clingo {

namespace Detail {

template <typename> inline constexpr bool always_false = false;

#ifndef __clang_analyzer__
#define CLINGO_ENABLE_BITSET_ENUM(E, ...)                                                                              \
    [[nodiscard]] CLINGO_ENUM_OP(~, (E a), __VA_ARGS__)->E {                                                           \
        return static_cast<E>(~static_cast<std::underlying_type_t<E>>(a));                                             \
    }                                                                                                                  \
    [[nodiscard]] CLINGO_ENUM_OP(|, (E a, E b), __VA_ARGS__)->E {                                                      \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) | static_cast<std::underlying_type_t<E>>(b));  \
    }                                                                                                                  \
    CLINGO_ENUM_OP(|=, (E & a, E b), __VA_ARGS__)->E & {                                                               \
        return a = a | b;                                                                                              \
    }                                                                                                                  \
    [[nodiscard]] CLINGO_ENUM_OP(&, (E a, E b), __VA_ARGS__)->E {                                                      \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) & static_cast<std::underlying_type_t<E>>(b));  \
    }                                                                                                                  \
    CLINGO_ENUM_OP(&=, (E & a, E b), __VA_ARGS__)->E & {                                                               \
        return a = a & b;                                                                                              \
    }                                                                                                                  \
    [[nodiscard]] CLINGO_ENUM_OP(-, (E a, E b), __VA_ARGS__)->E {                                                      \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) & static_cast<std::underlying_type_t<E>>(~b)); \
    }                                                                                                                  \
    CLINGO_ENUM_OP(-=, (E & a, E b), __VA_ARGS__)->E & {                                                               \
        return a = a - b;                                                                                              \
    }                                                                                                                  \
    [[nodiscard]] CLINGO_ENUM_OP(^, (E a, E b), __VA_ARGS__)->E {                                                      \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) ^ static_cast<std::underlying_type_t<E>>(b));  \
    }                                                                                                                  \
    CLINGO_ENUM_OP(^=, (E & a, E b), __VA_ARGS__)->E & {                                                               \
        return a = a ^ b;                                                                                              \
    }                                                                                                                  \
    [[nodiscard]] [[maybe_unused]] inline __VA_ARGS__ constexpr auto intersects(E a, E b) -> bool {                    \
        return static_cast<std::underlying_type_t<E>>(a & b) != 0;                                                     \
    }                                                                                                                  \
    static_assert(std::is_enum_v<E>)
#define CLINGO_ENUM_OP(op, arg, ...) [[maybe_unused]] inline __VA_ARGS__ constexpr auto operator op arg noexcept
#else
#define CLINGO_ENABLE_BITSET_ENUM(E, ...)                                                                              \
    [[nodiscard]] CLINGO_ENUM_OP(~, (E a), __VA_ARGS__)->E;                                                            \
    [[nodiscard]] CLINGO_ENUM_OP(|, (E a, E b), __VA_ARGS__)->E;                                                       \
    CLINGO_ENUM_OP(|=, (E & a, E b), __VA_ARGS__)->E &;                                                                \
    [[nodiscard]] CLINGO_ENUM_OP(&, (E a, E b), __VA_ARGS__)->E;                                                       \
    CLINGO_ENUM_OP(&=, (E & a, E b), __VA_ARGS__)->E &;                                                                \
    [[nodiscard]] CLINGO_ENUM_OP(-, (E a, E b), __VA_ARGS__)->E;                                                       \
    CLINGO_ENUM_OP(-=, (E & a, E b), __VA_ARGS__)->E &;                                                                \
    [[nodiscard]] CLINGO_ENUM_OP(^, (E a, E b), __VA_ARGS__)->E;                                                       \
    CLINGO_ENUM_OP(^=, (E & a, E b), __VA_ARGS__)->E &;                                                                \
    [[nodiscard]] [[maybe_unused]] __VA_ARGS__ auto intersects(E a, E b) -> bool;                                      \
    static_assert(std::is_enum_v<E>)
#define CLINGO_ENUM_OP(op, arg, ...) [[maybe_unused]] __VA_ARGS__ auto operator op arg noexcept
#endif

#define CLINGO_TRY try
#define CLINGO_CATCH                                                                                                   \
    catch (...) {                                                                                                      \
        return Clingo::Detail::store_error();                                                                          \
    }                                                                                                                  \
    return true

//! Store the active exception as a clingo error.
inline auto store_error() -> bool {
    try {
        throw;
    } catch (std::out_of_range const &e) {
        auto msg = std::string_view{e.what()};
        return clingo_set_error(clingo_result_range, msg.data(), msg.size());
    } catch (std::invalid_argument const &e) {
        auto msg = std::string_view{e.what()};
        return clingo_set_error(clingo_result_invalid, msg.data(), msg.size());
    } catch (std::bad_alloc const &e) {
        auto msg = std::string_view{e.what()};
        return clingo_set_error(clingo_result_bad_alloc, msg.data(), msg.size());
    } catch (std::logic_error const &e) {
        auto msg = std::string_view{e.what()};
        return clingo_set_error(clingo_result_logic, msg.data(), msg.size());
    } catch (std::exception const &e) {
        auto msg = std::string_view{e.what()};
        return clingo_set_error(clingo_result_runtime, msg.data(), msg.size());
    } catch (...) {
        auto msg = std::string_view{"no message"};
        return clingo_set_error(clingo_result_runtime, msg.data(), msg.size());
    }
}

//! Raise the current clingo error as an exception.
inline void raise_error() {
    clingo_string_t str;
    clingo_result_t rc = clingo_result_success;
    clingo_get_error(&rc, &str);
    switch (rc) {
        case clingo_result_bad_alloc: {
            throw std::bad_alloc{};
        }
        case clingo_result_logic: {
            throw std::logic_error{std::string{str.data, str.size}};
        }
        case clingo_result_invalid: {
            throw std::invalid_argument{std::string{str.data, str.size}};
        }
        case clingo_result_range: {
            throw std::out_of_range{std::string{str.data, str.size}};
        }
        default: {
            throw std::runtime_error{std::string{str.data, str.size}};
        }
    }
}

//! This function should be used to handle failing C API calls.
//!
//! It rethrows the current error as a python error.
inline void handle_error(bool res) {
    if (!res) {
        raise_error();
    }
}

// Similar to handle_error but also reraises if the code is success.
inline void handle_error_no_code(bool res, int code) {
    if (!res) {
        raise_error();
    }
    if ((code & 65) == 65 || (code & 33) == 33) { // NOLINT
        clingo_string_t str;
        clingo_result_t rc = clingo_result_success;
        clingo_get_error(&rc, &str);
        if (rc != clingo_result_success) {
            raise_error();
        }
    }
}

template <typename> struct funptr_traits;

template <typename R, typename... As> struct funptr_traits<R (*)(As...)> {
    using return_type = R;
    using args_tuple = std::tuple<As...>;
    template <std::size_t N> using arg = std::tuple_element_t<N, args_tuple>;

    static constexpr std::size_t arity = sizeof...(As);
    using last = arg<arity - 1>;
};

template <auto F, class... As> auto call(As &&...args) {
    using Traits = funptr_traits<decltype(F)>;
    auto res = std::remove_pointer_t<typename Traits::last>{};
    if constexpr (std::is_same_v<typename Traits::return_type, void>) {
        F(std::forward<As>(args)..., &res);
    } else {
        handle_error(F(std::forward<As>(args)..., &res));
    }
    return res;
}

template <auto F> struct Free {
    template <typename Ptr> void operator()(Ptr p) const noexcept { F(p); }
};

template <class T> struct value_handle {
  public:
    using pointer = typename T::pointer;

    value_handle() = default;

    value_handle(const value_handle &other) : ptr_{other ? T::copy(other.ptr_) : nullptr} {}

    value_handle(value_handle &&other) noexcept : ptr_{std::exchange(other.ptr_, nullptr)} {}

    explicit value_handle(pointer ptr, bool copy) : ptr_{copy && ptr != nullptr ? T::copy(ptr) : ptr} {}

    ~value_handle() {
        if (*this) {
            T::free(ptr_);
        }
    }

    auto operator=(const value_handle &other) -> value_handle & {
        if (this != &other) {
            if (*this) {
                T::free(ptr_);
            }
            ptr_ = other ? T::copy(other.ptr_) : nullptr;
        }
        return *this;
    }

    auto operator=(value_handle &&other) noexcept -> value_handle & {
        if (this != &other) {
            if (*this) {
                T::free(ptr_);
            }
            ptr_ = std::exchange(other.ptr_, nullptr);
        }
        return *this;
    }

    [[nodiscard]] auto get() const -> pointer { return ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }

  private:
    pointer ptr_ = nullptr;
};

template <auto Copy, auto Free> struct value_handle_traits {
    using pointer = std::remove_pointer_t<typename funptr_traits<decltype(Copy)>::template arg<1>>;
    using const_pointer = typename funptr_traits<decltype(Copy)>::template arg<0>;
    static auto copy(pointer p) -> pointer {
        auto res = pointer{};
        handle_error(Copy(p, &res));
        return res;
    }
    static void free(const_pointer p) noexcept { Free(p); }
};

template <class T, auto F> using unique_handle = std::unique_ptr<T, Free<F>>;

//! Use std::transform to build a vector.
template <std::input_iterator It, std::sentinel_for<It> S, class Pred> auto transform(It begin, S end, Pred pred) {
    auto p = std::vector<std::invoke_result_t<Pred, std::iter_reference_t<It>>>{};
    p.reserve(std::distance(begin, end));
    std::transform(begin, end, std::back_inserter(p), pred);
    return p;
}

//! Use std::transform to build a vector.
template <std::ranges::input_range Rng, class Pred> auto transform(Rng &&rng, Pred pred) { // NOLINT
    return transform(std::ranges::begin(rng), std::ranges::end(rng), pred);
}

//! Compute the hash for the given type.
//!
//! This is a convenience wrapper around the std::hash struct.
template <class T> auto hash_value(T const &x) {
    return std::hash<T>{}(x);
}

//! Combine the given hash values.
inline auto hash_combine(size_t a, size_t b) -> size_t {
    // NOLINTBEGIN
    auto p = std::make_pair(a, b);
    return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<char const *>(&p), sizeof(decltype(p))));
    // NOLINTEND
}

template <class T, class P> class intrusive_handle {
  public:
    intrusive_handle() = default;
    explicit intrusive_handle(P *ptr, bool inc = true) : ptr_{ptr} {
        if (inc) {
            T::acquire(ptr);
        }
    }
    intrusive_handle(intrusive_handle const &other) : intrusive_handle{other.ptr_} {}
    intrusive_handle(intrusive_handle &&other) noexcept : ptr_{std::exchange(other.ptr_, nullptr)} {}
    ~intrusive_handle() noexcept { T::release(ptr_); }
    auto operator=(intrusive_handle const &other) -> intrusive_handle & {
        T::acquire(other.ptr_);
        T::release(ptr_);
        ptr_ = other.ptr_;
        return *this;
    }
    auto operator=(intrusive_handle &&other) noexcept -> intrusive_handle & {
        T::release(ptr_);
        ptr_ = std::exchange(other.ptr_, nullptr);
        return *this;
    }
    [[nodiscard]] auto get() const noexcept -> P * { return ptr_; }
    void reset(std::nullptr_t = nullptr) noexcept {
        T::release(ptr_);
        ptr_ = nullptr;
    }
    void reset(P *ptr, bool inc = true) {
        T::release(ptr_);
        ptr_ = ptr;
        if (inc) {
            T::acquire(ptr_);
        }
    }
    friend auto operator==(intrusive_handle const &p, std::nullptr_t) noexcept -> bool { return p.ptr_ == nullptr; }
    friend auto operator!=(intrusive_handle const &p, std::nullptr_t) noexcept -> bool { return p.ptr_ != nullptr; }
    friend auto operator==(std::nullptr_t, intrusive_handle const &p) noexcept -> bool { return p.ptr_ == nullptr; }
    friend auto operator!=(std::nullptr_t, intrusive_handle const &p) noexcept -> bool { return p.ptr_ != nullptr; }

  private:
    P *ptr_ = nullptr;
};

template <typename T> class ArrowProxy {
  public:
    constexpr ArrowProxy(T value) : value_(std::move(value)) {}
    constexpr auto operator->() -> T * { return &value_; }

  private:
    T value_;
};

template <class Seq> class RandomAccessIterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename Seq::value_type;
    using size_type = typename Seq::size_type;
    using difference_type = typename Seq::difference_type;
    using pointer = typename Seq::pointer;
    using reference = typename Seq::reference;

    // NOTE: Added to fulfill the sentinel_for concept; should not be used.
    constexpr RandomAccessIterator() : view_{throw std::logic_error("invalid iterator")}, index_{0} {}
    constexpr RandomAccessIterator(Seq container, size_t index) noexcept : view_{std::move(container)}, index_{index} {}
    constexpr auto operator*() const -> reference { return view_.at(index_); }
    constexpr auto operator->() const -> pointer { return view_.at(index_); }
    constexpr auto operator++() -> RandomAccessIterator & {
        ++index_;
        return *this;
    }
    constexpr auto operator++(int) -> RandomAccessIterator {
        auto tmp = *this;
        ++index_;
        return tmp;
    }
    constexpr auto operator--() -> RandomAccessIterator & {
        --index_;
        return *this;
    }
    constexpr auto operator--(int) -> RandomAccessIterator {
        auto tmp = *this;
        --index_;
        return tmp;
    }
    constexpr auto operator-(const RandomAccessIterator &other) const -> difference_type {
        return static_cast<difference_type>(index_) - static_cast<difference_type>(other.index_);
    }
    constexpr auto operator+(difference_type n) const -> RandomAccessIterator {
        return RandomAccessIterator(view_, index_ + n);
    }
    friend constexpr auto operator+(difference_type n, RandomAccessIterator it) -> RandomAccessIterator {
        return RandomAccessIterator(it.view_, it.index_ + n);
    }
    constexpr auto operator-(difference_type n) const -> RandomAccessIterator {
        return RandomAccessIterator(view_, index_ - n);
    }
    constexpr auto operator+=(difference_type n) -> RandomAccessIterator & {
        index_ += n;
        return *this;
    }
    constexpr auto operator-=(difference_type n) -> RandomAccessIterator & {
        index_ -= n;
        return *this;
    }
    constexpr auto operator==(const RandomAccessIterator &other) const -> bool { return index_ == other.index_; }
    constexpr auto operator<=>(const RandomAccessIterator &other) const { return index_ <=> other.index_; }
    constexpr auto operator[](difference_type n) const -> reference { return view_.at(index_ + n); }

  private:
    Seq view_;
    size_t index_;
};

} // namespace Detail

//! @addtogroup cpp_core
//! Core types and functions used throughout all modules and version information.
//! @{

//! A program id used for various kinds of indices.
using ProgramId = clingo_id_t;
//! A span of program ids.
using ProgramIdSpan = std::span<ProgramId const>;

//! A program atom.
using ProgramAtom = clingo_atom_t;
//! A span of program atoms.
using ProgramAtomSpan = std::span<ProgramAtom const>;

//! A program literal.
using ProgramLiteral = clingo_literal_t;
//! A span of program literals.
using ProgramLiteralSpan = std::span<ProgramLiteral const>;
//! A vector of program literals.
using ProgramLiteralVector = std::vector<ProgramLiteral>;

//! A solver literal.
using SolverLiteral = clingo_literal_t;
//! A span of solver literals.
using SolverLiteralSpan = std::span<SolverLiteral const>;
//! A list of solver literals.
using SolverLiteralList = std::initializer_list<SolverLiteral const>;
//! A vector of solver literals.
using SolverLiteralVector = std::vector<SolverLiteral>;

//! A weight used in sum aggregates and minimize constraints.
using Weight = clingo_weight_t;
//! A span of weights.
using WeightSpan = std::span<clingo_weight_t const>;

//! A weighted literal, which is a literal with an associated weight.
using WeightedLiteral = clingo_weighted_literal_t;
//! A span of weighted literals.
using WeightedLiteralSpan = std::span<WeightedLiteral const>;

//! A sum representing the sum of weights.
using Sum = int64_t;
//! A span of sums.
using SumSpan = std::span<Sum const>;

//! A span of string views.
using StringSpan = std::span<std::string_view const>;
//! A list of string views
using StringList = std::initializer_list<std::string_view const>;

//! Enumeration of message codes.
enum class MessageCode : clingo_message_t {
    trace = clingo_message_trace,                             //!< a trace message
    debug = clingo_message_debug,                             //!< a debug message
    info = clingo_message_info,                               //!< an info message
    operation_undefined = clingo_message_operation_undefined, //!< undefined operation in program
    atom_undefined = clingo_message_atom_undefined,           //!< undefined atom in program
    file_included = clingo_message_file_included,             //!< same file included multiple times
    global_variable = clingo_message_global_variable,         //!< global variable in tuple of aggregate element
    warn = clingo_message_warn,                               //!< a warning message
    error = clingo_message_error, //!< to report multiple errors; a corresponding runtime error is raised later
};

//! Enumeration of log levels.
enum class LogLevel {
    trace = clingo_log_level_trace, //!< the trace level (most verbose)
    debug = clingo_log_level_debug, //!< the debug level
    info = clingo_log_level_info,   //!< the info level
    wart = clingo_log_level_warn,   //!< the warning level
    error = clingo_log_level_error, //!< the error level (least verbose)
};

//! Flags to create library objects.
enum class LibraryFlags : clingo_lib_flags_t {
    none = 0,                                     //!< no flags set
    slotted = clingo_lib_flags_slotted,           //!< use custom allocator for storing symbols
    shared = clingo_lib_flags_shared,             //!< create symbols in a thread-safe manner
    fast_release = clingo_lib_flags_fast_release, //!< whether to enable fast release of libraries
};
//! Enable bitset operations on LibraryFlags.
CLINGO_ENABLE_BITSET_ENUM(LibraryFlags);

//! The default message limit for the logger.
inline constexpr size_t default_message_limit = 25;

//! A callback function type for logging messages.
//!
//! The callback takes a message code and a string view as arguments.
using Logger = std::function<void(MessageCode, std::string_view)>;

//! The main library class for managing global information and logging.
//!
//! Library objects are reference counted. If the last reference goes out of
//! scope, all contained symbols are freed.
class Library {
  public:
    //! Constructs a library object.
    //!
    //! @param flags the flags to create the library with
    //! @param logger the logger to use for messages; if nullptr, the default logger is used
    //! @param level the log level for the logger
    //! @param limit the message limit for the logger; defaults to `default_message_limit`
    explicit Library(LibraryFlags flags = LibraryFlags::none, Logger logger = nullptr, LogLevel level = LogLevel::info,
                     size_t limit = default_message_limit) {
        auto log = logger ? std::make_unique<Logger>(std::move(logger)) : nullptr;
        auto has_log = static_cast<bool>(log);
        rep_.reset(Detail::call<clingo_lib_new>(static_cast<clingo_lib_flags_t>(flags),
                                                static_cast<clingo_log_level_t>(level), has_log ? &c_logger : nullptr,
                                                log.release(), limit),
                   false);
    }
    //! Constructs a library object from an existing C representation.
    //!
    //! For internal use.
    //!
    //! @param rep the C representation of the library object
    //! @param acquire whether to acquire the library object
    explicit Library(clingo_lib_t *rep, bool acquire) : rep_{rep, acquire} {}

    //! Casts the library object to its C representation.
    //!
    //! @param lib the library object to cast
    //! @return the C representation of the library object
    [[nodiscard]] friend auto c_cast(Library const &lib) -> clingo_lib_t * { return lib.rep_.get(); }

  private:
    friend class Detail::intrusive_handle<Library, clingo_lib_t>;

    static constexpr clingo_logger_t c_logger = {
        [](clingo_message_t code, char const *message, size_t size, void *data) {
            (*static_cast<Logger *>(data))(static_cast<MessageCode>(code), {message, size});
        },
        [](void *data) noexcept { std::unique_ptr<Logger>(static_cast<Logger *>(data)); },
    };

    static auto acquire(clingo_lib_t *ptr) { clingo_lib_acquire(ptr); }
    static auto release(clingo_lib_t *ptr) { clingo_lib_release(ptr); }

    Detail::intrusive_handle<Library, clingo_lib_t> rep_;
};

//! A string builder for constructing strings.
//!
//! Some API functions accept a string builder to construct strings in a
//! memory-efficient way.
//!
//! String builders have value semantics.
class StringBuilder {
  public:
    //! Construct an empty string builder.
    explicit StringBuilder() : bld_{Detail::call<clingo_string_builder_new>(), false} {}

    //! Cast the string builder to its C representation.
    //!
    //! @param bld the string builder to cast
    //! @return the C representation of the string builder
    [[nodiscard]] friend auto c_cast(StringBuilder &bld) -> clingo_string_builder_t * { return bld.bld_.get(); }

    //! Get a view of the string built by the string builder.
    //!
    //! @return the string view of the built string
    [[nodiscard]] auto str() const -> std::string_view {
        auto [data, size] = Detail::call<clingo_string_builder_string>(bld_.get());
        return {data, size};
    }

    //! Clear the string builder, removing all content.
    void clear() noexcept { clingo_string_builder_clear(bld_.get()); }

  private:
    using Traits = Detail::value_handle_traits<clingo_string_builder_copy, clingo_string_builder_free>;
    Detail::value_handle<Traits> bld_;
};

//! Enumeration of control modes.
enum class ExternalType : clingo_external_type_t {
    free = clingo_external_type_free,       //!< Allow an external to be assigned freely.
    true_ = clingo_external_type_true,      //!< Assign an external to true.
    false_ = clingo_external_type_false,    //!< Assign an external to false.
    release = clingo_external_type_release, //!< No longer treat an atom as external.
};

//! Enumeration of heuristic types.
enum class HeuristicType : clingo_heuristic_type_t {
    level = clingo_heuristic_type_level,   //!< Set the level of an atom.
    sign = clingo_heuristic_type_sign,     //!< Configure which sign to chose for an atom.
    factor = clingo_heuristic_type_factor, //!< Modify VSIDS factor of an atom.
    init = clingo_heuristic_type_init,     //!< Modify the initial VSIDS score of an atom.
    true_ = clingo_heuristic_type_true,    //!< Set the level of an atom and choose a positive sign.
    false_ = clingo_heuristic_type_false   //!< Set the level of an atom and choose a negative sign.
};

//! Class representing a position in a file.
//!
//! Positions implement value semantics supporting ordering and hashing.
class Position {
  public:
    //! Constructs a position from an existing C representation.
    //!
    //! For internal use.
    //!
    //! @param pos the C representation of the position
    explicit Position(clingo_position_t const *pos) : pos_{pos, true} {}

    //! Constructs a position from a file, line, and column.
    //!
    //! @param lib the library to store symbols
    //! @param file the file name
    //! @param line the line number (1-based)
    //! @param column the column number (1-based)
    explicit Position(Library const &lib, std::string_view file, size_t line, size_t column)
        : pos_{Detail::call<clingo_position_new>(c_cast(lib), file.data(), file.size(), line, column), false} {}

    //! Cast a position to its C representation.
    //!
    //! @param x the position to cast
    //! @return the C representation of the position
    friend auto c_cast(Position const &x) -> clingo_position_t const * { return x.pos_.get(); }

    //! Get the file name of the position.
    //!
    //! @return the file name as a string view
    [[nodiscard]] auto file() const -> std::string_view {
        auto [data, size] = Detail::call<clingo_position_file>(pos_.get());
        return {data, size};
    }

    //! Get the line number of the position.
    //!
    //! @return the line number (1-based)
    [[nodiscard]] auto line() const -> size_t { return clingo_position_line(pos_.get()); }

    //! Get the column number of the position.
    //!
    //! @return the column number (1-based)
    [[nodiscard]] auto column() const -> size_t { return clingo_position_column(pos_.get()); }

    //! Convert the position to a string representation.
    //!
    //! @return the string representation of the position
    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_position_to_string(pos_.get(), c_cast(bld)));
        return std::string{bld.str()};
    }

    //! Compute the hash of the position.
    //!
    //! There is also a corresponding specialization of std::hash.
    //!
    //! @return the hash value of the position
    [[nodiscard]] auto hash() const noexcept -> size_t { return clingo_position_hash(pos_.get()); }

    //! Compare two positions for equality.
    //!
    //! @param a the first position to compare
    //! @param b the second position to compare
    //! @return whether the two positions are equal
    friend auto operator==(Position const &a, Position const &b) noexcept -> bool {
        return clingo_position_equal(a.pos_.get(), b.pos_.get());
    }

    //! Compare two positions.
    //!
    //! @param a the first position to compare
    //! @param b the second position to compare
    //! @return the comparison result
    friend auto operator<=>(Position const &a, Position const &b) noexcept -> std::strong_ordering {
        return clingo_position_compare(a.pos_.get(), b.pos_.get()) <=> 0;
    }

  private:
    using Traits = Detail::value_handle_traits<clingo_position_copy, clingo_position_free>;
    Detail::value_handle<Traits> pos_;
};

//! Class representing a range in a file.
//!
//! Locations implement value semantics supporting ordering and hashing.
class Location {
  public:
    //! Constructs a location from an existing C representation.
    explicit Location(clingo_location_t const *loc) : loc_{loc, true} {}

    //! Constructs a location from a begin and end position.
    //!
    //! @param begin the position marking the beginning of the location
    //! @param end the position marking the end of the location
    explicit Location(Position const &begin, Position const &end)
        : loc_{Detail::call<clingo_location_new>(c_cast(begin), c_cast(end)), false} {}

    //! Cast the location to its C representation.
    //!
    //! @return the C representation of the location
    friend auto c_cast(Location const &x) -> clingo_location_t const * { return x.loc_.get(); }

    //! Get the position marking the beginning of the location.
    //!
    //! @return the position marking the beginning of the location
    [[nodiscard]] auto begin() const -> Position { return Position{clingo_location_begin(loc_.get())}; }

    //! Get the position marking the end of the location.
    //!
    //! @return the position marking the end of the location
    [[nodiscard]] auto end() const -> Position { return Position{clingo_location_end(loc_.get())}; }

    //! Convert the location to a string representation.
    //!
    //! @return the string representation of the location
    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_location_to_string(loc_.get(), c_cast(bld)));
        return std::string{bld.str()};
    }

    //! Get a hash value for the location.
    [[nodiscard]] auto hash() const noexcept -> size_t { return clingo_location_hash(loc_.get()); }

    //! Compare two locations for equality.
    //!
    //! @param a the first location to compare
    //! @param b the second location to compare
    //! @return whether the two positions are equal
    friend auto operator==(Location const &a, Location const &b) noexcept -> bool {
        return clingo_location_equal(a.loc_.get(), b.loc_.get());
    }

    //! Compare two locations.
    //!
    //! @param a the first location to compare
    //! @param b the second location to compare
    //! @return the comparison result
    friend auto operator<=>(Location const &a, Location const &b) noexcept -> std::strong_ordering {
        return clingo_location_compare(a.loc_.get(), b.loc_.get()) <=> 0;
    }

  private:
    using Traits = Detail::value_handle_traits<clingo_location_copy, clingo_location_free>;
    Detail::value_handle<Traits> loc_;
};

namespace Version {
//! The major version number of the Clingo library.
inline constexpr int major = CLINGO_VERSION_MAJOR;
//! The minor version number of the Clingo library.
inline constexpr int minor = CLINGO_VERSION_MINOR;
//! The revision number of the Clingo library.
inline constexpr int revision = CLINGO_VERSION_REVISION;
} // namespace Version

//! Get the version of the Clingo library as a tuple.
//!
//! @return a tuple containing the major, minor, and revision version numbers
inline auto version() -> std::tuple<int, int, int> {
    int major = 0;
    int minor = 0;
    int revision = 0;
    clingo_version(&major, &minor, &revision);
    return {major, minor, revision};
}

//! @}

} // namespace Clingo
