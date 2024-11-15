#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <span>
#include <wpi/SmallVector.h>

namespace pybind11 {
namespace detail {

template <typename Type> struct type_caster<std::span<Type>> {
    using value_conv = make_caster<Type>;
    using value_type = typename std::remove_cv<Type>::type;
    PYBIND11_TYPE_CASTER(std::span<Type>, _("List[") + value_conv::name + _("]"));

    wpi::SmallVector<value_type, 32> vec;
    bool load(handle src, bool convert) {
        if (!isinstance<sequence>(src) || isinstance<str>(src))
            return false;
        auto s = reinterpret_borrow<sequence>(src);
        vec.reserve(s.size());
        for (auto it : s) {
            value_conv conv;
            if (!conv.load(it, convert))
                return false;
            vec.push_back(cast_op<Type &&>(std::move(conv)));
        }
        value = std::span<Type>(std::data(vec), std::size(vec));
        return true;
    }

  public:
    template <typename T> static handle cast(T &&src, return_value_policy policy, handle parent) {
        if (!std::is_lvalue_reference<T>::value)
            policy = return_value_policy_override<Type>::policy(policy);
        list l(src.size());
        size_t index = 0;
        for (auto &&value : src) {
            auto value_ = reinterpret_steal<object>(value_conv::cast(forward_like<T>(value), policy, parent));
            if (!value_)
                return handle();
            PyList_SET_ITEM(l.ptr(), (ssize_t)index++,
                            value_.release().ptr()); // steals a reference
        }
        return l.release();
    }
};

// span specialization: accepts any readonly buffers
template <> struct type_caster<std::span<const uint8_t>> {
    PYBIND11_TYPE_CASTER(std::span<const uint8_t>, _("buffer"));

    bool load(handle src, bool convert) {
        if (!isinstance<buffer>(src))
            return false;
        auto buf = reinterpret_borrow<buffer>(src);
        auto req = buf.request();
        if (req.ndim != 1) {
            return false;
        }

        value = std::span<const uint8_t>((const uint8_t *)req.ptr, req.size * req.itemsize);
        return true;
    }

  public:
    template <typename T> static handle cast(T &&src, return_value_policy policy, handle parent) {
        return bytes((char *)src.data(), src.size()).release();
    }
};

// span specialization: writeable buffer
template <> struct type_caster<std::span<uint8_t>> {
    PYBIND11_TYPE_CASTER(std::span<uint8_t>, _("buffer"));

    bool load(handle src, bool convert) {
        if (!isinstance<buffer>(src))
            return false;
        auto buf = reinterpret_borrow<buffer>(src);
        auto req = buf.request(true); // buffer must be writeable
        if (req.ndim != 1) {
            return false;
        }

        value = std::span<uint8_t>((uint8_t *)req.ptr, req.size * req.itemsize);
        return true;
    }

  public:
    template <typename T> static handle cast(T &&src, return_value_policy policy, handle parent) {
        // TODO: should this be a memoryview instead?
        return bytes((char *)src.data(), src.size()).release();
    }
};

} // namespace detail
} // namespace pybind11
