#pragma once

#include <clingo/util/immutable_value.hh>

#include <algorithm>
#include <span>
#include <stdexcept>
#include <vector>

namespace CppClingo::Util {

//! @addtogroup util_immutable
//! @{

//! An immutable array with efficient copying.
//!
//! Note that this can be implemented more efficiently avoiding the double indirection.
template <typename T> class immutable_array {
  private:
    using vector_type = std::vector<T>;

  public:
    //! The value type.
    using value_type = typename vector_type::value_type;
    //! The reference type.
    using const_reference = typename vector_type::const_reference;
    //! The allocator type.
    using allocator_type = typename vector_type::allocator_type;
    //! The unsigned size type.
    using size_type = typename vector_type::size_type;
    //! The signed size type.
    using difference_type = typename vector_type::difference_type;
    //! The pointer type.
    using const_pointer = typename vector_type::const_pointer;
    //! Theconst iterator type.
    using const_iterator = typename vector_type::const_iterator;

    //! Construct an empty array.
    constexpr immutable_array() noexcept = default;

    //! Copy construct the array.
    immutable_array(immutable_array const &other) noexcept = default;

    //! Move construct the array.
    immutable_array(immutable_array &&other) noexcept = default;

    //! Copy assign the array.
    auto operator=(immutable_array const &other) noexcept -> immutable_array & = default;

    //! Move assign the array.
    auto operator=(immutable_array &&other) noexcept -> immutable_array & = default;

    //! Construct array copying values from the given span.
    immutable_array(std::span<T> span) : immutable_array{span.begin(), span.end()} {}

    //! Construct array copying values from the given span.
    immutable_array(std::span<T const> span) : immutable_array{span.begin(), span.end()} {}

    //! Construct array taking ownership of the given vector.
    //!
    //! Note that this might change in case the data type is implemented differently.
    immutable_array(std::vector<T> &&vec) {
        if (!vec.empty()) {
            vec.shrink_to_fit();
            vec_ = Util::make_immutable<vector_type>(std::move(vec));
        }
    }

    //! Construct array copying from the given vector.
    immutable_array(std::vector<T> const &vec) : immutable_array{vec.begin(), vec.end()} {}

    //! Construct array coping from the given initializer list.
    immutable_array(std::initializer_list<T> init) : immutable_array(init.begin(), init.end()) {}

    //! Construct array coping from the given iterator range.
    template <class It> immutable_array(It first, It last) {
        if (first != last) {
            vec_ = Util::make_immutable<vector_type>(first, last);
        }
    }

    //! Construct array coping from the given iterator range.
    template <class It, class Pred> immutable_array(It first, It last, Pred conv) {
        if (first != last) {
            auto vec = vector_type{};
            vec.reserve(std::distance(first, last));
            for (auto it = first; it != last; ++it) {
                vec.emplace_back(conv(*it));
            }
            vec_ = Util::make_immutable<vector_type>(std::move(vec));
        }
    }

    //! Get the element at the given index.
    //!
    //! Throws in case the index is to large.
    [[nodiscard]] auto at(size_type pos) const -> const_reference {
        if (pos >= size()) {
            throw std::out_of_range("");
        }
        return operator[](pos);
    }

    //! Get the element at the given index.
    auto operator[](size_type pos) const -> const_reference { return vec_->operator[](pos); }

    //! Get a reference to the first element.
    [[nodiscard]] auto front() const -> const_reference { return vec_->front(); }

    //! Get a reference to the last element.
    [[nodiscard]] auto back() const -> const_reference { return vec_->back(); }

    //! Get a pointer to the undelying data.
    [[nodiscard]] auto data() const -> T const * { return vector_().data(); }

    //! Get an iterator pointing to the beginning of the array.
    [[nodiscard]] auto begin() const noexcept -> const_iterator { return vector_().begin(); }

    //! Get an iterator pointing to the end of the array.
    [[nodiscard]] auto end() const noexcept -> const_iterator { return vector_().end(); }

    //! Check if the array is empty.
    [[nodiscard]] auto empty() const noexcept -> bool { return vector_().empty(); }

    //! Get the size of the array.
    [[nodiscard]] auto size() const noexcept -> size_type { return vector_().size(); }

    //! Swap two immutable arrays.
    void swap(immutable_array &other) noexcept { swap(other.vec_, vec_); }

    //! Compare two vectors.
    friend auto operator==(immutable_array const &lhs, immutable_array const &rhs) -> bool {
        return lhs.vector_() == rhs.vector_();
    }

    //! Compare two vectors lexicographically.
    friend auto operator<=>(immutable_array const &lhs, immutable_array const &rhs) {
        return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

  private:
    [[nodiscard]] auto vector_() const -> std::vector<T> const & { return vec_ ? *vec_ : empty_(); }

    static auto empty_() -> std::vector<T> const & {
        static std::vector<T> res;
        return res;
    }
    Util::immutable_value<vector_type> vec_;
};

//! Decduction guide to construct an immutable array from an iterator range.
template <class It> immutable_array(It, It) -> immutable_array<typename std::iterator_traits<It>::value_type>;

//! Swap two immutable arrays.
template <class T> void swap(immutable_array<T> &lhs, immutable_array<T> &rhs) noexcept {
    lhs.swap(rhs);
}

//! Construct an immutable array from the given elements.
//!
//! (Avoids useless copies in initializer lists).
template <class T, class... Ts> auto make_immutable_array(Ts &&...args) -> immutable_array<T> {
    std::vector<T> res;
    res.reserve(sizeof...(Ts));
    (res.emplace_back(std::forward<Ts>(args)), ...);
    return immutable_array(std::move(res));
}

//! @}

} // namespace CppClingo::Util
