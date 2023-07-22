#pragma once

#include <sstream>

#include <catch2/catch_test_macros.hpp>

#include <input/algo/parse.hh>
#include <input/algo/print.hh>

#include <util/algorithm.hh>
#include <util/print.hh>

namespace Gringo::Input::Test {

template <class T> auto to_str(T const &value) -> std::string { return to_string(value); }

template <class T> auto to_str(std::optional<T> const &value) -> std::string {
    if (value) {
        return to_str(value.value());
    }
    return "<failed>";
}

template <class T> auto to_str(std::vector<T> const &value, char const *sep = ", ") -> std::string {
    std::ostringstream oss;
    oss << "[" << Util::p_range(value, sep) << "]";
    return oss.str();
}

} // namespace Gringo::Input::Test
