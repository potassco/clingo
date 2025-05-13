#pragma once

#include <clingo/util/hash.hh>

#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace CppClingo::Util::Record {

//! @addtogroup util_record
//! @{

//! Helper to capture a value bound by an attribute.
template <auto T, auto C, class V> struct AttributeValue {
    //! The tag of the attribute.
    static constexpr auto tag = T;
    //! Whether the attribute is relevant for comparison.
    static constexpr auto compare = C;
    //! The type of the stored value.
    using Val = V;
    //! Construct a bound attribute.
    constexpr AttributeValue(Val val) : val(std::forward<Val>(val)) {}
    //! The value of the attribute.
    Val val;
};

//! A named attribute.
template <auto T, bool C = true> struct AttributeName {
    //! The tag of the attribute.
    static constexpr auto tag = T;
    //! Whether the attribute is relevant for comparison.
    static constexpr auto compare = C;
    //! Helper to bind an attribute to a value.
    // NOLINTNEXTLINE(cppcoreguidelines-c-copy-assignment-signature)
    template <class Val> constexpr auto operator=(Val &&val) const {
        if constexpr (std::is_member_pointer_v<std::decay_t<Val>>) {
            return AttributeValue<tag, compare, Val>{std::forward<Val>(val)};
        } else {
            return AttributeValue<tag, compare, Val &&>{std::forward<Val>(val)};
        }
    }
};

//! Get the index of an attribute tag.
template <auto i, auto needle, auto tag, auto... tags> constexpr auto get_index_() {
    if constexpr (needle == tag) {
        return i;
    } else {
        return get_index_<i + 1, needle, tags...>();
    }
}

//! Get the index of an attribute tag.
template <auto tag, class... Attrs> constexpr auto get_index([[maybe_unused]] std::tuple<Attrs...> names) {
    return get_index_<0, tag, Attrs::tag...>();
}

//! Check if an argument is valid.
template <class Arg, class... Attrs>
constexpr auto is_valid_argument([[maybe_unused]] std::tuple<Attrs...> attrs) -> bool {
    return ((std::remove_cvref_t<Arg>::tag == Attrs::tag) || ...);
}

//! Check if an argument is unique.
template <class Arg, class... Args> constexpr auto is_unique_argument() {
    return ((Arg::tag != Args::tag) && ...);
}

//! Check if all arguments are unique.
template <class Arg, class... Args> constexpr auto check_unique_arguments() {
    if constexpr (sizeof...(Args) > 0) {
        return is_unique_argument<Arg, Args...>() && check_unique_arguments<Args...>();
    }
    return true;
}

//! Concept requiring arguments to be valid.
template <class Rec, class... Args>
concept ValidArguments = (is_valid_argument<Args>(Rec::attributes()) && ...) && check_unique_arguments<Args...>();

//! Select the argument with the given tag among the given arguments.
//!
//! If Opt is true, the arguments are supposed to be wrapped in optionals.
//! If the argument is not found or not engaged, it is taken from the given record.
template <auto tag, bool Opt, class Rec, class Arg, class... Args>
auto select_value(Rec const &rec, Arg &arg, Args &...args) -> decltype(auto) {
    if constexpr (tag == Arg::tag) {
        if constexpr (Opt) {
            using M = std::decay_t<decltype(rec.template get_value<tag>())>;
            return arg.val ? M{*std::forward<typename Arg::Val>(arg.val)} : M{rec.template get_value<tag>()};
        } else {
            return std::forward<typename Arg::Val>(arg.val);
        }
    } else if constexpr (sizeof...(args) == 0) {
        return rec.template get_value<tag>();
    } else {
        return select_value<tag, Opt>(rec, args...);
    }
}

//! Update a record by returning a copy with the given arguments updated.
//!
//! If Opt is true, the arguments are supposed to be wrapped in optionals.
//! If the argument is not engaged, it is taken from the given record.
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
template <bool Opt = false, typename Rec, typename... Args> auto update_record(Rec const &x, Args &&...args) -> Rec {
    return [&]<class... Attrs>([[maybe_unused]] std::tuple<Attrs...> attrs) {
        return Rec{select_value<Attrs::tag, Opt>(x, args...)...};
    }(Rec::attributes());
}

