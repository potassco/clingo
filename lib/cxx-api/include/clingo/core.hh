#pragma once

#include <clingo/core.h>

#include <algorithm>
#include <functional>
#include <iterator>
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
#define CLINGO_CATCH_PTR(ptr)                                                                                          \
    catch (...) {                                                                                                      \
        ptr = std::current_exception();                                                                                \
        return clingo_result_unknown;                                                                                  \
    }                                                                                                                  \
    return clingo_result_success
#define CLINGO_CATCH                                                                                                   \
    catch (...) {                                                                                                      \
        Detail::get_exception_ptr() = std::current_exception();                                                        \
        return clingo_result_unknown;                                                                                  \
    }                                                                                                                  \
    return clingo_result_success

inline auto get_exception_ptr() -> std::exception_ptr & {
    thread_local std::exception_ptr ptr;
    return ptr;
}

inline auto handle_error(std::exception_ptr &ptr) -> clingo_result_t {
    try {
        throw;
    } catch (...) {
        ptr = std::current_exception();
    }
    return clingo_result_unknown;
}

//! Map the given result code to an error or rethrow the given pointer if it is
//! not null and has a value.
inline void handle_error_impl(clingo_result_t res, std::exception_ptr *ptr) {
    if (ptr != nullptr) {
        std::rethrow_exception(std::exchange(*ptr, nullptr));
    }
    switch (res) {
        case clingo_result_runtime: {
            throw std::runtime_error("runtime error");
        }
        case clingo_result_logic: {
            throw std::runtime_error("logic error");
        }
        case clingo_result_range: {
            throw std::runtime_error("range error");
        }
        case clingo_result_bad_alloc: {
            throw std::runtime_error("bad alloc");
        }
        default: {
            throw std::runtime_error("unknown error");
        }
    }
}

//! Simple error handling.
//!
//! The function either maps the given result code to an exception or rethrows
//! an exception set in a callback. The exception pointer is cleared at the
//! point where it is rethrown.
inline void handle_error(clingo_result_t res) {
    if (res != clingo_result_success) {
        handle_error_impl(res, &get_exception_ptr());
    }
}

//! Error handling for asynchronous processes using dedicated exception pointers.
inline void handle_error(clingo_result_t res, std::exception_ptr &ptr) {
    if (res != clingo_result_success) {
        handle_error_impl(res, &ptr);
    }
}

inline auto user_data_slot() -> size_t {
    static auto slot = clingo_user_data_slot();
    return slot;
}

//! Use std::transform to build a vector.
template <class It, class Pred> auto transform(It begin, It end, Pred pred) {
    auto p = std::vector<std::invoke_result_t<Pred, typename std::iterator_traits<It>::value_type>>{};
    p.reserve(std::distance(begin, end));
    std::transform(begin, end, std::back_inserter(p), pred);
    return p;
}

//! Use std::transform to build a vector.
template <class Rng, class Pred> auto transform(Rng const &rng, Pred pred) {
    using std::begin;
    using std::end;
    return transform(begin(rng), end(rng), pred);
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

template <class T, class P> class ManagedPtr {
  public:
    ManagedPtr() = default;
    explicit ManagedPtr(P *ptr, bool inc = true) : ptr_{ptr} {
        if (inc) {
            T::acquire(ptr);
        }
    }
    ManagedPtr(ManagedPtr const &other) : ManagedPtr{other.ptr_} {}
    ManagedPtr(ManagedPtr &&other) noexcept : ptr_{std::exchange(other.ptr_, nullptr)} {}
    ~ManagedPtr() noexcept { T::release(ptr_); }
    auto operator=(ManagedPtr const &other) -> ManagedPtr & {
        T::acquire(other.ptr_);
        T::release(ptr_);
        ptr_ = other.ptr_;
        return *this;
    }
    auto operator=(ManagedPtr &&other) noexcept -> ManagedPtr & {
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
    friend auto operator==(ManagedPtr const &p, std::nullptr_t) noexcept -> bool { return p.ptr_ == nullptr; }
    friend auto operator!=(ManagedPtr const &p, std::nullptr_t) noexcept -> bool { return p.ptr_ != nullptr; }
    friend auto operator==(std::nullptr_t, ManagedPtr const &p) noexcept -> bool { return p.ptr_ == nullptr; }
    friend auto operator!=(std::nullptr_t, ManagedPtr const &p) noexcept -> bool { return p.ptr_ != nullptr; }

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

class StringBuffer {
  public:
    explicit StringBuffer(std::string_view sv) {
        if (sv.size() < BUFFER_SIZE) {
            auto &arr = storage_.emplace<StaticArray>();
            *std::ranges::copy_n(sv.data(), std::ssize(sv), arr.data()).out = '\0';
        } else {
            auto &arr = storage_.emplace<DynamicArray>(std::make_unique_for_overwrite<char[]>(sv.size() + 1)); // NOLINT
            *std::ranges::copy_n(sv.data(), std::ssize(sv), arr.get()).out = '\0';
        }
    }
    [[nodiscard]] auto c_str() const -> char const * {
        return std::visit(
            []<class T>(T const &buf) -> char const * {
                if constexpr (std::is_same_v<T, DynamicArray>) {
                    return buf.get();
                } else {
                    return buf.data();
                }
            },
            storage_);
    }

    operator const char *() const { return c_str(); }

  private:
    static constexpr size_t BUFFER_SIZE = 256;
    using DynamicArray = std::unique_ptr<char[]>; // NOLINT
    using StaticArray = std::array<char, BUFFER_SIZE>;

    std::variant<DynamicArray, StaticArray> storage_;
};
} // namespace Detail

using Id = clingo_id_t;

using Literal = clingo_literal_t;
using LiteralSpan = std::span<Literal const>;
using LiteralVector = std::vector<Literal>;

using Weight = clingo_weight_t;
using WeightSpan = std::span<clingo_weight_t const>;

using Sum = int64_t;
using SumSpan = std::span<Sum const>;

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
                                            static_cast<clingo_log_level_t>(level), log ? &logger_ : nullptr, log.get(),
                                            limit, &ptr));
        rep_.reset(ptr, false);
        if (log) {
            Detail::handle_error(
                clingo_lib_set_user_data(rep_.get(), Detail::user_data_slot(), log.release(), &free_logger_));
        }
    }
    explicit Library(clingo_lib_t *rep, bool acquire) : rep_{rep, acquire} {}

    [[nodiscard]] friend auto c_cast(Library const &lib) -> clingo_lib_t * { return lib.rep_.get(); }

  private:
    friend class Detail::ManagedPtr<Library, clingo_lib_t>;

    static void free_logger_(void *data) noexcept { std::unique_ptr<Logger>(static_cast<Logger *>(data)); }
    static void logger_(clingo_message_t code, char const *message, size_t size, void *data) {
        (*static_cast<Logger *>(data))(static_cast<MessageCode>(code), {message, size});
    }
    static auto acquire(clingo_lib_t *ptr) { clingo_lib_acquire(ptr); }
    static auto release(clingo_lib_t *ptr) { clingo_lib_release(ptr); }

    Detail::ManagedPtr<Library, clingo_lib_t> rep_;
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

} // namespace Clingo
