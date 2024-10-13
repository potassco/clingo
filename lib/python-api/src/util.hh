#pragma once

#include <clingo.h>

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// NOLINTBEGIN(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

#define CLINGO_TRY try
#define CLINGO_CATCH                                                                                                   \
    catch (...) {                                                                                                      \
        return handle_error();                                                                                         \
    }                                                                                                                  \
    return clingo_result_success

#define CLINGO_PY_TOTAL_ORDER                                                                                          \
    .def(py::self == py::self)                                                                                         \
        .def(py::self != py::self)                                                                                     \
        .def(py::self < py::self)                                                                                      \
        .def(py::self <= py::self)                                                                                     \
        .def(py::self > py::self)                                                                                      \
        .def(py::self >= py::self)

#define CLINGO_CPP_TOTAL_ORDER(type, T)                                                                                \
    [[maybe_unused]] type auto operator!=(T const &a, T const &b)->bool { return !(a == b); }                          \
    [[maybe_unused]] type auto operator<=(T const &a, T const &b)->bool { return !(b < a); }                           \
    [[maybe_unused]] type auto operator>(T const &a, T const &b)->bool { return b < a; }                               \
    [[maybe_unused]] type auto operator>=(T const &a, T const &b)->bool { return !(a < b); }

// NOLINTEND(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

namespace Clingo::Python {

namespace py = pybind11;

constexpr auto doc(char const *str) -> char const * {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return str + 1;
}

inline void handle_error(clingo_result_t code) {
    switch (static_cast<clingo_result_e>(code)) {
        case clingo_result_success: {
            break;
        }
        case clingo_result_unknown: {
            throw std::runtime_error("unknown error");
        }
        case clingo_result_runtime: {
            throw std::runtime_error("runtime error");
        }
        case clingo_result_logic: {
            throw std::logic_error("logic error");
        }
        case clingo_result_invalid: {
            throw std::invalid_argument("invalid argumets");
        }
        case clingo_result_range: {
            throw std::range_error("range error");
        }
        case clingo_result_bad_alloc: {
            throw std::bad_alloc();
        }
    }
}

inline auto handle_error() -> clingo_result_t {
    try {
        throw;
    } catch (std::invalid_argument const &e) {
        return clingo_result_invalid;
    } catch (std::range_error const &e) {
        return clingo_result_range;
    } catch (std::bad_alloc const &e) {
        return clingo_result_bad_alloc;
    } catch (std::logic_error const &e) {
        return clingo_result_logic;
    } catch (...) {
        return clingo_result_runtime;
    }
}

//! Use std::transform to build a vector.
template <class It, class Pred> auto transform(It begin, It end, Pred pred) {
    auto p = std::vector<std::invoke_result_t<Pred, typename std::iterator_traits<It>::value_type>>{};
    p.reserve(std::distance(begin, end));
    std::transform(begin, end, std::back_inserter(p), pred);
    return p;
}

//! Use std::transform to build a vector.
template <class Rng, class Pred> auto transform(Rng const &rng, Pred pred) {
    using std::begin;
    using std::end;
    return transform_vec(begin(rng), end(rng), pred);
}

} // namespace Clingo::Python
