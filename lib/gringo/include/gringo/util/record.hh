#include <cstdint>
#include <optional>
#include <utility>

namespace Gringo::Util::Record {

constexpr auto max_attributes = 64U;

template <auto id, class V> struct AttributeValue {
    static constexpr auto Id = id;
    using Value = V;

    AttributeValue(V attr) : attr(std::forward<V>(attr)) {}
    V attr;
};

template <auto id> struct AttributeName {
    static_assert(id < max_attributes, "only 32 attributes");
    static constexpr auto Id = static_cast<uint64_t>(1) << id;

    template <class V> constexpr auto operator=(V &&attr) const { return AttributeValue<Id, V>{std::forward<V>(attr)}; }
};

template <bool Opt, class A, typename M> constexpr auto select(A const &x, M const &mem) -> decltype(auto) {
    static_cast<void>(x);
    return mem;
}

template <bool Opt, class A, typename M, typename T, typename... Ts>
constexpr auto select(A const &x, M const &mem, T &arg, Ts &...args) -> decltype(auto) {
    if constexpr (T::Id == A::Id) {
        if constexpr (Opt) {
            return arg.attr ? M{*std::forward<typename T::Value>(arg.attr)} : M{mem};
        } else {
            return std::forward<typename T::Value>(arg.attr);
        }
    } else {
        return select<Opt>(x, mem, args...);
    }
}

template <class T, class... Args> auto rewrite(T const &x, Args &&...args) -> std::optional<T> {
    if ((true && ... && !args.attr.has_value())) {
        return std::nullopt;
    }
    return x.template update<true>(std::forward<Args>(args)...);
}

template <class... Args> struct Types {
    constexpr Types(Args const &...args) { (static_cast<void>(args), ...); }
};

template <class... Attr, class... Args> constexpr void check(Types<Attr...> attr, Types<Args...> args) {
    static_cast<void>(attr);
    static_cast<void>(args);
    constexpr auto sum_all = (0 | ... | Attr::Id);
    constexpr auto sum_plus = (0 + ... + Args::Id);
    constexpr auto sum_or = (0 | ... | Args::Id);
    static_assert(sum_plus == sum_or, "duplicate arguments");
    static_assert((sum_or & ~sum_all) == 0, "unsupported arguments");
}

} // namespace Gringo::Util::Record
