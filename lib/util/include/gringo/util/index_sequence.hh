#pragma once

#include <tuple>
#include <vector>

namespace Gringo::Util {

//! Container to store integer sequences.
//!
//! Consecutive integers are stored in an interval.
template <class T> class index_sequence {
  public:
    //! Add an integer to the sequence.
    void add(T x) {
        if (values_.empty()) {
            values_.emplace_back(0, 1, x);
        } else {
            auto &[l, r, y] = values_.back();
            if (r + y == x) {
                ++r;
            } else {
                values_.emplace_back(size(), size() + 1, x - size());
            }
        }
    }
    //! Get the i-th integer in the sequence.
    [[nodiscard]] auto operator[](T i) const -> T {
        assert(i < size());
        auto const &[l, r, y] = values_[last_];
        auto ib = l < i ? values_.begin() + last_ : values_.begin();
        auto ie = i < r ? ib + last_ + 1 : values_.end();
        auto it = std::upper_bound(ib, ie, i, [](auto const &a, auto const &b) { return a < std::get<1>(b); });
        last_ = static_cast<size_t>(std::distance(values_.begin(), it));
        return i + std::get<2>(*it);
    }
    //! Get the number of indices in the sequence.
    [[nodiscard]] auto size() const -> size_t { return std::get<1>(values_.back()); }
    //! Check whether the sequence is empty.
    [[nodiscard]] auto empty() const -> bool { return values_.empty(); }

  private:
    std::vector<std::tuple<T, T, T>> values_;
    size_t mutable last_ = 0;
};

} // namespace Gringo::Util
