#pragma once

#include <string>
#include <vector>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include <util/lexy_report_error.hh>
#include <util/lexy_stream_input.hh>

#define STRING_TAG(n, v)                                                                                               \
    struct expected_##n {                                                                                              \
        static constexpr char const *name = v;                                                                         \
    }

template <typename Reader, typename State> class StatefulReader : public Reader {
  public:
    explicit StatefulReader(Reader reader, State &state) : Reader{std::move(reader)}, state_{&state} {};

    [[nodiscard]] auto state() const -> State & { return *state_; }

  private:
    State *state_;
};

template <typename Input, typename State> class StatefulInput {
  public:
    StatefulInput(Input &input, State &state) : input_{input}, state_{&state} {}
    [[nodiscard]] auto reader() const & { return StatefulReader(input_.reader(), *state_); }

  private:
    Input &input_;
    State *state_;
};

class Comments {
  public:
    void append(std::string comment) { comments_.emplace_back(std::move(comment)); }
    void mark() { mark_ = comments_.size(); }
    auto begin() -> std::vector<std::string>::iterator { return comments_.begin(); }
    auto end() -> std::vector<std::string>::iterator { return comments_.begin() + mark_; }
    void clear() { comments_.erase(comments_.begin(), comments_.begin() + mark_); };

  private:
    std::vector<std::string> comments_;
    size_t mark_ = 0;
};

namespace grammar {

namespace dsl = lexy::dsl;

using encoding = lexy::utf8_encoding;

namespace detail {

template <typename V, typename T> struct has_state_ {
    using value_t = std::false_type;
};

template <typename T>
struct has_state_<std::void_t<decltype(std::declval<T &>().remaining_input().reader().state())>, T> {
    using value_t = std::true_type;
};

/// Check if the reader associated with the given scanner has a state method.
template <typename T> constexpr bool has_state = has_state_<void, T>::value_t::value;

} // namespace detail

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
        if constexpr (detail::has_state<lexy::rule_scanner<Context, Reader>>) {
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
        if constexpr (detail::has_state<lexy::rule_scanner<Context, Reader>>) {
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

} // namespace grammar
