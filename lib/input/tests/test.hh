#pragma once

#include <gringo/input/print.hh>

#include <gringo/input/rewrite/parse.hh>
#include <gringo/input/rewrite/rewrite_context.hh>

#include <gringo/util/algorithm.hh>
#include <gringo/util/print.hh>

#include <catch2/catch_test_macros.hpp>

#include <span>
#include <sstream>

namespace Gringo::Input::Test {

template <class T> inline auto opt_value(T &&opt) {
    REQUIRE(opt);
    if (opt) {
        return *std::forward<T>(opt);
    }
    throw std::bad_optional_access();
}

inline auto default_store() -> SymbolStore & {
    static auto store = make_symbol_store(true, true);
    return *store;
}

class ParseHelper {
  public:
    ParseHelper()
        : log_{[this](MessageCode code, std::string str) { push_(code, std::move(str)); }}, store_{default_store()} {}

    auto term(std::string_view str) -> std::optional<Term> {
        reset();
        return Gringo::Input::parse_term(log_, default_store(), str);
    }

    auto literal(std::string_view str) -> std::optional<Lit> {
        reset();
        return Gringo::Input::parse_literal(log_, default_store(), str);
    }

    auto head_literal(std::string_view str) -> std::optional<HdLit> {
        reset();
        return Gringo::Input::parse_head_literal(log_, default_store(), str);
    }

    auto body_literal(std::string_view str) -> std::optional<BdLit> {
        reset();
        return Gringo::Input::parse_body_literal(log_, default_store(), str);
    }

    auto statement(std::string_view str) -> std::optional<Stm> {
        reset();
        return Gringo::Input::parse_statement(log_, default_store(), str);
    }

    auto logger() -> Logger & { return *this; }

    auto store() -> SymbolStore & { return *this; }

    auto ctx() -> RewriteContext & { return *this; }

    auto parser() -> TheoryAtomParser & { return parser_; }

    operator Logger &() { return log_; }

    operator SymbolStore &() { return store_; }

    operator RewriteContext &() {
        ctx_.init({}, "__A_");
        return ctx_;
    }

    auto messages() -> std::span<std::pair<MessageCode, std::string>> { return messages_; }

    void reset() {
        log_.reset();
        messages_.clear();
    }

    void check() {
        if (ctx_.has_error() || parser_.has_error()) {
            throw rewrite_error();
        }
    }

  private:
    void push_(MessageCode code, std::string str) { messages_.emplace_back(code, std::move(str)); }

    Logger log_;
    RewriteOptions opts_;
    TheoryAtomParser parser_;
    ConstMap const_map_;
    ParamMap param_map_;
    SymbolStore &store_;
    RewriteContext ctx_ = RewriteContext{log_, store_, opts_, parser_, param_map_, const_map_};
    std::vector<std::pair<MessageCode, std::string>> messages_;
};

template <class T> auto to_str(T const &value) -> std::string { return to_string(value); }

inline auto to_str(bool const &value) -> std::string { return value ? "T" : "F"; }

template <class T> auto to_str(std::optional<T> const &value) -> std::string {
    if (value) {
        return to_str(value.value());
    }
    return "<failed>";
}

template <class T> auto to_str(Util::immutable_array<T> const &value, char const *sep = ", ") -> std::string {
    std::ostringstream oss;
    oss << "[" << Util::p_range(value, sep) << "]";
    return oss.str();
}

template <class T> auto to_str(std::vector<T> const &value, char const *sep = ", ") -> std::string {
    std::ostringstream oss;
    oss << "[" << Util::p_range(value, sep) << "]";
    return oss.str();
}

template <class T, class U> auto to_str(std::pair<T, U> const &x) -> std::string {
    std::ostringstream oss;
    oss << "(" << to_str(x.first) << ", " << to_str(x.second) << ")";
    return oss.str();
}

} // namespace Gringo::Input::Test
