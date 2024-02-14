#pragma once

#include <sstream>

#include <tcb/span.hpp>

#include <catch2/catch_test_macros.hpp>

#include <gringo/input/algo/parse.hh>
#include <gringo/input/algo/print.hh>

#include <gringo/util/algorithm.hh>
#include <gringo/util/print.hh>

namespace Gringo::Input::Test {

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

    operator Logger &() { return log_; }

    operator SymbolStore &() { return store_; }

    auto messages() -> tcb::span<std::pair<MessageCode, std::string>> { return messages_; }

    void reset() {
        log_.reset();
        messages_.clear();
    }

  private:
    void push_(MessageCode code, std::string str) { messages_.emplace_back(code, std::move(str)); }

    Logger log_;
    SymbolStore &store_;
    std::vector<std::pair<MessageCode, std::string>> messages_;
};

template <class T> auto to_str(T const &value) -> std::string { return to_string(value); }

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

} // namespace Gringo::Input::Test
