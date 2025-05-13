#pragma once

#include <clingo/input/parser.hh>
#include <clingo/input/print.hh>

#include <clingo/input/rewrite/rewrite_context.hh>

#include <clingo/util/algorithm.hh>
#include <clingo/util/print.hh>

#include <catch2/catch_test_macros.hpp>

#include <span>
#include <sstream>

namespace CppClingo::Input::Test {

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
        : log_{[this](MessageCode code, std::string_view str) { push_(code, str); }}, store_{default_store()},
          prs_{log_, store_} {}

    auto term(std::string_view str) -> std::optional<Term> {
        reset();
        prs_.init(str, *store_.string("<string>"));
        return prs_.parse_term();
    }

    auto literal(std::string_view str) -> std::optional<Lit> {
        reset();
        prs_.init(str, *store_.string("<string>"));
        return prs_.parse_literal();
    }

    auto head_literal(std::string_view str) -> std::optional<HdLit> {
        reset();
        prs_.init(str, *store_.string("<string>"));
        return prs_.parse_head_literal();
    }

    auto body_literal(std::string_view str) -> std::optional<BdLit> {
        reset();
        prs_.init(str, *store_.string("<string>"));
        return prs_.parse_body_literal();
    }

    auto statement(std::string_view str) -> std::optional<Stm> {
        reset();
        prs_.init(str, *store_.string("<string>"));
        return prs_.parse_statement();
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
    void push_(MessageCode code, std::string_view str) { messages_.emplace_back(code, str); }

    Logger log_;
    RewriteOptions opts_;
    TheoryAtomParser parser_;
    ConstMap const_map_;
    ParamMap param_map_;
    SymbolStore &store_;
    Parser prs_;
    RewriteContext ctx_ = RewriteContext{log_, store_, opts_, parser_, param_map_, const_map_};
    std::vector<std::pair<MessageCode, std::string>> messages_;
};

struct Print {
    template <class T> auto operator()(T const &value) const -> std::string { return to_string(value); }

    auto operator()(bool const &value) const -> std::string { return value ? "T" : "F"; }

    auto operator()(std::string const &x) const -> std::string { return x; }

    auto operator()(SharedSymbol const &x) const -> std::string {
        std::ostringstream oss;
        oss << *x;
        return oss.str();
    }

    auto operator()(SharedString const &x) const -> std::string {
        std::ostringstream oss;
        oss << *x;
        return oss.str();
    }

    template <class T> auto operator()(std::optional<T> const &value) const -> std::string {
        if (value) {
            return operator()(value.value());
        }
        return "<failed>";
    }

    template <class T> auto operator()(Util::immutable_array<T> const &value) const -> std::string {
        return operator()(std::span(value.data(), value.size()));
    }

    template <class T> auto operator()(std::vector<T> const &value) const -> std::string {
        return operator()(std::span(value.data(), value.size()));
    }

    template <class T> auto operator()(std::span<T> const &value) const -> std::string {
        std::ostringstream oss;
        oss << "[";
        bool comma = false;
        for (auto &x : value) {
            if (comma) {
                oss << sep;
            } else {
                comma = true;
            }
            oss << operator()(x);
        }
        oss << "]";
        return oss.str();
    }

    template <class T, class U> auto operator()(std::pair<T, U> const &x) const -> std::string {
        std::ostringstream oss;
        oss << "(" << operator()(x.first) << ", " << operator()(x.second) << ")";
        return oss.str();
    }

    char const *sep = ", ";
};

template <class T> auto to_str(T const &value, char const *sep = ", ") -> std::string {
    return Print{sep}(value);
}

} // namespace CppClingo::Input::Test
