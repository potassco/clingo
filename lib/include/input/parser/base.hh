#pragma once

#include <deque>
#include <string>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include <util/lexy_report_error.hh>
#include <util/lexy_stream_input.hh>
#include <util/shared_ptr.hh>

#define STRING_TAG(n, v)                                                                                               \
    struct expected_##n {                                                                                              \
        static constexpr char const *name = v;                                                                         \
    }

namespace Gringo::Input {

//! @defgroup parser Parser
//! @ingroup language
//!
//! Data structures and functions for parsing.
//!
//! @{

//! Helper class to attach state information to a reader.
//!
//! @todo Move into separate header.
template <typename Reader, typename State> class StatefulReader : public Reader {
  public:
    //! Construct a stateful reader.
    explicit StatefulReader(Reader reader, State &state) : Reader{std::move(reader)}, state_{&state} {};

    //! Return the state associated with the reader.
    [[nodiscard]] auto state() const -> State & { return *state_; }

  private:
    State *state_;
};

//! Helper class to attach state information to an input.
//!
//! @todo Move into separate header.
template <typename Input, typename State> class StatefulInput {
  public:
    //! Construct a stateful input.
    StatefulInput(Input &input, State &state) : input_{input}, state_{&state} {}

    //! Return the state associated with the input.
    [[nodiscard]] auto reader() const & { return StatefulReader(input_.reader(), *state_); }

  private:
    Input &input_;
    State *state_;
};

//! Helper class to track comments.
class Comments {
  public:
    //! Append the given comment to a list.
    void append(std::string comment) { comments_.emplace_back(std::move(comment)); }
    //! Mark the position of the last comment appended.
    void mark() { mark_ = comments_.size(); }
    //! Return an iterator to the first available comment.
    auto begin() -> std::deque<std::string>::iterator { return comments_.begin(); }
    //! Return an iterator after the last marked comment.
    auto end() -> std::deque<std::string>::iterator { return comments_.begin() + mark_; }
    //! Clear the comments up to the marker.
    void clear() { comments_.erase(comments_.begin(), comments_.begin() + mark_); };

  private:
    std::deque<std::string> comments_;
    size_t mark_ = 0;
};

//! @}

namespace Grammar {

namespace dsl = lexy::dsl;

using encoding = lexy::utf8_encoding;

namespace Detail {

template <typename V, typename T> struct has_state_ {
    using value_t = std::false_type;
};

template <typename T>
struct has_state_<std::void_t<decltype(std::declval<T &>().remaining_input().reader().state())>, T> {
    using value_t = std::true_type;
};

/// Check if the reader associated with the given scanner has a state method.
template <typename T> constexpr bool has_state = has_state_<void, T>::value_t::value;

template <typename T, typename R = Util::shared_ptr<T>> struct construct_sv_ {
    using return_type = R;

    template <typename... Args>
    constexpr auto operator()(Args &&...args) const
        -> std::enable_if_t<std::is_constructible_v<T, Args &&...>, return_type> {
        return Util::construct_shared<T>(std::forward<Args>(args)...);
    }
};

//! Helper to construct a shared pointer and then convert it to another type.
template <typename T, typename R = Util::shared_ptr<T>> constexpr auto construct_sv = construct_sv_<T, R>{};

template <typename T, typename R> struct construct_v_ {
    using return_type = R;

    template <typename... Args>
    constexpr auto operator()(Args &&...args) const
        -> std::enable_if_t<std::is_constructible_v<T, Args &&...>, return_type> {
        return T{std::forward<Args>(args)...};
    }
};

//! Helper to construct an object and then convert it to another type.
template <typename T, typename R> constexpr auto construct_v = construct_v_<T, R>{};

template <typename T, typename B> struct construct_shared_ {
    using return_type = Util::shared_ptr<B>;

    template <typename... Args>
    constexpr auto operator()(Args &&...args) const
        -> std::enable_if_t<std::is_constructible_v<T, Args &&...>, return_type> {
        return Util::construct_shared<T, B>(std::forward<Args>(args)...);
    }
};

//! Helper to construct a shared pointer.
template <typename T, typename R> constexpr auto construct_shared = construct_shared_<T, R>{};

} // namespace Detail

struct block_comment : lexy::scan_production<void> {
    static constexpr char const *name = "block comment";
    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        size_t n = 0;
        auto begin = scanner.position();
        do {
            if (scanner.branch(LEXY_LIT("%*"))) {
                ++n;
                continue;
            }
            if (scanner.branch(LEXY_LIT("*%"))) {
                --n;
                continue;
            }
            scanner.parse(dsl::code_point);
            if (!scanner) {
                return lexy::scan_failed;
            }
        } while (n > 0);
        auto end = scanner.position();
        if constexpr (Detail::has_state<lexy::rule_scanner<Context, Reader>>) {
            scanner.remaining_input().reader().state().append(lexy::as_string<std::string, encoding>(begin, end));
        }
        return scan_result{true};
    }
};

struct comment : lexy::scan_production<void> {
    static constexpr char const *name = "comment";
    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto begin = scanner.position();
        scanner.branch(LEXY_LIT("%"));
        while (!scanner.peek(dsl::newline) && !scanner.is_at_eof()) {
            scanner.parse(dsl::code_point);
            if (!scanner) {
                return lexy::scan_failed;
            }
        };
        auto end = scanner.position();
        if constexpr (Detail::has_state<lexy::rule_scanner<Context, Reader>>) {
            scanner.remaining_input().reader().state().append(lexy::as_string<std::string, encoding>(begin, end));
        }
        return scan_result{true};
    }
};

struct control {
    struct expected_bc_close {
        static constexpr char const *name = "unclosed block comment";
    };
    struct expected_nl {
        static constexpr char const *name = "unterminated comment";
    };
    static constexpr auto whitespace = dsl::ascii::space | dsl::newline |
                                       dsl::peek(LEXY_LIT("%*")) >>
                                           (dsl::token(dsl::p<block_comment>) | dsl::error<expected_bc_close>) |
                                       dsl::peek(LEXY_LIT("%")) >>
                                           (dsl::token(dsl::p<comment>) | dsl::error<expected_nl>);
};

} // namespace Grammar

} // namespace Gringo::Input
