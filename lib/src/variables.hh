#pragma once

#include <functional>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#include <util/shared_ptr.hh>

namespace detail {

using VarOccCounts = std::unordered_map<std::string, size_t>;
using VarVisitFun = std::function<void(std::string const &var)>;

template <class P> class VarVisitHelper : public P {
  public:
    template <class... T> VarVisitHelper(T &&...args) : P{std::forward<T>(args)...} {}

    template <class T> auto add(T const &x) -> std::void_t<decltype(x.visit_variables(std::declval<VarVisitFun>()))> {
        P::visit_(x);
    }

    template <class T> auto add(T const &x) -> std::enable_if_t<std::is_enum_v<T>> { static_cast<void>(x); }

    template <class T> void add(shared_ptr<T> const &ptr) { add(*ptr); }

    template <class T> void add(std::optional<T> const &opt) {
        if (opt.has_value()) {
            add(opt.value());
        }
    }

    template <class T> void add(std::vector<T> const &vec) {
        for (auto const &x : vec) {
            add(x);
        }
    }

    template <class A, class B> auto add(std::pair<A, B> const &p) {
        add(p.first);
        add(p.second);
    }

    template <class... A> auto add(std::tuple<A...> const &tup) { add_(tup, std::index_sequence_for<A...>{}); }

    template <class... A> auto add(std::variant<A...> const &var) { add_(var, std::index_sequence_for<A...>{}); }

    template <class... T> void add(T const &...args) { (add(args), ...); }

  private:
    template <class... T, size_t... Indices>
    void add_(std::tuple<T...> const &tup, std::index_sequence<Indices...> indices) {
        static_cast<void>(indices);
        return (add(std::get<Indices>(tup)), ...);
    }

    template <size_t i, class... T> void add_(size_t j, std::variant<T...> const &var) {
        if (i == j) {
            add(std::get<i>(var));
        }
    }

    template <class... T, size_t... Indices>
    void add_(std::variant<T...> const &var, std::index_sequence<Indices...> indices) {
        static_cast<void>(indices);
        size_t i = var.index();
        (add_<Indices>(i, var), ...);
    }
};

class VarCounterHelper {
  public:
    VarCounterHelper(VarOccCounts const &global) : global_{global} {}

    [[nodiscard]] operator VarOccCounts const &() { return local_; }

  protected:
    template <class T> void visit_(T const &x) {
        x.visit_variables([this](std::string const &var) {
            if (!global_.contains(var)) {
                ++local_[var];
            }
        });
    }

  private:
    VarOccCounts const &global_;
    VarOccCounts local_;
};

class VarVisitorHelper {
  public:
    VarVisitorHelper(VarVisitFun const &fun) : fun_{fun} {}
    [[nodiscard]] auto visitor() const -> VarVisitFun const & { return fun_; }

  protected:
    template <class T> void visit_(T const &var) { var.visit_variables(fun_); }

  private:
    VarVisitFun const &fun_;
};

} // namespace detail

using VarCounter = detail::VarVisitHelper<detail::VarCounterHelper>;
using VarVisitor = detail::VarVisitHelper<detail::VarVisitorHelper>;
