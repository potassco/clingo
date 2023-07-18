#pragma once

//! @file
//! This file contains utilities for visiting generic data structures.

#include <optional>
#include <tuple>
#include <variant>
#include <vector>

#include <util/shared_ptr.hh>

namespace Gringo::Input {

//! A visitor that eases visiting standard types.
template <class T> class Visitor {
  public:
    //! Recursively visit the given type.
    template <class U> void visit(U const &x) const {
        if constexpr (std::is_invocable_r_v<void, T, U const &>) {
            static_cast<T const *>(this)->operator()(x);
        } else {
            visit_(x);
        }
    }

    //! Recursively visit all of the given arguments.
    template <class... U> void visit(U... args) const { (visit(args), ...); }

  private:
    template <class U> void visit_(std::optional<U> const &opt) const {
        if (opt.has_value()) {
            visit(opt.value());
        }
    }

    template <class U> auto visit_(Util::shared_ptr<U> const &ptr) const { visit(*ptr); }

    template <class U, class V> auto visit_(std::pair<U, V> const &pair) const {
        visit(pair.first);
        visit(pair.second);
    }

    template <class... U, size_t... I>
    void visit_(std::tuple<U...> const &tup, std::index_sequence<I...> indices) const {
        static_cast<void>(indices);
        (visit(std::get<I>(tup)), ...);
    }

    template <class... U> void visit_(std::tuple<U...> const &tup) const {
        return visit_(tup, std::index_sequence_for<U...>{});
    }

    template <class... U> void visit_(std::variant<U...> const &var) const {
        return std::visit([this](auto const &elem) { this->visit(elem); }, var);
    }

    template <class U> void visit_(std::vector<U> const &vec) const {
        for (auto &elem : vec) {
            visit(elem);
        }
    }
};

} // namespace Gringo::Input
