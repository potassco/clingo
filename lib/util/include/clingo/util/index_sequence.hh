#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

namespace CppClingo::Util {

//! @addtogroup util_container
//! @{

//! Container to store integer sequences.
//!
//! Consecutive integers are stored in an interval.
template <class T> class index_sequence {
  public:
    //! Const iterator for index sequence.
    class iterator {
      public:
        friend class index_sequence;
        //! The iterator category.
        using iterator_category = std::forward_iterator_tag;
        //! The value type.
        using value_type = T;
        //! The difference type.
        using difference_type = std::ptrdiff_t;
        //! The reference type.
        using reference = T;
        //! The pointer type.
        using pointer = void;

        //! Construct an end iterator.
        iterator() = default;

        //! Get the current value.
        auto operator*() const -> reference { return it_->second + static_cast<T>(idx_ - start_); }

        //! Increment the iterator.
        auto operator++() -> iterator & {
            ++idx_;
            if (idx_ == it_->first) {
                start_ = it_->first;
                ++it_;
            }
            return *this;
        }

        //! Post increment the iterator.
        auto operator++(int) -> iterator {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        //! Compare two iterators.
        auto operator==(const iterator &other) const -> bool { return idx_ == other.idx_; }

        //! Compare two iterators.
        auto operator!=(const iterator &other) const -> bool { return !(*this == other); }

      private:
        using interval_iterator = std::vector<std::pair<size_t, T>>::const_iterator;

        iterator(interval_iterator it, size_t idx, size_t start) : it_{it}, idx_{idx}, start_{start} {}

        interval_iterator it_;
        size_t idx_ = 0;
        size_t start_ = 0;
    };

    //! Add an integer to the sequence.
    void add(T value) {
        if (values_.empty()) {
            values_.emplace_back(1, value);
        } else {
            auto &[r, s] = values_.back();
            auto l = values_.size() > 1 ? values_[values_.size() - 2].first : size_t{0};
            if (s + static_cast<T>(r - l) == value) {
                ++r;
            } else {
                values_.emplace_back(size() + 1, value);
            }
        }
    }

    //! Set the sequence to the interval `[s, e)`.
    void assign(T s, T e) {
        last_ = 0;
        values_.clear();
        if (s < e) {
            values_.emplace_back(static_cast<size_t>(e - s), s);
        }
    }

    //! Get the i-th integer in the sequence.
    [[nodiscard]] auto operator[](size_t index) const -> T {
        assert(index < size() && last_ < values_.size());
        auto l = last_ > 0 ? values_[last_ - 1].first : size_t{0};
        auto r = values_[last_].first;
        assert(l < r);
        auto ib = index < l ? values_.begin() : values_.begin() + last_;
        auto ie = index < r ? values_.begin() + last_ + 1 : values_.end();
        auto it = std::upper_bound(ib, ie, index, [](auto const &a, auto const &b) { return a < b.first; });
        last_ = static_cast<size_t>(std::distance(values_.begin(), it));
        auto start = last_ > 0 ? values_[last_ - 1].first : size_t{0};
        return it->second + static_cast<T>(index - start);
    }
    //! Get the index of the value in the sequence.
    //!
    //! Returns size() if the value is not found.
    [[nodiscard]] auto find(T value) const -> size_t {
        // NOTE: has linear complexity in the worst case:
        // - locality could be improved
        // - reverse mapping could be stored
        size_t l = 0;
        for (auto const &[r, s] : values_) {
            auto e = s + static_cast<T>(r - l);
            if (s <= value && value < e) {
                return l + static_cast<size_t>(value - s);
            }
            l = r;
        }
        return size();
    }
    //! Get the number of indices in the sequence.
    [[nodiscard]] auto size() const -> size_t { return empty() ? 0 : values_.back().first; }
    //! Check whether the sequence is empty.
    [[nodiscard]] auto empty() const -> bool { return values_.empty(); }
    //! Get an iterator to the beginning of the sequence.
    [[nodiscard]] auto begin() const -> iterator { return {values_.cbegin(), 0, 0}; }
    //! Get an iterator to the end of the sequence.
    [[nodiscard]] auto end() const -> iterator { return {values_.cend(), size(), size()}; }

  private:
    std::vector<std::pair<size_t, T>> values_;
    size_t mutable last_ = 0;
};

//! @}

} // namespace CppClingo::Util
