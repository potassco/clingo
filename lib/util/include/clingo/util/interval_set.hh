#pragma once

#include <algorithm>
#include <clingo/util/small_vector.hh>

namespace CppClingo::Util {

//! @addtogroup util_container
//! @{

//! A set of intervals.
//!
//! Closed and open intervals of bounds are supported.
//!
//! Note that intervals involving discrete values should be closed in a
//! post-processing step to obtain a canonical set representation, e.g., the
//! inteval `(1,2)` should be removed and `(1,3)` be represented as `[2]`.
template <class T> class interval_set {
  public:
    //! The value type used as bounds.
    using value_type = T;
    struct right_bound;

    //! A left bound of an interval.
    struct left_bound {
        //! Construct the bound.
        constexpr left_bound(value_type bound, bool inclusive) : bound{std::move(bound)}, inclusive{inclusive} {}
        //! Construct the bound from a right bound.
        explicit left_bound(right_bound const &x) : bound(x.bound), inclusive(!x.inclusive) {}

        //! The value of the bound.
        value_type bound;
        //! Whether the bound is open (!inclusive) or closed (inclusive).
        bool inclusive;
    };

    //! A right bound of an interval.
    struct right_bound {
        //! Construct the bound.
        constexpr right_bound(value_type bound, bool inclusive) : bound{std::move(bound)}, inclusive{inclusive} {}
        //! Construct the bound from a left bound.
        explicit right_bound(left_bound const &x) : bound(x.bound), inclusive(!x.inclusive) {}

        //! The value of the bound.
        value_type bound;
        //! Whether the bound is open (!inclusive) or closed (inclusive).
        bool inclusive;
    };

    //! An interval determined by a left and a right bound.
    struct interval {
        //! Construct the bound.
        constexpr interval(left_bound left, right_bound right) : left{std::move(left)}, right{std::move(right)} {}
        //! Whether the interval contains the given value.
        auto contains(value_type const &x) const -> bool { return !(x < *this) && !(*this < x); }
        //! Whether the interval is empty.
        [[nodiscard]] auto empty() const -> bool { return !(left < right); }

        //! The left bound of the interval.
        left_bound left;
        //! The right bound of the interval.
        right_bound right;
    };
    //! The vector used to store intervals.
    using interval_vector = small_vector<interval>;
    //! An iterator over the intervals in the set.
    using iterator = typename interval_vector::const_iterator;

    //! Left bound x is smaller than left bound y.
    friend auto operator<(left_bound const &x, left_bound const &y) -> bool {
        return x.bound < y.bound || (x.bound == y.bound && x.inclusive && !y.inclusive);
    }
    //! Left bound x is smaller than or equal to left bound y.
    friend auto operator<=(left_bound const &x, left_bound const &y) -> bool {
        return x.bound < y.bound || (x.bound == y.bound && (x.inclusive || !y.inclusive));
    }
    //! There is a gap between x and y.
    //!
    //! For example, two intervals with bounds x and y overlap.
    //! ```
    //! x:    [---
    //! y: ---]
    //! ```
    //! Can be used to check whether an interval is empty.
    friend auto operator<(left_bound const &x, right_bound const &y) -> bool {
        return x.bound < y.bound || (x.bound == y.bound && x.inclusive && y.inclusive);
    }

    //! Right bound x is smaller than right bound y.
    friend auto operator<(right_bound const &x, right_bound const &y) -> bool {
        return x.bound < y.bound || (y.bound == x.bound && !x.inclusive && y.inclusive);
    }
    //! Right bound x is smaller than or equal to right bound y.
    friend auto operator<=(right_bound const &x, right_bound const &y) -> bool {
        return x.bound < y.bound || (y.bound == x.bound && (!x.inclusive || y.inclusive));
    }
    //! There is a gap between x and y.
    //!
    //! For example, two intervals with bounds x and y do not overlap:
    //! ```
    //! x: ---)
    //! y:    (---
    //! ```
    //! Can be used to check whether two intervals can be merged.
    friend auto operator<(right_bound const &x, left_bound const &y) -> bool {
        return x.bound < y.bound || (y.bound == x.bound && !x.inclusive && !y.inclusive);
    }

    //! Interval x is before interval y.
    friend auto operator<(interval const &x, interval const &y) -> bool { return x.right < y.left; }
    //! Value x is before interval y.
    friend auto operator<(value_type const &x, interval const &y) -> bool {
        return x < y.left.bound || (y.left.bound == x && !y.left.inclusive);
    }
    //! Interval x is before value y.
    friend auto operator<(interval const &x, value_type const &y) -> bool {
        return x.right.bound < y || (y == x.right.bound && !x.right.inclusive);
    }

    //! Construct an empty interval set.
    interval_set() = default;
    //! Destroy the interval set.
    ~interval_set() = default;

    //! The default copy constructor.
    interval_set(interval_set const &other) = default;
    //! The default copy assignment.
    auto operator=(interval_set const &other) -> interval_set & = default;

    //! The default move constructor.
    interval_set(interval_set &&other) noexcept = default;
    //! The default copy assignment.
    auto operator=(interval_set &&other) noexcept -> interval_set & = default;

    //! Construct an interval set containing the given intervals.
    interval_set(std::initializer_list<interval> list) {
        for (auto &x : list) {
            add(x);
        }
    }

    //! Reserve space for at least n elements.
    //!
    //! Calls the reserve method of the underlying vector.
    //!
    //! @param n the number of elements to reserve space for
    void reserve(size_t n) { vec_.reserve(n); }

    //! Releases the underlying vector.
    auto release() -> interval_vector { return std::move(vec_); }

    //! Add the given interval to the set.
    //!
    //! @param x the interval to add
    //! @return a reference to the set
    auto add(interval const &x) -> interval_set & {
        if (!x.empty()) {
            auto it = std::lower_bound(vec_.begin(), vec_.end(), x);
            if (it == vec_.end()) {
                vec_.emplace_back(x);
            } else {
                auto jt = std::upper_bound(it, vec_.end(), x);
                if (it == jt) {
                    vec_.emplace(it, x);
                } else {
                    it->left = std::min(x.left, it->left);
                    it->right = std::max(x.right, (jt - 1)->right);
                    vec_.erase(it + 1, jt);
                }
            }
        }
        return *this;
    }

    //! Subtract the given interval from the set.
    //!
    //! @param x the interval to subtract
    //! @return a reference to the set
    auto remove(interval const &x) -> interval_set & {
        if (!x.empty()) {
            auto it = std::lower_bound(vec_.begin(), vec_.end(), x);
            if (it != vec_.end()) {
                auto jt = std::upper_bound(it, vec_.end(), x);
                if (it + 1 == jt) {
                    auto r = interval{left_bound{x.right}, it->right};
                    it->right = right_bound{x.left};
                    if (it->empty()) {
                        if (r.empty()) {
                            vec_.erase(it);
                        } else {
                            *it = r;
                        }
                    } else if (!r.empty()) {
                        vec_.emplace(it + 1, r);
                    }
                } else if (it != jt) {
                    it->right = right_bound{x.left};
                    (jt - 1)->left = left_bound{x.right};
                    vec_.erase(it + !it->empty(), jt - !(jt - 1)->empty());
                }
            }
        }
        return *this;
    }

    //! Check if the set contains the given interval.
    //!
    //! @param x the interval to check
    //! @return whether the set contains the interval
    [[nodiscard]] auto contains(interval const &x) const -> bool {
        if (x.empty()) {
            return true;
        }
        for (auto &y : vec_) {
            if (x.right <= y.right) {
                return y.left <= x.left;
            }
        }
        return false;
    }

    //! Check if the set contains the given value.
    //!
    //! @param x the value to check
    //! @return whether the set contains the value
    [[nodiscard]] auto contains(T const &x) const -> bool {
        for (auto &y : vec_) {
            if (!(y < x)) {
                return !(x < y);
            }
        }
        return false;
    }

    //! Check if the set intersects the given interval.
    //!
    //! @param x the interval to check
    //! @return whether the set intersects the interval
    [[nodiscard]] auto intersects(interval const &x) const -> bool {
        if (!x.empty()) {
            for (auto &y : vec_) {
                if (x.left < y.right) {
                    return y.left < x.right;
                }
            }
        }
        return false;
    }

    //! Check if the set is empty.
    [[nodiscard]] auto empty() const -> bool { return vec_.empty(); }

    //! Get the size of the set.
    [[nodiscard]] auto size() const -> size_t { return vec_.size(); }

    //! Get the first interval in the set.
    [[nodiscard]] auto front() const -> interval const & { return vec_.front(); }

    //! Get the last interval in the set.
    [[nodiscard]] auto back() const -> interval const & { return vec_.back(); }

    //! Get an iterator to the beginning of the set.
    [[nodiscard]] auto begin() const -> iterator { return vec_.begin(); }

    //! Get an iterator to the end of the set.
    [[nodiscard]] auto end() const -> iterator { return vec_.end(); }

    //! Clear the set.
    void clear() { return vec_.clear(); }

    //! Compute the intersection of two interval sets.
    [[nodiscard]] auto intersect(interval_set const &set) const -> interval_set {
        auto it = vec_.begin();
        interval_set intersection;
        for (auto &x : set.vec_) {
            for (; it != vec_.end() && it->right < x.left; ++it) {
            }
            for (; it != vec_.end() && it->right <= x.right; ++it) {
                intersection.vec_.emplace_back(interval{std::max(it->left, x.left), it->right});
            }
            if (it != vec_.end() && it->left < x.right) {
                intersection.vec_.emplace_back(interval{std::max(it->left, x.left), x.right});
            }
        }
        return intersection;
    }

    //! Compute the difference between two interval sets.
    auto difference(interval_set const &set) const -> interval_set {
        auto it = set.vec_.begin();
        interval_set difference;
        for (auto &x : vec_) {
            interval current = x;
            for (; it != set.vec_.end() && it->right < current.left; ++it) {
            }
            for (; it != set.vec_.end() && it->right <= current.right; ++it) {
                if (current.left < it->left) {
                    difference.vec_.emplace_back(current);
                    difference.vec_.back().right = right_bound{it->left};
                }
                current.left = left_bound{it->right};
            }
            if (it != set.vec_.end() && it->left < current.right) {
                current.right = right_bound{it->left};
            }
            if (current.left < current.right) {
                difference.vec_.emplace_back(current);
            }
        }
        return difference;
    }

  private:
    interval_vector vec_;
};

//! @}

} // namespace CppClingo::Util
