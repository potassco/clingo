#pragma once

#include <functional>
#include <optional>
#include <vector>

#include <gringo/util/algorithm.hh>
#include <gringo/util/immutable_array.hh>

namespace Gringo::Util {

namespace Detail {

template <class T, class F> using transform_result = std::optional<std::remove_cv_t<std::invoke_result_t<F, T>>>;

template <class T, class F>
using and_then_result = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, T>>>;

template <class T, class F>
using transform_vec_result = std::optional<std::vector<typename transform_result<T, F>::value_type>>;

} // namespace Detail

//! Helper to update attributes of a record.
//!
//! A shortcut for <tt>opt ? static_cast<O>(*std::move(opt)) : old</tt>.
template <class N, class O> auto upa(O const &old, std::optional<N> opt) -> O {
    if (opt) {
        return *std::move(opt);
    }
    return old;
}

//! Implemenatation of std::optional<T>::transform.
template <class T, class F> constexpr auto transform(std::optional<T> &x, F &&f) -> Detail::transform_result<T &, F> {
    if (x.has_value()) {
        return std::invoke(std::forward<F>(f), *x);
    }
    return std::nullopt;
}

//! Implemenatation of std::optional<T>::transform.
template <class T, class F>
constexpr auto transform(std::optional<T> const &x, F &&f) -> Detail::transform_result<T const &, F> {
    if (x.has_value()) {
        return std::invoke(std::forward<F>(f), *x);
    }
    return std::nullopt;
}

//! Map the value in the optional with the given predicate.
//! Implemenatation of std::optional<T>::transform.
template <class T, class F> constexpr auto transform(std::optional<T> &&x, F &&f) -> Detail::transform_result<T, F> {
    if (x.has_value()) {
        return std::invoke(std::forward<F>(f), *std::move(x));
    }
    return std::nullopt;
}

//! Implemenatation of std::optional<T>::transform.
template <class T, class F>
constexpr auto transform(std::optional<T> const &&x, F &&f) -> Detail::transform_result<T const, F> {
    if (x.has_value()) {
        return std::invoke(std::forward<F>(f), *std::move(x));
    }
    return std::nullopt;
}

//! Implemenatation of std::optional<T>::and_then.
template <class T, class F> constexpr auto and_then(std::optional<T> &x, F &&f) -> Detail::and_then_result<T &, F> {
    if (x) {
        return std::invoke(std::forward<F>(f), *x);
    }
    return std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, T &>>>{};
}

//! Implemenatation of std::optional<T>::and_then.
template <class T, class F>
constexpr auto and_then(std::optional<T> const &x, F &&f) -> Detail::and_then_result<T const &, F> {
    if (x) {
        return std::invoke(std::forward<F>(f), *x);
    }
    return std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, T const &>>>{};
}

//! Implemenatation of std::optional<T>::and_then.
template <class T, class F> constexpr auto and_then(std::optional<T> &&x, F &&f) -> Detail::and_then_result<T, F> {
    if (x) {
        return std::invoke(std::forward<F>(f), *std::move(x));
    }
    return std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, T>>>{};
}

//! Implemenatation of std::optional<T>::and_then.
template <class T, class F>
constexpr auto and_then(std::optional<T> const &&x, F &&f) -> Detail::and_then_result<T const, F> {
    if (x) {
        return std::invoke(std::forward<F>(f), *std::move(x));
    }
    return std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, T const>>>{};
}

//! Map the given predicate over an optional vector.
template <class T, class F>
auto transform_vec(std::optional<std::vector<T>> const &vec, F const &f)
    -> Detail::transform_vec_result<T const &, F const &> {
    return transform(vec, [&f](auto const &vec) {
        typename Detail::transform_vec_result<T const &, F const &>::value_type ret;
        ret.reserve(vec.size());
        for (auto const &elem : vec) {
            ret.emplace_back(std::invoke(f, elem));
        }
        return ret;
    });
}

//! Map the given predicate over an optional vector.
template <class T, class F>
auto transform_vec(std::optional<std::vector<T>> &&vec, F const &f) -> Detail::transform_vec_result<T, F const &> {
    return transform(vec, [&f](auto &vec) {
        typename Detail::transform_vec_result<T, F const &>::value_type ret;
        ret.reserve(vec.size());
        for (auto &elem : vec) {
            ret.emplace_back(std::invoke(f, std::move(elem)));
        }
        return ret;
    });
}

//! Truth values for expressions.
enum class TruthValue {
    top,     //!< Indicate a true expression.
    bot,     //!< Indicate a false expression.
    unknown, //!< Indicate an expression with an unknown truth value.
};

//! The result of a simplification.
//!
//! The result consists of a state resulting from simplification
//! together with an optional value in case the expression has changed.
template <class E, class S = bool> struct ResultState {
    //! Construct from state and value.
    ResultState(S state = S{}, std::optional<E> value = std::nullopt)
        : state{std::move(state)}, value{std::move(value)} {}
    //! Construct from compatible result state.
    template <class F> ResultState(ResultState<F, S> const &res) : state{res.state}, value{res.value} {}
    //! Construct from compatible result state.
    template <class F>
    ResultState(ResultState<F, S> &&res) : state{std::move(res.state)}, value{std::move(res.value)} {}

    //! A truth value or state.
    S state = S{};
    //! An optional rewritten expression.
    std::optional<E> value = std::nullopt;
};

//! Helper to update a vector of elements.
template <class T> class ResultVec {
  public:
    using Span = tcb::span<T const>;
    using Vector = std::vector<T>;
    using Array = Util::immutable_array<T>;

    //! Construct a result vec to track changes to the given source.
    ResultVec(Span source) : source_{source}, current_{source_.begin()} {}
    //! Construct a result vec to track changes to the given source.
    ResultVec(Vector const &source) : source_{source}, current_{source_.begin()} {}
    //! Construct a result vec to track changes to the given source.
    ResultVec(Array const &source) : source_{source}, current_{source_.begin()} {}

    //! Get current element.
    [[nodiscard]] auto current() const -> T const & { return *current_; }

    //! Keep the current element.
    void keep() {
        if (result_) {
            result_->emplace_back(*current_);
        }
        ++current_;
    }
    //! Keep all elements.
    void keep_all() {
        if (result_) {
            result_->insert(result_->end(), current_, source_.end());
        }
        current_ = source_.end();
    }
    //! Remove the current element.
    void remove() {
        if (!result_) {
            result_ = Util::copy_n(source_, std::distance(source_.begin(), current_));
        }
        ++current_;
    }
    //! Replace the current element.
    template <class... Args> void replace(Args &&...args) {
        if (!result_) {
            result_ = Util::copy_n(source_, std::distance(source_.begin(), current_));
        }
        result_->emplace_back(std::forward<Args>(args)...);
        ++current_;
    }
    //! Update the current alement given the optional value.
    void update(std::optional<T> value) {
        if (!value.has_value()) {
            keep();
        } else {
            replace(*std::move(value));
        }
    }
    //! Append fresh elements.
    template <class... Args> void append(Args &&...args) {
        if (!result_) {
            result_ = Util::copy_n(source_, std::distance(source_.begin(), current_));
        }
        result_->emplace_back(std::forward<Args>(args)...);
    }
    //! Append fresh elements.
    template <class It> void extend(It begin, It end) {
        if (!result_) {
            result_ = Util::copy_n(source_, std::distance(source_.begin(), current_));
        }
        result_->insert(result_->end(), begin, end);
    }
    //! Get a const reference to the current vector.
    //!
    //! This returns a reference to the old vector if it does not have a new one.
    [[nodiscard]] auto value() const & -> Span { return result_ ? Span{*result_} : source_; }
    //! Move out the new vector or return a copy of the old one.
    [[nodiscard]] auto value() && -> Vector {
        if (result_) {
            return *std::move(result_);
        }
        return {source_.begin(), source_.end()};
    }
    //! Check if the old vector has been updated.
    [[nodiscard]] auto has_value() const -> bool { return result_.has_value(); }
    //! Return a reference to the updated vector if there was a change.
    [[nodiscard]] auto as_optional() & -> std::optional<std::vector<T>> & { return result_; }
    //! Move out the updated vector.
    [[nodiscard]] auto as_optional() && -> std::optional<std::vector<T>> { return std::move(result_); }

    //! Get a const reference to the current vector.
    //!
    //! This returns a reference to the old vector if it does not have a new one.
    [[nodiscard]] auto operator*() const & -> Span { return value(); }
    //! Move out the new vector or return a copy of the old one.
    [[nodiscard]] auto operator*() && -> std::vector<T> { return std::move(*this).value(); }
    //! Check if the old vector has been updated.
    explicit operator bool() const { return has_value(); }
    //! Check if all elements have been processed.
    [[nodiscard]] auto complete() const { return current_ == source_.end(); }

  private:
    Span source_;
    std::optional<Vector> result_;
    typename Span::iterator current_;
};

template <class O, class N> struct UPA {
    O const &old;
    N opt;
};

namespace Detail {

template <class T> auto upa_has_value_(T const &x) -> bool {
    static_cast<void>(x);
    return false;
}

template <class T> auto upa_has_value_(ResultVec<T> const &x) -> bool {
    static_cast<void>(x);
    return x.has_value();
}

template <class O, class N> auto upa_has_value_(UPA<O, N> const &x) -> bool { return x.opt.has_value(); }

template <class T> auto upa_get_value_(T &&x) -> decltype(auto) { return std::forward<T>(x); }

template <class O, class N> auto upa_get_value_(UPA<O, N> x) { return upa(x.old, std::move(x.opt)); }

template <class T> auto upa_get_value_(ResultVec<T> x) { return std::move(x).value(); }

} // namespace Detail

//! Helper to update attributes of a record.
template <class T, class... Args> auto update(Args &&...args) -> std::optional<T> {
    if ((Detail::upa_has_value_(args) || ...)) {
        return T{Detail::upa_get_value_(std::forward<Args>(args))...};
    }
    return std::nullopt;
}

} // namespace Gringo::Util
