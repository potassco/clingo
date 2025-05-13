#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <span>
#include <vector>

namespace PyClingo {

namespace py = pybind11;

template <typename T> class Sequence : public py::object {
    PYBIND11_OBJECT_DEFAULT(Sequence, py::object, PyObject_Type)
    using py::object::object;
};

template <typename T> class Annotation : public py::object {
    PYBIND11_OBJECT_DEFAULT(Annotation, py::object, PyObject_Type)
    using py::object::object;
};

// NOLINTBEGIN
template <size_t N> struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) { std::copy_n(str, N, value); }
    char value[N]{};
};
// NOLINTEND

template <StringLiteral L> class TypeHint : public py::object {
    PYBIND11_OBJECT_DEFAULT(TypeHint, py::object, PyObject_Type)
    using py::object::object;
};

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

} // namespace PyClingo

namespace pybind11::detail {

template <typename T> struct handle_type_name<PyClingo::Sequence<T>> {
    static constexpr auto name = const_name("Sequence[") + make_caster<T>::name + const_name("]");
};

template <typename T> struct handle_type_name<PyClingo::Annotation<T>> {
    static constexpr auto name = make_caster<T>::name;
};

template <PyClingo::StringLiteral L> struct handle_type_name<PyClingo::TypeHint<L>> {
    static constexpr auto name = const_name(L.value);
};

template <typename T, typename A> class type_caster<PyClingo::Iterable<T, A>> {
  public:
    using value_conv = make_caster<T>;
    using value_type = std::remove_cv_t<T>;
    using type = PyClingo::Iterable<T, A>;

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
            auto py_value = reinterpret_steal<object>(value_conv::cast(forward_like<T>(value), policy, parent));
            if (!py_value) {
                return {};
            }
            PyList_SET_ITEM(l.ptr(), static_cast<ssize_t>(index++), py_value.release().ptr());
        }
        return l.release();
    }
};

template <typename T> struct type_caster<std::span<T>> {
    using value_type = std::remove_cv_t<T>;
    using value_conv = make_caster<value_type>;
    using type = std::span<T>;

    PYBIND11_TYPE_CASTER(type, _("Sequence[") + make_caster<T>::name + _("]"));

    auto load(handle src, bool convert) -> bool {
        if (!isinstance<sequence>(src)) {
            return false;
        }
        storage_.clear();
        for (auto const &val : src) {
            auto conv = value_conv{};
            if (!conv.load(val, convert)) {
                return false;
            }
            storage_.emplace_back(cast_op<value_type &&>(std::move(conv)));
        }
        value = type{storage_.data(), storage_.size()};
        return true;
    }
    static auto cast(std::span<T> const &src, [[maybe_unused]] return_value_policy policy, handle parent) -> handle {
        list result{src.size()};
        size_t index = 0;
        for (auto const &item : src) {
            auto py_item = reinterpret_steal<object>(value_conv::cast(item, return_value_policy::copy, parent));
            if (!py_item) {
                return {};
            }
            PyList_SET_ITEM(result.ptr(), static_cast<ssize_t>(index++), py_item.release().ptr());
        }
        return result.release();
    }

  private:
    std::vector<value_type> storage_;
};

} // namespace pybind11::detail
