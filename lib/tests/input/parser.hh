#pragma once

#include <sstream>

#include <input/algo/parse.hh>
#include <input/algo/print.hh>
#include <input/algo/project.hh>
#include <input/algo/unpool.hh>
#include <input/algo/visit_variables.hh>

#include <util/algorithm.hh>
#include <util/print.hh>

namespace Gringo::Input::Test {

template <class T> auto to_str(T const &value) -> std::string { return to_string(value); }

template <class T> auto to_str(Util::shared_ptr<T> const &value) -> std::string { return to_string(*value); }

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

template <class T> auto unpool_str(std::optional<T> value, char const *sep = ", ") -> std::string {
    if (value) {
        auto unpooled = unpool(value.value());
        if (!unpooled.has_value()) {
            unpooled = Util::make_vec<T>(value.value());
        }
        return to_str(unpooled.value(), sep);
    }
    return "<failed>";
}

template <class T> auto unpool_str(std::optional<Util::shared_ptr<T>> value, char const *sep = ", ") -> std::string {
    if (value) {
        auto unpooled = value.value()->unpool();
        if (!unpooled.has_value()) {
            unpooled = Util::make_vec<Util::shared_ptr<T>>(value.value());
        }
        return to_str(unpooled.value(), sep);
    }
    return "<failed>";
}

template <class T> auto project_str(std::optional<T> value) -> std::string {
    if (value) {
        return to_str(project(value.value(), ProjectionMode::pure, true).value_or(value.value()));
    }
    return "<failed>";
}

template <class T> auto project_str(std::optional<Util::shared_ptr<T>> value) -> std::string {
    if (value) {
        return to_str(value.value()->project(ProjectionMode::pure, true).value_or(value.value()));
    }
    return "<failed>";
}

template <class T> auto variables_str(T const &value) -> std::string {
    auto vars = select_variables(value);
    auto sorted = std::vector<VariableSet::value_type>{vars.begin(), vars.end()};
    std::sort(sorted.begin(), sorted.end());
    return to_str(sorted);
}

template <class T> auto variables_str(Util::shared_ptr<T> const &value) -> std::string {
    if (value) {
        return variables_str(*value);
    }
    return "<failed>";
}

template <class T> auto variables_str(std::optional<T> const &value) -> std::string {
    if (value.has_value()) {
        return variables_str(value.value());
    }
    return "<failed>";
}

} // namespace Gringo::Input::Test
