#pragma once

#include <vector>

#include <gringo/util/shared_ptr.hh>

namespace Gringo::Util {

//! @addtogroup core_shared_ptr
//! @{

//! An const vector with efficient copying.
//!
//! Note that this can be implemented more efficiently avoiding the double indirection.
template <typename T> class immutable_vector {
  private:
    using vector_type = std::vector<T>;

  public:
    using value_type = typename vector_type::value_type;
    using const_reference = typename vector_type::const_reference;
    using allocator_type = typename vector_type::allocator_type;
    using size_type = typename vector_type::size_type;
    using difference_type = typename vector_type::difference_type;
    using const_pointer = typename vector_type::const_pointer;
    using const_iterator = typename vector_type::const_iterator;

    constexpr immutable_vector() noexcept = default;

    immutable_vector(immutable_vector const &other) = default;

    immutable_vector(immutable_vector &&other) noexcept = default;

    auto operator=(immutable_vector const &other) noexcept -> immutable_vector & = default;

    auto operator=(immutable_vector &&other) noexcept -> immutable_vector & = default;

    operator std::vector<T> const &() const noexcept { return vector(); }

    immutable_vector(std::vector<T> &&vec) {
        if (!vec.empty()) {
            vec.shrink_to_fit();
            vec_ = Util::construct_shared<vector_type>(std::move(vec));
        }
    }

    immutable_vector(std::vector<T> const &vec) : immutable_vector{std::vector<T>{vec}} {}

    template <class It> immutable_vector(It first, It last) {
        if (first != last) {
            vec_ = Util::construct_shared<vector_type>(first, last);
        }
    }

    immutable_vector(std::initializer_list<T> init) {
        if (init.size() > 0) {
            vec_ = Util::construct_shared<vector_type>(std::move(init));
        }
    }

    [[nodiscard]] auto at(size_type pos) const -> const_reference {
        if (pos >= size()) {
            throw std::out_of_range("");
        }
        return operator[](pos);
    }

    auto operator[](size_type pos) const -> const_reference { return vec_->operator[](pos); }

    [[nodiscard]] auto front() const -> const_reference { return vec_->front(); }

    [[nodiscard]] auto back() const -> const_reference { return vec_->back(); }

    [[nodiscard]] auto vector() const -> std::vector<T> const & { return vec_ ? *vec_ : empty_(); }

    [[nodiscard]] auto data() const -> T const * { return vector().data(); }

    [[nodiscard]] auto begin() const noexcept -> const_iterator { return vector().begin(); }

    [[nodiscard]] auto end() const noexcept -> const_iterator { return vector().end(); }

    [[nodiscard]] auto empty() const noexcept -> bool { return vector().empty(); }

    [[nodiscard]] auto size() const noexcept -> size_type { return vector().size(); }

    void swap(immutable_vector &other) noexcept { swap(other.vec_, vec_); }

    friend auto operator==(immutable_vector const &lhs, immutable_vector const &rhs) -> bool {
        return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    friend auto operator!=(immutable_vector const &lhs, immutable_vector const &rhs) -> bool { return !(lhs == rhs); }

    friend auto operator<(immutable_vector const &lhs, immutable_vector const &rhs) -> bool {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    friend auto operator<=(immutable_vector const &lhs, immutable_vector const &rhs) -> bool { return !(rhs < lhs); }

    friend auto operator>(immutable_vector const &lhs, immutable_vector const &rhs) -> bool { return (rhs < lhs); }

    friend auto operator>=(immutable_vector const &lhs, immutable_vector const &rhs) -> bool { return !(lhs < rhs); }

  private:
    static auto empty_() -> std::vector<T> const & {
        static std::vector<T> res;
        return res;
    }
    Util::shared_ptr<vector_type> vec_;
};

template <class It> immutable_vector(It, It) -> immutable_vector<typename std::iterator_traits<It>::value_type>;

template <class T> void swap(immutable_vector<T> &lhs, immutable_vector<T> &rhs) noexcept { lhs.swap(rhs); }

template <class T, class... Ts> auto make_immutable_vector(Ts &&...args) -> immutable_vector<T> {
    std::vector<T> res;
    res.reserve(sizeof...(Ts));
    (res.emplace_back(std::forward<Ts>(args)), ...);
    return immutable_vector(std::move(res));
}

//! @}

} // namespace Gringo::Util
