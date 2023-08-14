#pragma once

#include <optional>
#include <variant>
#include <vector>

#include <util/algorithm.hh>
#include <util/shared_ptr.hh>

namespace Gringo::Input {

template <class U> struct TranslateArgument {
    U const &orig;
    std::optional<U> transformed;
};

template <class T> class Transformer {
  public:
    template <class U> auto tr(U const &arg) const { return TranslateArgument<U>{arg, std::nullopt}; }

    template <class U> auto transform(U const &x) const -> std::optional<U> {
        if constexpr (std::is_invocable_r_v<std::optional<U>, T, U const &>) {
            return static_cast<T const *>(this)->operator()(x);
        } else {
            return transform_(x);
        }
    }

    template <class U, class... Args> auto transform_construct(Args &&...args) const -> std::optional<U> {
        (apply_(args), ...);
        if ((has_value_(args) || ...)) {
            return {U{get_value_(std::forward<Args>(args))...}};
        }
        return std::nullopt;
    }

  private:
    template <class... U, size_t... I>
    auto transform_(std::tuple<U...> const &tup, std::index_sequence<I...> indices,
                    std::optional<U>... transfomed) const -> std::optional<std::tuple<U...>> {
        static_cast<void>(indices);
        if ((transfomed.has_value() || ...)) {
            return {std::move(transfomed).value_or(std::get<I>(tup))...};
        }
        return std::nullopt;
    }

    template <size_t i, class... U, class... Args>
    auto transform_(std::tuple<U...> const &tup, Args &&...args) const -> std::optional<std::tuple<U...>> {
        // Note: we have to use the complicated recursive version because the
        // C++ standard leaves the order of argument evaluation unspecified.
        if constexpr (i == sizeof...(U)) {
            return transform_(tup, std::forward<Args>(args)..., std::index_sequence_for<U...>{});
        } else {
            return transform_(tup, std::forward<Args>(args)..., transform(std::get<i>(tup)));
        }
    }

    template <class... U> auto transform_(std::tuple<U...> const &tup) const -> std::optional<std::tuple<U...>> {
        return transform_<0>(tup);
    }

    template <class U> auto transform_(std::optional<U> const &opt) const -> std::optional<std::optional<U>> {
        if (opt.has_value()) {
            // Note that the transformer will never remove an optional. If this
            // is desired, then the visitor extending the transformer should
            // handle this case.
            if (auto ret = transform(opt.value()); ret.has_value()) {
                return {std::move(ret)};
            }
        }
        return std::nullopt;
    }

    template <class U> auto transform_(Util::shared_ptr<U> const &ptr) const -> std::optional<Util::shared_ptr<U>> {
        return Util::map_opt(transform(*ptr), [](U val) { return Util::construct_shared<U>(std::move(val)); });
    }

    template <class U, class V> auto transform_(std::pair<U, V> const &pair) const -> std::optional<std::pair<U, V>> {
        auto first = transform(pair.first);
        auto second = transform(pair.second);
        if (first.has_value() || second.has_value()) {
            return std::pair<U, V>{std::move(first).value_or(pair.first), std::move(second).value_or(pair.second)};
        }
        return std::nullopt;
    }

    template <class... U> auto transform_(std::variant<U...> const &var) const -> std::optional<std::variant<U...>> {
        return std::visit(
            [this](auto const &x) -> std::optional<std::variant<U...>> {
                if (auto ret = this->transform(x); ret.has_value()) {
                    return std::variant<U...>{std::move(ret).value()};
                }
                return std::nullopt;
            },
            var);
    }

    template <class U> auto transform_(std::vector<U> const &vec) const -> std::optional<std::vector<U>> {
        size_t n = 0;
        std::optional<std::vector<U>> ret;
        for (auto const &elem : vec) {
            std::optional<U> transformed = transform(elem);
            if (transformed.has_value() && !ret.has_value()) {
                ret = Util::copy_n(vec, n);
            }
            if (ret.has_value()) {
                ret->emplace_back(std::move(transformed).value_or(elem));
            }
            ++n;
        }
        return ret;
    }

    template <class U> auto apply_(TranslateArgument<U> &arg) const { return arg.transformed = transform(arg.orig); }

    template <class U> auto apply_(U const &arg) const { static_cast<void>(arg); }

    template <class U> auto get_value_(TranslateArgument<U> &&arg) const {
        return std::move(arg.transformed).value_or(arg.orig);
    }

    template <class U> auto get_value_(U const &arg) const { return arg; }

    template <class U> auto has_value_(U const &arg) const -> bool {
        static_cast<void>(arg);
        return false;
    }

    template <class U> auto has_value_(TranslateArgument<U> const &arg) const -> bool {
        return arg.transformed.has_value();
    }
};

} // namespace Gringo::Input
