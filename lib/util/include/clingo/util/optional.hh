#pragma once

#include <clingo/util/algorithm.hh>
#include <clingo/util/immutable_array.hh>

#include <functional>
#include <optional>
#include <vector>

namespace CppClingo::Util {

namespace Detail {

template <class T, class F> using transform_result = std::optional<std::remove_cv_t<std::invoke_result_t<F, T>>>;

template <class T, class F>
using and_then_result = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F, T>>>;

template <class T, class F>
using transform_vec_result = std::optional<std::vector<typename transform_result<T, F>::value_type>>;

} // namespace Detail

//! @addtogroup util_optional
//! @{

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
// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
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
    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    ResultState(ResultState<F, S> &&res) : state{std::move(res.state)}, value{std::move(res.value)} {}

    //! A truth value or state.
    S state = S{};
    //! An optional rewritten expression.
    std::optional<E> value = std::nullopt;
};

//! Helper to update a vector of elements.
//!
//! Flag UseSpan can be set to false when using an immutable_array.
//! This uses the efficient copying of the array in case there was no change.
template <class T, bool UseSpan = true> class ResultVec {
  public:
    //! The value type.
    using ValueType = T;
    //! A span of values.
    using Span = std::span<ValueType const>;
    //! An array of values.
    using Array = Util::immutable_array<ValueType>;
    //! A vector of values.
    using Vector = std::vector<ValueType>;
    //! The (reference to the) source values.
    using Source = std::conditional_t<UseSpan, Span, Array const &>;
    //! A constant iterator to the source values.
    using Iterator = std::conditional_t<UseSpan, typename Span::iterator, typename Array::const_iterator>;
    //! The result values.
    using Result = std::conditional_t<UseSpan, Vector, Array>;

    //! Construct a result vec to track changes to the given source.
    ResultVec(Source source) : source_{source}, current_{source_.begin()} {}

    //! Get current element.
    [[nodiscard]] auto current() const -> ValueType const & { return *current_; }

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
            result_ = Util::copy_n(source_, static_cast<size_t>(std::distance(source_.begin(), current_)));
        }
        ++current_;
    }
    //! Replace the current element.
    template <class... Args> void replace(Args &&...args) {
        if (!result_) {
            result_ = Util::copy_n(source_, static_cast<size_t>(std::distance(source_.begin(), current_)));
        }
        result_->emplace_back(std::forward<Args>(args)...);
        ++current_;
    }
    //! Update the current alement given the optional value.
    void update(std::optional<ValueType> value) {
        if (!value.has_value()) {
            keep();
        } else {
            replace(*std::move(value));
        }
    }
    //! Append fresh elements.
    template <class... Args> void append(Args &&...args) {
        if (!result_) {
            result_ = Util::copy_n(source_, static_cast<size_t>(std::distance(source_.begin(), current_)));
        }
        result_->emplace_back(std::forward<Args>(args)...);
    }
    //! Append fresh elements.
    template <class It> void extend(It begin, It end) {
        if (!result_) {
            result_ = Util::copy_n(source_, static_cast<size_t>(std::distance(source_.begin(), current_)));
        }
        result_->insert(result_->end(), begin, end);
    }
    //! Get a const reference to the current vector.
    //!
    //! This returns a reference to the old vector if it does not have a new one.
    [[nodiscard]] auto value() const & -> Span {
        if (result_) {
            return *result_;
        }
        return source_;
    }
    //! Move out the new vector or return a copy of the old one.
    [[nodiscard]] auto value() && -> Result {
        if (result_) {
            return *std::move(result_);
        }
        if constexpr (UseSpan) {
            return {source_.begin(), source_.end()};
        } else {
            return source_;
        }
    }
    //! Check if the old vector has been updated.
    [[nodiscard]] auto has_value() const -> bool { return result_.has_value(); }
    //! Return a reference to the updated vector if there was a change.
    [[nodiscard]] auto as_optional() & -> std::optional<Vector> & { return result_; }
    //! Move out the updated vector.
    [[nodiscard]] auto as_optional() && -> std::optional<Vector> { return std::move(result_); }

    //! Get a const reference to the current vector.
    //!
    //! This returns a reference to the old vector if it does not have a new one.
    [[nodiscard]] auto operator*() const & -> Span { return value(); }
    //! Move out the new vector or return a copy of the old one.
    [[nodiscard]] auto operator*() && -> Result { return std::move(*this).value(); }
    //! Check if the old vector has been updated.
    explicit operator bool() const { return has_value(); }
    //! Check if all elements have been processed.
    [[nodiscard]] auto complete() const { return current_ == source_.end(); }

  private:
    Source source_;
    std::optional<Vector> result_;
    Iterator current_;
};

//! Deduction guide to construct from a std::vector.
template <class T> ResultVec(std::vector<T> const &) -> ResultVec<T, true>;

//! Deduction guide to construct from a std::span.
template <class T> ResultVec(std::span<T const> const &) -> ResultVec<T, true>;

//! Deduction guide to construct from an immutable array.
template <class T> ResultVec(immutable_array<T> const &) -> ResultVec<T, false>;

//! @}

} // namespace CppClingo::Util
