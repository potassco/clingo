#pragma once

#include <clingo/core.h>
#include <clingo/symbol.h>

#include <clingo/control/solver.hh>
#include <clingo/core/logger.hh>
#include <clingo/core/symbol.hh>

#include <stdexcept> // IWYU pragma: keep

struct user_data_deleter {
    void operator()(void *data) const {
        if (deleter != nullptr) {
            deleter(data);
        }
    }
    void (*deleter)(void *data) = nullptr;
};

struct clingo_lib {
    clingo_lib(Clingo::Logger log, std::unique_ptr<Clingo::SymbolStore> store, void *data, bool fast_release)
        : log{std::move(log)}, store{std::move(store)}, data{data}, fast_release{fast_release} {}
    Clingo::Logger log;
    Clingo::Control::Scripts scripts;
    std::unique_ptr<Clingo::SymbolStore> store;
    void *data;
    clingo_lib_t *next_ = nullptr;
    std::atomic<size_t> ref_count = 1;
    std::vector<std::unique_ptr<void, user_data_deleter>> user_data;
    bool fast_release;
};

static constexpr auto c_cast(std::strong_ordering cmp) noexcept -> int {
    // NOLINTNEXTLINE(readability-avoid-nested-conditional-operator)
    return (cmp < 0) ? -1 : ((cmp == 0) ? 0 : 1);
}

inline auto c_cast(Clingo::Location const *loc) -> clingo_location_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_location_t const *>(loc);
}

inline auto cpp_cast(clingo_location const *loc) -> Clingo::Location const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Location const *>(loc);
}

inline auto c_cast(Clingo::Symbol const *sym) -> clingo_symbol_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_symbol_t const *>(sym);
}

inline auto c_cast(Clingo::SharedSymbol const *sym) -> clingo_symbol_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_symbol_t const *>(sym);
}

inline auto cpp_cast(clingo_symbol_t sym) -> Clingo::Symbol {
    return Clingo::Symbol::from_rep(sym);
}

inline auto cpp_cast(clingo_symbol_t const *sym) -> Clingo::Symbol const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Symbol const *>(sym);
}

inline auto map(clingo_weighted_literal_t const *lits, size_t size) -> Potassco::WeightLitSpan {
    // NOLINTNEXTLINE
    return Potassco::WeightLitSpan{reinterpret_cast<Potassco::WeightLit const *>(lits), size};
}

inline auto map(Potassco::WeightLitSpan lits) -> clingo_weighted_literal_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_weighted_literal_t const *>(lits.data());
}

class Error {
  public:
    static auto instance() noexcept -> Error &;
    void forward(Error &other) noexcept;
    void set(clingo_result_t code, char const *message, size_t size) noexcept;
    void get(clingo_result_t *code, clingo_string_t *message) noexcept;
    void clear() noexcept;
    [[nodiscard]] auto active() const noexcept -> bool { return code_ != clingo_result_success; }
    void raise();
    auto store() noexcept -> clingo_result_t;

  private:
    auto set_(clingo_result_t code, char const *message) noexcept -> clingo_result_t;

    clingo_result_t code_ = clingo_result_success;
    std::string message_;
};

class ClingoError : public std::exception {
  public:
    ClingoError(clingo_result_t code) : code_{code} {}
    [[nodiscard]] auto what() const noexcept -> char const * override { return "solving failed"; }
    [[nodiscard]] auto code() const -> clingo_result_t { return code_; }

  private:
    clingo_result_t code_;
};

void raise_error(Error *error);
auto store_error(Error *error) -> clingo_result_t;

inline void handle_error(clingo_result_t code) {
    if (code != clingo_result_success) {
        raise_error(nullptr);
    }
}

inline void handle_error(clingo_result_t code, Error &error) {
    if (code != clingo_result_success || error.active()) {
        raise_error(&error);
    }
}

template <typename In, typename C, typename Pred> auto append_n(In begin, size_t n, C &out, Pred pred) {
    out.reserve(out.size() + n);
    for (auto end = begin + n; begin != end; ++begin) {
        out.emplace_back(pred(*begin));
    }
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define CLINGO_TRY try
#define CLINGO_CATCH                                                                                                   \
    catch (...) {                                                                                                      \
        return store_error(nullptr);                                                                                   \
    }                                                                                                                  \
    return clingo_result_success
#define CLINGO_CATCH_ERROR(x)                                                                                          \
    catch (...) {                                                                                                      \
        return store_error(&(x));                                                                                      \
    }                                                                                                                  \
    return clingo_result_success

// NOLINTEND(cppcoreguidelines-macro-usage)
