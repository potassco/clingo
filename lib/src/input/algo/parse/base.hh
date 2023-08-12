#pragma once

#include <functional>
#include <queue>
#include <string>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include <input/statement.hh>

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

//! Convert a lexeme or sink to string.
static constexpr auto as_string = lexy::as_string<std::string, encoding>;

struct position_ : lexyd::rule_base {
    template <typename NextParser> struct p {
        template <typename Context, typename Reader, typename... Args>
        LEXY_PARSER_FUNC static auto parse(Context &context, Reader &reader, Args &&...args) -> bool {
            auto pos = reader.position();
            context.on(lexyd::_ev::token{}, lexy::position_token_kind, pos, pos);
            return NextParser::parse(context, reader, LEXY_FWD(args)..., context.control_block->parse_state->pos(pos));
        }
    };
};

template <typename Rule> struct position_rule_ : lexyd::_copy_base<Rule> {
    template <typename Reader> struct bp {
        lexy::branch_parser_for<Rule, Reader> rule;

        template <typename ControlBlock> constexpr auto try_parse(const ControlBlock *cb, const Reader &reader) {
            return rule.try_parse(cb, reader);
        }

        template <typename Context> constexpr void cancel(Context &context) { rule.cancel(context); }

        template <typename NextParser, typename Context, typename... Args>
        LEXY_PARSER_FUNC auto finish(Context &context, Reader &reader, Args &&...args) {
            auto pos = reader.position();
            context.on(lexyd::_ev::token{}, lexy::position_token_kind, pos, pos);
            return rule.template finish<NextParser>(context, reader, LEXY_FWD(args)...,
                                                    context.control_block->parse_state->pos(pos));
        }
    };

    template <typename NextParser> struct p {
        template <typename Context, typename Reader, typename... Args>
        LEXY_PARSER_FUNC static auto parse(Context &context, Reader &reader, Args &&...args) -> bool {
            auto pos = reader.position();
            context.on(lexyd::_ev::token{}, lexy::position_token_kind, pos, pos);
            return lexy::parser_for<Rule, NextParser>::parse(context, reader, LEXY_FWD(args)...,
                                                             context.control_block->parse_state->pos(pos));
        }
    };
};

struct postition_dsl_ : position_ {
    template <typename Rule> constexpr auto operator()(Rule ph) const {
        static_cast<void>(ph);
        return position_rule_<Rule>{};
    }
};

struct post_position_ : lexyd::rule_base {
    template <typename NextParser> struct p {
        template <typename Context, typename Reader, typename... Args>
        LEXY_PARSER_FUNC static auto parse(Context &context, Reader &reader, Args &&...args) -> bool {
            auto pos = reader.position();
            context.on(lexyd::_ev::token{}, lexy::position_token_kind, pos, pos);
            return NextParser::parse(context, reader, LEXY_FWD(args)...,
                                     context.control_block->parse_state->post_pos(pos));
        }
    };
};

struct post_position_dsl_ : post_position_ {
    template <typename Rule> constexpr auto operator()(Rule ph) const { return ph >> post_position_{}; }
};

template <typename Rule> struct location_rule_ : lexyd::_copy_base<Rule> {
    struct location_ {};

    LEXY_PARSER_FUNC static auto map_location_(location_ ph, Location &loc) {
        static_cast<void>(ph);
        return std::move(loc);
    }

    template <class T> LEXY_PARSER_FUNC static auto map_location_(T arg, Location &loc) {
        static_cast<void>(loc);
        return arg;
    }

    struct pp : lexyd::rule_base {
        template <typename NextParser> struct p {
            template <typename Context, typename Reader, typename... Args>
            LEXY_PARSER_FUNC static auto parse(Context &context, Reader &reader, Position begin, Args &&...args)
                -> bool {
                auto end = reader.position();
                auto loc = Location{std::move(begin), context.control_block->parse_state->post_pos(end)};
                context.on(lexyd::_ev::token{}, lexy::position_token_kind, end, end);
                return NextParser::parse(context, reader, map_location_(LEXY_FWD(args), loc)...);
            }
        };
    };

    template <typename Reader> struct bp {
        using rule_pp = decltype(std::declval<Rule>() >> pp{});
        lexy::branch_parser_for<rule_pp, Reader> rule;

        template <typename ControlBlock> constexpr auto try_parse(const ControlBlock *cb, const Reader &reader) {
            return rule.try_parse(cb, reader);
        }

        template <typename Context> constexpr void cancel(Context &context) { rule.cancel(context); }

        template <typename NextParser, typename Context, typename... Args>
        LEXY_PARSER_FUNC auto finish(Context &context, Reader &reader, Args &&...args) {
            auto begin = reader.position();
            auto pos = context.control_block->parse_state->pos(begin);
            context.on(lexyd::_ev::token{}, lexy::position_token_kind, begin, begin);
            return rule.template finish<NextParser>(context, reader, std::move(pos), LEXY_FWD(args)..., location_{});
        }
    };

    template <typename NextParser> struct p {
        using rule_pp = decltype(std::declval<Rule>() + pp{});
        template <typename Context, typename Reader, typename... Args>
        LEXY_PARSER_FUNC static auto parse(Context &context, Reader &reader, Args &&...args) -> bool {
            auto begin = reader.position();
            auto pos = context.control_block->parse_state->pos(begin);
            context.on(lexyd::_ev::token{}, lexy::position_token_kind, begin, begin);
            return lexy::parser_for<rule_pp, NextParser>::parse(context, reader, std::move(pos), LEXY_FWD(args)...,
                                                                location_{});
        }
    };
};

//! Produces a position to the current reader position without parsing anything.
constexpr auto position = postition_dsl_{};

//! Produce a position ignoring leading whitespace.
constexpr auto post_position = post_position_dsl_{};

//! Produce a location for the given rule.
template <typename Rule> constexpr auto location(Rule ph) {
    static_cast<void>(ph);
    return location_rule_<Rule>{};
}

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
            auto &state = scanner.remaining_input().reader().state();
            state.end_token(begin, end);
            state.push(state.loc(begin, end), CommentType::block, Detail::as_string(begin, end));
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
            auto &state = scanner.remaining_input().reader().state();
            scanner.remaining_input().reader().state().end_token(begin, end);
            state.push(state.loc(begin, end), CommentType::line, Detail::as_string(begin, end));
        }
        return scan_result{true};
    }
};

struct gobble : lexy::scan_production<void> {
    static constexpr char const *name = "whitespace";
    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto begin = scanner.position();
        while (scanner.branch(dsl::ascii::space | dsl::newline)) {
        }
        auto end = scanner.position();
        if constexpr (Detail::has_state<lexy::rule_scanner<Context, Reader>>) {
            scanner.remaining_input().reader().state().end_token(begin, end);
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
    static constexpr auto
        whitespace = dsl::peek(dsl::ascii::newline / dsl::ascii::space) >> dsl::token(dsl::p<gobble>) |
                     dsl::peek(LEXY_LIT("%*")) >> (dsl::token(dsl::p<block_comment>) | dsl::error<expected_bc_close>) |
                     dsl::peek(LEXY_LIT("%")) >> (dsl::token(dsl::p<comment>) | dsl::error<expected_nl>);
};

} // namespace Gringo::Input::Grammar
