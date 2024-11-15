#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <vector>

namespace Clingo::Python {

template <class T, class A = std::allocator<T>> class Iterable {
  public:
    using Vector = std::vector<T, A>;

    Iterable() = default;
    Iterable(Iterable const &other) = default;
    Iterable(Iterable &&other) noexcept = default;
    auto operator=(Iterable const &other) -> Iterable & = default;
    auto operator=(Iterable &&other) -> Iterable & = default;

    Iterable(Vector const &other) : vec_{other} {}
    Iterable(Vector &&other) : vec_{std::move(other)} {}
    auto operator=(Vector const &other) -> Iterable & {
        vec_ = other;
        return *this;
    }
    auto operator=(Vector &&other) noexcept -> Iterable & {
        vec_ = std::move(other);
        return *this;
    }

    [[nodiscard]] auto size() const -> size_t { return vec_.size(); }
    [[nodiscard]] auto vector() const -> Vector const & { return vec_; }
    [[nodiscard]] auto vector() -> Vector & { return vec_; }

    friend auto begin(Iterable &x) { return x.vector().begin(); }
    friend auto begin(Iterable const &x) { return x.vector().begin(); }
    friend auto end(Iterable &x) { return x.vector().end(); }
    friend auto end(Iterable const &x) { return x.vector().end(); }

  private:
    Vector vec_;
};

} // namespace Clingo::Python

namespace pybind11::detail {

template <typename T, typename A> class type_caster<Clingo::Python::Iterable<T, A>> {
  public:
    using value_conv = make_caster<T>;
    using value_type = std::remove_cv_t<T>;
    using type = Clingo::Python::Iterable<T, A>;

    PYBIND11_TYPE_CASTER(type, _("Iterable[") + value_conv::name + _("]"));

    auto load(handle src, bool convert) -> bool {
        if (isinstance<iterable>(src) && !isinstance<str>(src)) {
            auto s = reinterpret_borrow<iterable>(src);
            if (isinstance<sequence>(src)) {
                auto t = reinterpret_borrow<sequence>(src);
                value.vector().reserve(t.size());
            }
            for (auto it : s) {
                value_conv conv;
                if (!conv.load(it, convert)) {
                    return false;
                }
                value.vector().push_back(cast_op<T &&>(std::move(conv)));
            }
            return true;
        }
        return false;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    template <typename U> static auto cast(U &&src, return_value_policy policy, handle parent) -> handle {
        if (!std::is_lvalue_reference_v<U>) {
            policy = return_value_policy_override<U>::policy(policy);
        }
        auto l = list{src.size()};
        auto index = size_t{0};
        for (auto &&value : src) {
            auto value_ = reinterpret_steal<object>(value_conv::cast(forward_like<T>(value), policy, parent));
            if (!value_) {
                return {};
            }
            PyList_SET_ITEM(l.ptr(), static_cast<ssize_t>(index++), value_.release().ptr());
        }
        return l.release();
    }
};

} // namespace pybind11::detail
