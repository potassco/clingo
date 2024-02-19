#include <optional>
#include <tuple>
#include <utility>

namespace Gringo::Util::Record {

template <auto T, class V> struct AttributeValue {
    static constexpr auto tag = T;
    using Val = V;
    constexpr AttributeValue(Val val) : val(std::forward<Val>(val)) {}
    Val val;
};

template <auto T> struct AttributeName {
    static constexpr auto tag = T;
    template <class Val> constexpr auto operator=(Val &&val) const {
        return AttributeValue<tag, Val>{std::forward<Val>(val)};
    }
};

template <auto i, auto needle, auto tag, auto... tags> constexpr auto get_index_() {
    if constexpr (needle == tag) {
        return i;
    } else {
        return get_index_<i + 1, needle, tags...>();
    }
}

template <auto tag, class... Attrs> constexpr auto get_index(std::tuple<Attrs...> names) {
    static_cast<void>(names);
    return get_index_<0, tag, Attrs::tag...>();
}

template <class Arg, class... Attrs> constexpr auto is_valid_argument(std::tuple<Attrs...> attrs) -> bool {
    static_cast<void>(attrs);
    return ((Arg::tag == Attrs::tag) || ...);
}

template <class Arg, class... Args> constexpr auto is_unique_argument() { return ((Arg::tag != Args::tag) && ...); }

template <class Arg, class... Args> constexpr auto check_unique_arguments() {
    if constexpr (sizeof...(Args) > 0) {
        return is_unique_argument<Arg, Args...>() && check_unique_arguments<Args...>();
    }
    return true;
}

template <class Rec, class... Args>
concept ValidArguments = (is_valid_argument<Args>(Rec::attributes()) && ...) && check_unique_arguments<Args...>();

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

template <bool Opt = false, typename Rec, typename... Args> auto update_record(Rec const &x, Args &&...args) -> Rec {
    return [&]<class... Attrs>(std::tuple<Attrs...> attrs) {
        static_cast<void>(attrs);
        return Rec{select_value<Attrs::tag, Opt>(x, args...)...};
    }(Rec::attributes());
}

template <class Rec, class... Args> auto rewrite_record(Rec const &x, Args &&...args) -> std::optional<Rec> {
    if ((!args.val.has_value() && ...)) {
        return std::nullopt;
    }
    return update_record<true>(x, std::forward<Args>(args)...);
}

template <class Rec> class Base {
  public:
    template <auto tag> [[nodiscard]] auto get_value() const -> decltype(auto) {
        static constexpr auto mem = std::get<get_index<tag>(Rec::attributes())>(Rec::attributes()).val;
        if constexpr (std::is_member_function_pointer_v<decltype(mem)>) {
            return (static_cast<Rec const *>(this)->*mem)();
        } else {
            return static_cast<Rec const *>(this)->*mem;
        }
    }
    template <class... Args>
        requires ValidArguments<Rec, Args...>
    [[nodiscard]] auto update(Args &&...args) const {
        return update_record(*static_cast<Rec const *>(this), std::forward<Args>(args)...);
    }
    template <class... Args>
        requires ValidArguments<Rec, Args...>
    [[nodiscard]] auto rewrite(Args &&...args) const {
        return rewrite_record(*static_cast<Rec const *>(this), std::forward<Args>(args)...);
    }

    //! Compare two literals.
    friend auto operator==(Base const &a, Base const &b) -> bool = default;
    //! Compare two literals.
    friend auto operator<=>(Base const &a, Base const &b) -> std::strong_ordering = default;
};

} // namespace Gringo::Util::Record