//! Rewrite a record by returning a copy with the given arguments updated.
//!
//! Requires that arguments are passed as optionals (or similar).
//! At least one argument has to be engaged, to produce a result.
template <class Rec, class... Args> auto rewrite_record(Rec const &x, Args &&...args) -> std::optional<Rec> {
    if ((!args.val.has_value() && ...)) {
        return std::nullopt;
    }
    return update_record<true>(x, std::forward<Args>(args)...);
}

//! Get an index sequence containing the tags of the attributes of the record relevant for comparison.
template <class Rec, size_t n, size_t... I> [[nodiscard]] static constexpr auto comparison_sequence() {
    if constexpr (n == std::tuple_size_v<std::remove_cvref_t<decltype(Rec::attributes())>>) {
        return std::index_sequence<I...>{};
    } else {
        auto constexpr attr = std::get<n>(Rec::attributes());
        if constexpr (attr.compare) {
            return comparison_sequence<Rec, n + 1, I..., attr.tag>();
        } else {
            return comparison_sequence<Rec, n + 1, I...>();
        }
    }
}

namespace Comp {

//! Compare spans by value.
template <class T, size_t E> auto operator==(std::span<T, E> const &a, std::span<T, E> const &b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

//! Compare spans by value.
template <class T, size_t E> auto operator<=>(std::span<T, E> const &a, std::span<T, E> const &b) {
    return std::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end());
}

//! Helper to compare record attributes.
template <class Base>
[[nodiscard]] auto compare([[maybe_unused]] Base const &a, [[maybe_unused]] Base const &b) -> std::strong_ordering {
    return std::strong_ordering::equal;
}

//! Helper to compare record attributes.
template <class Base, auto Tag, auto... Tags>
[[nodiscard]] auto compare(Base const &a, Base const &b) -> std::strong_ordering {
    if (auto comp = a.template get_value<Tag>() <=> b.template get_value<Tag>(); comp != 0) {
        return comp;
    }
    return compare<Base, Tags...>(a, b);
}

//! Helper to compare record attributes.
template <class Base, auto... Tags> [[nodiscard]] auto equal(Base const &a, Base const &b) -> bool {
    return ((a.template get_value<Tags>() == b.template get_value<Tags>()) && ...);
}

} // namespace Comp

//! Record base class to enable keyword argument based record updates.
//!
//! Also generates convenient functions and enables meta programming involving the named attributes of the record.
template <class Rec> class Base {
  public:
    //! Get the attribute with the given tag.
    template <auto tag> [[nodiscard]] auto get_value() const -> decltype(auto) {
        static constexpr auto mem = std::get<get_index<tag>(Rec::attributes())>(Rec::attributes()).val;
        if constexpr (std::is_member_function_pointer_v<decltype(mem)>) {
            return (static_cast<Rec const *>(this)->*mem)();
        } else {
            return static_cast<Rec const *>(this)->*mem;
        }
    }
    //! See update_record().
    template <class... Args>
        requires ValidArguments<Rec, Args...>
    [[nodiscard]] auto update(Args &&...args) const {
        return update_record(*static_cast<Rec const *>(this), std::forward<Args>(args)...);
    }
    //! See rewrite_record().
    template <class... Args>
        requires ValidArguments<Rec, Args...>
    [[nodiscard]] auto rewrite(Args &&...args) const {
        return rewrite_record(*static_cast<Rec const *>(this), std::forward<Args>(args)...);
    }

    //! Equality compare to records.
    [[nodiscard]] auto equal(Base const &other) const -> bool {
        return [&]<auto... Tags>(std::index_sequence<Tags...>) {
            return Comp::equal<Base, Tags...>(*this, other);
        }(comparison_sequence<Rec, 0>());
    }
    //! Compare to records.
    [[nodiscard]] auto compare(Base const &other) const -> std::strong_ordering {
        return [&]<auto... Tags>(std::index_sequence<Tags...>) {
            return Comp::compare<Base, Tags...>(*this, other);
        }(comparison_sequence<Rec, 0>());
    }
    //! Compute the hash of the record.
    [[nodiscard]] auto hash() const -> size_t {
        return [&]<auto... Tags>(std::index_sequence<Tags...>) {
            return value_hash_record<Rec>(get_value<Tags>()...);
        }(comparison_sequence<Rec, 0>());
    }
};

//! @}

} // namespace CppClingo::Util::Record
