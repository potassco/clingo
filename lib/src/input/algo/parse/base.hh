#pragma once

#include <functional>
#include <queue>
#include <string>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include <util/shared_ptr.hh>

#include "report_error.hh"
#include "stateful_input.hh"
#include "stream_input.hh"

#define STRING_TAG(n, v)                                                                                               \
    struct expected_##n {                                                                                              \
        static constexpr char const *name = v;                                                                         \
    }

namespace Gringo::Input::Grammar {

namespace dsl = lexy::dsl;

using encoding = lexy::utf8_encoding;

namespace Detail {

/// Check if the reader associated with the given scanner has a state method.
template <typename T, typename V = void> static constexpr bool has_state = false;

template <typename T>
static constexpr bool has_state<T, std::void_t<decltype(std::declval<T &>().remaining_input().reader().state())>> =
    true;

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

//! Helper to inject the state object of the parser.
template <class R, class... CB> constexpr auto with_state(CB... cb) {
    return lexy::bind(lexy::callback<R>([cb](auto &state, auto &&...args)
                                            -> std::invoke_result_t<decltype(cb), decltype(state), decltype(args)...> {
                          return std::invoke(cb, state, std::forward<decltype(args)>(args)...);
                      }...),
                      lexy::parse_state, lexy::values);
}

//! Construct a location from the two given iterators.
auto loc(auto &state, auto begin, auto end) {
    auto pos_end = state.pos(end);
    auto pos_begin = state.pos(begin);
    return Location{std::move(pos_begin), std::move(pos_end)};
}

//! Construct a location from the given lexeme.
auto loc(auto &state, auto token) { return loc(state, token.begin(), token.end()); }

//! Convert a lexeme or sink to string.
static constexpr auto as_string = lexy::as_string<std::string, encoding>;

//! Get the location after the given rule.
//!
//! Best used with token rules.
static constexpr auto post_position = [](auto rule) { return dsl::no_whitespace(rule >> dsl::position); };

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
            scanner.remaining_input().reader().state().push(Detail::as_string(begin, end));
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
            scanner.remaining_input().reader().state().push(Detail::as_string(begin, end));
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

} // namespace Gringo::Input::Grammar
