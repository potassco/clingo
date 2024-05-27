#pragma once

#include <concepts>
#include <iterator>

namespace Gringo::Util {

template <typename T>
concept iterable = requires(T x) {
    x.begin();
    x.end();
};
template <class T>
    requires std::forward_iterator<T> || std::integral<T>
class enumerate;

template <std::forward_iterator T> class enumerate<T> {
  public:
    class iterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<size_t, typename T::reference>;
        using difference_type = std::make_signed_t<size_t>;

        iterator() = default;

        iterator(size_t num, T cur) : num_{num}, cur_{cur} {}

        auto operator*() const -> value_type { return {num_, *cur_}; }

        auto operator++() -> iterator & {
            ++cur_;
            ++num_;
            return *this;
        }

        auto operator++(int) -> iterator {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend auto operator==(iterator const &a, iterator const &b) -> bool { return a.cur_ == b.cur_; }

      private:
        size_t num_ = 0;
        T cur_;
    };
    static_assert(std::forward_iterator<iterator>);

    template <iterable U>
        requires std::is_lvalue_reference_v<U>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    enumerate(U &&x) : enumerate{x.begin(), x.end()} {}
    enumerate(T begin, T end) : begin_{begin}, end_{end} {}

    [[nodiscard]] auto begin() const -> iterator { return {0, begin_}; }
    [[nodiscard]] auto end() const -> iterator { return {0, end_}; }

  private:
    T begin_;
    T end_;
};

template <iterable U>
    requires std::is_lvalue_reference_v<U>
enumerate(U &&x) -> enumerate<decltype(x.begin())>;

template <std::integral T> class enumerate<T> {
  public:
    enumerate(T begin, T end) : begin_{begin}, end_{end} {}
    enumerate(T end) : begin_{}, end_{end} {}

    class iterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::make_signed_t<value_type>;

        iterator() = default;

        iterator(value_type cur) : cur_{cur} {}

        auto operator*() const -> value_type { return cur_; }

        auto operator++() -> iterator & {
            ++cur_;
            return *this;
        }

        auto operator++(int) -> iterator {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend auto operator==(iterator const &a, iterator const &b) -> bool = default;

      private:
        value_type cur_ = 0;
    };
    static_assert(std::forward_iterator<iterator>);

    [[nodiscard]] auto begin() const -> iterator { return {begin_}; }
    [[nodiscard]] auto end() const -> iterator { return {end_}; }

  private:
    T begin_;
    T end_;
};

template <std::integral T> enumerate(T end) -> enumerate<T>;

template <std::integral T> enumerate(T begin, T end) -> enumerate<T>;

} // namespace Gringo::Util
