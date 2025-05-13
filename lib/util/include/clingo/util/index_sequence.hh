#pragma once

#include <algorithm>
#include <tuple>
#include <vector>

namespace CppClingo::Util {

//! @addtogroup util_container
//! @{

//! Container to store integer sequences.
//!
//! Consecutive integers are stored in an interval.
template <class T> class index_sequence {
  public:
    class iterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = T;
        using pointer = void;

        iterator() = default;

        auto operator*() const -> reference { return std::get<2>(*it_) + static_cast<T>(idx_); }

        auto operator++() -> iterator & {
            ++idx_;
            if (idx_ == std::get<1>(*it_)) {
                ++it_;
            }
            return *this;
        }

        auto operator++(int) -> iterator {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        auto operator==(const iterator &other) const -> bool { return idx_ == other.idx_; }

        auto operator!=(const iterator &other) const -> bool { return !(*this == other); }

      private:
        using interval_iterator = std::vector<std::tuple<size_t, size_t, T>>::const_iterator;

        iterator(interval_iterator it, size_t idx) : it_{it}, idx_{idx} {}

        interval_iterator it_;
        size_t idx_ = 0;
    };

    //! Add an integer to the sequence.
    void add(T value) {
        if (values_.empty()) {
            values_.emplace_back(0, 1, value);
        } else {
            auto &[l, r, y] = values_.back();
            if (static_cast<T>(r) + y == value) {
                ++r;
            } else {
                values_.emplace_back(size(), size() + 1, value - static_cast<T>(size()));
            }
        }
    }

    //! Set the sequence to the interval `[l,r-1]`.
    void assign(T l, T r) {
        values_.clear();
        if (l < r) {
            values_.emplace_back(0, static_cast<size_t>(r - l), l);
        }
    }

    //! Get the i-th integer in the sequence.
    [[nodiscard]] auto operator[](size_t index) const -> T {
        assert(index < size() && last_ < values_.size());
        auto const &[l, r, y] = values_[last_];
        assert(l < r);
        auto ib = index < l ? values_.begin() : values_.begin() + last_;
        auto ie = index < r ? values_.begin() + last_ + 1 : values_.end();
        auto it = std::upper_bound(ib, ie, index, [](auto const &a, auto const &b) { return a < std::get<1>(b); });
        last_ = static_cast<size_t>(std::distance(values_.begin(), it));
        return static_cast<T>(index) + std::get<2>(*it);
    }
    //! Check if the sequence contains an element.
    [[nodiscard]] auto find(T value) const -> size_t {
        // TODO: has linear complexity in the worst case:
        // - locality could be improved
        // - reverse mapping could be stored
        for (auto const &[l, r, y] : values_) {
            auto i = static_cast<size_t>(value - y);
            if (y <= value && l <= i && i < r) {
                return i;
            }
        }
        return size();
    }
    //! Get the number of indices in the sequence.
    [[nodiscard]] auto size() const -> size_t { return empty() ? 0 : std::get<1>(values_.back()); }
    //! Check whether the sequence is empty.
    [[nodiscard]] auto empty() const -> bool { return values_.empty(); }
    //! Get an iterator to the beginning of the sequence.
    [[nodiscard]] auto begin() const -> iterator { return {values_.cbegin(), 0}; }
    //! Get an iterator to the end of the sequence.
    [[nodiscard]] auto end() const -> iterator { return {values_.cend(), size()}; }

  private:
    std::vector<std::tuple<size_t, size_t, T>> values_;
    size_t mutable last_ = 0;
};

//! @}

} // namespace CppClingo::Util
