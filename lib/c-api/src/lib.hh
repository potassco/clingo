#pragma once

#include <clingo/core.h>
#include <clingo/symbol.h>

#include <clingo/control/solver.hh>
#include <clingo/core/logger.hh>
#include <clingo/core/symbol.hh>

#include <stdexcept>

struct clingo_lib {
    clingo_lib(Clingo::Logger log, std::unique_ptr<Clingo::SymbolStore> store,
               std::unique_ptr<void, clingo_free_t> data)
        : log{std::move(log)}, store{std::move(store)}, data{std::move(data)} {}
    Clingo::Logger log;
    Clingo::Control::Scripts scripts;
    std::unique_ptr<Clingo::SymbolStore> store;
    std::unique_ptr<void, clingo_free_t> data;
    clingo_lib_t *next_ = nullptr;
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

class ClingoError : public std::exception {
  public:
    ClingoError(clingo_result_t code) : code_{code} {}
    [[nodiscard]] auto code() const -> clingo_result_t { return code_; }

  private:
    clingo_result_t code_;
};

inline void handle_error(clingo_result_t code) {
    if (code != clingo_result_success) {
        throw ClingoError(code);
    }
}

inline auto handle_error() -> clingo_result_t {
    try {
        throw;
    } catch (std::bad_alloc const &) {
        return clingo_result_bad_alloc;
    } catch (std::range_error const &) {
        return clingo_result_range;
    } catch (std::invalid_argument const &) {
        return clingo_result_invalid;
    } catch (std::logic_error const &) {
        return clingo_result_logic;
    } catch (ClingoError const &e) {
        return e.code();
    } catch (...) {
        return clingo_result_runtime;
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
        return handle_error();                                                                                         \
    }                                                                                                                  \
    return clingo_result_success

// NOLINTEND(cppcoreguidelines-macro-usage)
