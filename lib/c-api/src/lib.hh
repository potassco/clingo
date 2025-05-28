#pragma once

#include <clingo/core.h>
#include <clingo/symbol.h>

#include <clingo/control/solver.hh>
#include <clingo/core/logger.hh>
#include <clingo/core/symbol.hh>

#include <stdexcept> // IWYU pragma: keep

struct clingo_lib {
    clingo_lib(CppClingo::Logger log, std::unique_ptr<CppClingo::SymbolStore> store, void *data, bool fast_release)
        : log{std::move(log)}, store{std::move(store)}, data{data}, fast_release{fast_release} {}
    CppClingo::Logger log;
    CppClingo::Control::Scripts scripts;
    std::unique_ptr<CppClingo::SymbolStore> store;
    void *data;
    clingo_lib_t *next_ = nullptr;
    std::atomic<size_t> ref_count = 1;
    bool fast_release;
};

namespace CppClingo::CAPI {

class fill {
  public:
    fill(size_t n, char c = ' ') : n_{n}, c_{c} {}
    friend auto operator<<(CppClingo::Util::OutputBuffer &out, fill const &x) -> CppClingo::Util::OutputBuffer & {
        std::ranges::fill(out.reserve(static_cast<std::ptrdiff_t>(x.n_)), x.c_);
        return out;
    }

  private:
    size_t n_;
    char c_;
};

static constexpr auto c_cast(std::strong_ordering cmp) noexcept -> int {
    // NOLINTNEXTLINE(readability-avoid-nested-conditional-operator)
    return (cmp < 0) ? -1 : ((cmp == 0) ? 0 : 1);
}

inline auto c_cast(CppClingo::Location const *loc) -> clingo_location_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_location_t const *>(loc);
}

inline auto cpp_cast(clingo_location const *loc) -> CppClingo::Location const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<CppClingo::Location const *>(loc);
}

inline auto c_cast(CppClingo::Symbol const *sym) -> clingo_symbol_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_symbol_t const *>(sym);
}

inline auto c_cast(CppClingo::SharedSymbol const *sym) -> clingo_symbol_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_symbol_t const *>(sym);
}

inline auto cpp_cast(clingo_symbol_t sym) -> CppClingo::Symbol {
    return CppClingo::Symbol::from_rep(sym);
}

inline auto cpp_cast(clingo_symbol_t const *sym) -> CppClingo::Symbol const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<CppClingo::Symbol const *>(sym);
}

inline auto map(clingo_weighted_literal_t const *lits, size_t size) -> Potassco::WeightLitSpan {
    // NOLINTNEXTLINE
    return Potassco::WeightLitSpan{reinterpret_cast<Potassco::WeightLit const *>(lits), size};
}

inline auto map(Potassco::WeightLitSpan lits) -> clingo_weighted_literal_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_weighted_literal_t const *>(lits.data());
}

void raise_error();
auto store_error() -> bool;
auto fail_arguments() -> bool;
auto fail_with(clingo_result_t code, std::string_view msg) -> bool;

inline void handle_error(bool res) {
    if (!res) {
        raise_error();
    }
}

inline void handle_error_no_code(bool res);

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
        return store_error();                                                                                          \
    }                                                                                                                  \
    return true

// NOLINTEND(cppcoreguidelines-macro-usage)

} // namespace CppClingo::CAPI
