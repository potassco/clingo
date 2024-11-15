#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <vector>

namespace Clingo::Python {

template <class T, class Alloc = std::allocator<T>> class Array : public std::vector<T, Alloc> {
  public:
    using std::vector<T, Alloc>::vector;
};

} // namespace Clingo::Python

namespace pybind11::detail {

template <typename T, typename A> class type_caster<Clingo::Python::Array<T, A>> {
  public:
    using value_conv = make_caster<T>;
    using value_type = std::remove_cv_t<T>;
    using array_type = Clingo::Python::Array<T, A>;

    PYBIND11_TYPE_CASTER(array_type, _("Iterable[") + value_conv::name + _("]"));

    auto load(handle src, bool convert) -> bool {
        if (isinstance<iterable>(src) && !isinstance<str>(src)) {
            auto s = reinterpret_borrow<iterable>(src);
            value = Clingo::Python::Array<T, A>{};
            if (isinstance<sequence>(src)) {
                auto t = reinterpret_borrow<sequence>(src);
                value.reserve(t.size());
            }
            for (auto it : s) {
                value_conv conv;
                if (!conv.load(it, convert)) {
                    return false;
                }
                value.push_back(cast_op<T &&>(std::move(conv)));
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
