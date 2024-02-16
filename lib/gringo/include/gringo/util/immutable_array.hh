#pragma once

#include <algorithm>
#include <span>
#include <stdexcept>
#include <vector>

#include <gringo/util/immutable_value.hh>

namespace Gringo::Util {

template <class It, class Jt, class Cmp>
constexpr auto lexicographical_compare_three_way(It it, It ie, Jt jt, Jt je, Cmp comp) -> decltype(comp(*it, *jt)) {
    using ret_t = decltype(comp(*it, *jt));
    static_assert(std::disjunction_v<std::is_same<ret_t, std::strong_ordering>, std::is_same<ret_t, std::weak_ordering>,
                                     std::is_same<ret_t, std::partial_ordering>>,
                  "The return type must be a comparison category type.");

    bool end_i = (it == ie);
    bool end_j = (jt == je);
    for (; !end_i && !end_j; end_i = (++it == ie), end_j = (++jt == je)) {
        if (auto c = comp(*it, *jt); c != 0) {
            return c;
        }
    }

    return !end_i ? std::strong_ordering::greater : !end_j ? std::strong_ordering::less : std::strong_ordering::equal;
}

//! @addtogroup core_immutable
//! @{

//! An immutable array with efficient copying.
//!
//! Note that this can be implemented more efficiently avoiding the double indirection.
template <typename T> class immutable_array {
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

    constexpr immutable_array() noexcept = default;

    immutable_array(immutable_array const &other) = default;

    immutable_array(immutable_array &&other) noexcept = default;

    auto operator=(immutable_array const &other) noexcept -> immutable_array & = default;

    auto operator=(immutable_array &&other) noexcept -> immutable_array & = default;

    operator std::vector<T> const &() const noexcept { return vector_(); }

    immutable_array(std::span<T> span) : immutable_array{span.begin(), span.end()} {}

    immutable_array(std::span<T const> span) : immutable_array{span.begin(), span.end()} {}

    immutable_array(std::vector<T> &&vec) {
        if (!vec.empty()) {
            vec.shrink_to_fit();
            vec_ = Util::make_immutable<vector_type>(std::move(vec));
        }
    }

    immutable_array(std::vector<T> const &vec) : immutable_array{vec.begin(), vec.end()} {}

    template <class It> immutable_array(It first, It last) {
        if (first != last) {
            vec_ = Util::make_immutable<vector_type>(first, last);
        }
    }

    immutable_array(std::initializer_list<T> init) {
        if (init.size() > 0) {
            vec_ = Util::make_immutable<vector_type>(std::move(init));
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

    [[nodiscard]] auto data() const -> T const * { return vector_().data(); }

    [[nodiscard]] auto begin() const noexcept -> const_iterator { return vector_().begin(); }

    [[nodiscard]] auto end() const noexcept -> const_iterator { return vector_().end(); }

    [[nodiscard]] auto empty() const noexcept -> bool { return vector_().empty(); }

    [[nodiscard]] auto size() const noexcept -> size_type { return vector_().size(); }

    void swap(immutable_array &other) noexcept { swap(other.vec_, vec_); }

    friend auto operator==(immutable_array const &lhs, immutable_array const &rhs) -> bool {
        return lhs.vector_() == rhs.vector_();
    }

    friend auto operator<=>(immutable_array const &lhs, immutable_array const &rhs) {
        return lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                                                 std::compare_three_way{});
    }

  private:
    [[nodiscard]] auto vector_() const -> std::vector<T> const & { return vec_ ? *vec_ : empty_(); }

    static auto empty_() -> std::vector<T> const & {
        static std::vector<T> res;
        return res;
    }
    Util::immutable_value<vector_type> vec_;
};

template <class It> immutable_array(It, It) -> immutable_array<typename std::iterator_traits<It>::value_type>;

template <class T> void swap(immutable_array<T> &lhs, immutable_array<T> &rhs) noexcept { lhs.swap(rhs); }

template <class T, class... Ts> auto make_immutable_array(Ts &&...args) -> immutable_array<T> {
    std::vector<T> res;
    res.reserve(sizeof...(Ts));
    (res.emplace_back(std::forward<Ts>(args)), ...);
    return immutable_array(std::move(res));
}

//! @}

} // namespace Gringo::Util
