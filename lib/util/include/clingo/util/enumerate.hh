#pragma once

#include <concepts>
#include <iterator>

namespace CppClingo::Util {

//! @addtogroup util_algorithm
//! @{

//! Concept ensuring a collection is iterable.
template <typename T>
concept iterable = requires(T x) {
    x.begin();
    x.end();
};

//! A python style enumerate/range object.
template <class T>
    requires std::forward_iterator<T> || std::integral<T>
class enumerate;

//! A python style enumerate object.
template <std::forward_iterator T> class enumerate<T> {
  public:
    //! Iterator yielding a index and a reference.
    class iterator {
      public:
        //! The iterator category.
        using iterator_category = std::forward_iterator_tag;
        //! The index and the reference the iterator is referring to.
        using value_type = std::pair<size_t, typename T::reference>;
        //! The difference type.
        using difference_type = std::make_signed_t<size_t>;

        //! Construct an invalid iterator.
        iterator() = default;

        //! Construct an iterator from an index and a forward iterator.
        iterator(size_t num, T cur) : num_{num}, cur_{cur} {}

        //! Get the reference.
        auto operator*() const -> value_type { return {num_, *cur_}; }

        //! Advance the iterator.
        auto operator++() -> iterator & {
            ++cur_;
            ++num_;
            return *this;
        }

        //! Advance the iterator.
        auto operator++(int) -> iterator {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        //! Compare two iterators.
        friend auto operator==(iterator const &a, iterator const &b) -> bool { return a.cur_ == b.cur_; }

      private:
        size_t num_ = 0;
        T cur_;
    };
    static_assert(std::forward_iterator<iterator>);

    //! Enumerate an iterable collection.
    template <iterable U>
        requires std::is_lvalue_reference_v<U>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    enumerate(U &&x) : enumerate{x.begin(), x.end()} {}
    //! Enumerate an iterator range.
    enumerate(T begin, T end) : begin_{begin}, end_{end} {}

    //! Get the begin iterator.
    [[nodiscard]] auto begin() const -> iterator { return {0, begin_}; }
    //! Get the end iterator (sentinel).
    [[nodiscard]] auto end() const -> iterator { return {0, end_}; }

  private:
    T begin_;
    T end_;
};

//! Deduction guide for iterables.
template <iterable U>
    requires std::is_lvalue_reference_v<U>
enumerate(U &&x) -> enumerate<decltype(x.begin())>;

//! Deduction guide for forward iterators.
template <std::forward_iterator T> enumerate(T begin, T end) -> enumerate<T>;

//! An object similar to pythons range expression.
template <std::integral T> class enumerate<T> {
  public:
    //! Enumerate an integer range.
    enumerate(T begin, T end) : begin_{begin}, end_{end} {}
    //! Enumerate an integer range.
    enumerate(T end) : begin_{}, end_{end} {}

    class iterator {
      public:
        //! The iterator category.
        using iterator_category = std::forward_iterator_tag;
        //! The value type.
        using value_type = T;
        //! The difference type.
        using difference_type = std::make_signed_t<value_type>;

        //! Construct an invalid iterator.
        iterator() = default;

        //! Construct an iterator from an integer.
        iterator(value_type cur) : cur_{cur} {}

        //! Get the current integer.
        auto operator*() const -> value_type { return cur_; }

        //! Advance the iterator.
        auto operator++() -> iterator & {
            ++cur_;
            return *this;
        }

        //! Advance the iterator.
        auto operator++(int) -> iterator {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        //! Compare two iterators.
        friend auto operator==(iterator const &a, iterator const &b) -> bool = default;

      private:
        value_type cur_ = 0;
    };
    static_assert(std::forward_iterator<iterator>);

    //! Get the begin iterator.
    [[nodiscard]] auto begin() const -> iterator { return {begin_}; }
    //! Get the end iterator.
    [[nodiscard]] auto end() const -> iterator { return {end_}; }

  private:
    T begin_;
    T end_;
};

//! Deduction guide for integrals.
template <std::integral T> enumerate(T end) -> enumerate<T>;

//! Deduction guide for integrals.
template <std::integral T> enumerate(T begin, T end) -> enumerate<T>;

//! @}

} // namespace CppClingo::Util
