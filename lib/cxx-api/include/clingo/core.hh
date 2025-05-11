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

// Similar to handle_error but also reraises if the code is succsess.
inline void handle_error_no_code(bool res) {
    if (!res) {
        raise_error();
    }
    clingo_string_t str;
    clingo_result_t rc = clingo_result_success;
    clingo_get_error(&rc, &str);
    if (rc != clingo_result_success) {
        raise_error();
    }
}

template <auto F> struct Free {
    template <typename Ptr> void operator()(Ptr p) const noexcept { F(p); }
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
    using std::begin;
    using std::end;
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

    // NOTE: Added to fullfil the sentinel_for concept; should not be used.
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

using ProgramId = clingo_id_t;
using ProgramIdSpan = std::span<ProgramId const>;

using ProgramAtom = clingo_atom_t;
using ProgramAtomSpan = std::span<ProgramAtom const>;

using ProgramLiteral = clingo_literal_t;
using ProgramLiteralSpan = std::span<ProgramLiteral const>;
using ProgramLiteralVector = std::vector<ProgramLiteral>;

using SolverLiteral = clingo_literal_t;
using SolverLiteralSpan = std::span<SolverLiteral const>;
using SolverLiteralVector = std::vector<SolverLiteral>;

using Weight = clingo_weight_t;
using WeightSpan = std::span<clingo_weight_t const>;

using WeightedLiteral = clingo_weighted_literal_t;
using WeightedLiteralSpan = std::span<WeightedLiteral const>;

using Sum = int64_t;
using SumSpan = std::span<Sum const>;

using StringSpan = std::span<std::string_view const>;
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
CLINGO_ENABLE_BITSET_ENUM(LibraryFlags);

constexpr size_t default_message_limit = 25;

using Logger = std::function<void(MessageCode, std::string_view)>;

class Library {
  public:
    Library(LibraryFlags flags = LibraryFlags::none, Logger logger = nullptr, LogLevel level = LogLevel::info,
            size_t limit = default_message_limit) {
        clingo_lib_t *ptr = nullptr;
        auto log = std::make_unique<Logger>(logger ? std::move(logger) : nullptr);
        Detail::handle_error(clingo_lib_new(static_cast<clingo_lib_flags_t>(flags),
                                            static_cast<clingo_log_level_t>(level), log ? &c_logger : nullptr,
                                            log.release(), limit, &ptr));
        rep_.reset(ptr, false);
    }
    explicit Library(clingo_lib_t *rep, bool acquire) : rep_{rep, acquire} {}

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

class StringBuilder {
  public:
    StringBuilder() { Detail::handle_error(clingo_string_builder_new(&rep_)); }
    ~StringBuilder() { clingo_string_builder_free(rep_); }

    StringBuilder(StringBuilder const &other) { Detail::handle_error(clingo_string_builder_copy(other.rep_, &rep_)); }
    auto operator=(StringBuilder const &other) -> StringBuilder & {
        if (other.rep_ != rep_) {
            clingo_string_builder_free(rep_);
            Detail::handle_error(clingo_string_builder_copy(other.rep_, &rep_));
        }
        return *this;
    }

    StringBuilder(StringBuilder &&other) noexcept : rep_{std::exchange(other.rep_, nullptr)} {}
    auto operator=(StringBuilder &&other) noexcept -> StringBuilder & {
        if (other.rep_ != rep_) {
            clingo_string_builder_free(rep_);
            rep_ = std::exchange(other.rep_, nullptr);
        }
        return *this;
    }

    [[nodiscard]] friend auto c_cast(StringBuilder &bld) -> clingo_string_builder_t * { return bld.rep_; }

    [[nodiscard]] auto str() const -> std::string_view {
        clingo_string_t res;
        Detail::handle_error(clingo_string_builder_string(rep_, &res));
        return {res.data, res.size};
    }

    void clear() noexcept { clingo_string_builder_clear(rep_); }

  private:
    clingo_string_builder_t *rep_ = nullptr;
};

enum class ExternalType : clingo_external_type_t {
    free = clingo_external_type_free,       //!< allow an external to be assigned freely
    true_ = clingo_external_type_true,      //!< assign an external to true
    false_ = clingo_external_type_false,    //!< assign an external to false
    release = clingo_external_type_release, //!< no longer treat an atom as external
};

enum class HeuristicType : clingo_heuristic_type_t {
    level = clingo_heuristic_type_level,   //!< set the level of an atom
    sign = clingo_heuristic_type_sign,     //!< configure which sign to chose for an atom
    factor = clingo_heuristic_type_factor, //!< modify VSIDS factor of an atom
    init = clingo_heuristic_type_init,     //!< modify the initial VSIDS score of an atom
    true_ = clingo_heuristic_type_true,    //!< set the level of an atom and choose a positive sign
    false_ = clingo_heuristic_type_false   //!< set the level of an atom and choose a negative sign
};

class Position {
  public:
    Position(Position const &other) { Detail::handle_error(clingo_position_copy(other.pos_, &pos_)); }

    auto operator=(Position const &other) -> Position & {
        if (this != &other) {
            assert(pos_ == nullptr || pos_ != other.pos_);
            clingo_position_free(std::exchange(pos_, nullptr));
            Detail::handle_error(clingo_position_copy(other.pos_, &pos_));
        }
        return *this;
    }

    Position(Position &&other) noexcept : pos_{std::exchange(other.pos_, nullptr)} {}

    friend auto c_cast(Position const &x) -> clingo_position_t const * { return x.pos_; }

    auto operator=(Position &&other) noexcept -> Position & {
        if (this != &other) {
            assert(pos_ == nullptr || pos_ != other.pos_);
            clingo_position_free(std::exchange(pos_, std::exchange(other.pos_, nullptr)));
        }
        return *this;
    }

    ~Position() noexcept { clingo_position_free(pos_); }

    explicit Position(clingo_position_t const *pos) { Detail::handle_error(clingo_position_copy(pos, &pos_)); }

    Position(Library const &lib, std::string_view file, size_t line, size_t column) {
        Detail::handle_error(clingo_position_new(c_cast(lib), file.data(), file.size(), line, column, &pos_));
    }

    [[nodiscard]] auto file() const -> std::string_view {
        clingo_string_t val;
        clingo_position_file(pos_, &val);
        return {val.data, val.size};
    }

    [[nodiscard]] auto line() const -> size_t { return clingo_position_line(pos_); }

    [[nodiscard]] auto column() const -> size_t { return clingo_position_column(pos_); }

    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_position_to_string(pos_, c_cast(bld)));
        return std::string{bld.str()};
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_position_hash(pos_); }

    friend auto operator==(Position const &a, Position const &b) -> bool {
        return clingo_position_equal(a.pos_, b.pos_);
    }
    friend auto operator<=>(Position const &a, Position const &b) -> std::strong_ordering {
        return clingo_position_compare(a.pos_, b.pos_) <=> 0;
    }

  private:
    clingo_position_t const *pos_ = nullptr;
};

