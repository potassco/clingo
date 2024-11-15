#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <vector>

namespace pybind11::detail {

template <typename Type> class iterable_caster {
  public:
    using value_conv = make_caster<Type>;
    using value_type = std::remove_cv_t<Type>;

    PYBIND11_TYPE_CASTER(std::vector<Type>, _("Iterable[") + value_conv::name + _("]"));

    auto load(handle src, bool convert) -> bool {
        if (isinstance<iterable>(src) && !isinstance<str>(src)) {
            auto s = reinterpret_borrow<iterable>(src);
            value = std::vector<Type>{};
            if (isinstance<sequence>(src)) {
                auto t = reinterpret_borrow<sequence>(src);
                value.reserve(t.size());
            }
            for (auto it : s) {
                value_conv conv;
                if (!conv.load(it, convert)) {
                    return false;
                }
                value.push_back(cast_op<Type &&>(std::move(conv)));
            }
            return true;
        }
        return false;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    template <typename T> static auto cast(T &&src, return_value_policy policy, handle parent) -> handle {
        if (!std::is_lvalue_reference_v<T>) {
            policy = return_value_policy_override<Type>::policy(policy);
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