class Location {
  public:
    Location(Location const &other) { Detail::handle_error(clingo_location_copy(other.loc_, &loc_)); }

    auto operator=(Location const &other) -> Location & {
        if (this != &other) {
            assert(loc_ == nullptr || loc_ != other.loc_);
            clingo_location_free(std::exchange(loc_, nullptr));
            Detail::handle_error(clingo_location_copy(other.loc_, &loc_));
        }
        return *this;
    }

    Location(Location &&other) noexcept : loc_{std::exchange(other.loc_, nullptr)} {}

    auto operator=(Location &&other) noexcept -> Location & {
        if (this != &other) {
            assert(loc_ == nullptr || loc_ != other.loc_);
            clingo_location_free(std::exchange(loc_, std::exchange(other.loc_, nullptr)));
        }
        return *this;
    }

    ~Location() noexcept { clingo_location_free(loc_); }

    explicit Location(clingo_location_t const *loc) { Detail::handle_error(clingo_location_copy(loc, &loc_)); }

    Location(Position const &begin, Position const &end) {
        Detail::handle_error(clingo_location_new(c_cast(begin), c_cast(end), &loc_));
    }

    friend auto c_cast(Location const &x) -> clingo_location_t const * { return x.loc_; }

    [[nodiscard]] auto begin() const -> Position { return Position{clingo_location_begin(loc_)}; }

    [[nodiscard]] auto end() const -> Position { return Position{clingo_location_end(loc_)}; }

    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_location_to_string(loc_, c_cast(bld)));
        return std::string{bld.str()};
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_location_hash(loc_); }

    friend auto operator==(Location const &a, Location const &b) -> bool {
        return clingo_location_equal(a.loc_, b.loc_);
    }

    friend auto operator<=>(Location const &a, Location const &b) -> std::strong_ordering {
        return clingo_location_compare(a.loc_, b.loc_) <=> 0;
    }

  private:
    clingo_location_t const *loc_ = nullptr;
};

} // namespace Clingo
