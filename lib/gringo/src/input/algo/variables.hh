#pragma once

#include <functional>

#include <gringo/util/unordered_map.hh>

#include <gringo/input/statement.hh>

namespace Gringo::Input {

namespace Detail {

using VarOccCounts = Util::unordered_map<std::string, size_t>;
using VarVisitFun = std::function<void(std::string const &var)>;

template <class P> class VarVisitHelper : public P {
  public:
    using P::add;

    template <class... T> VarVisitHelper(T &&...args) : P{std::forward<T>(args)...} {}

    template <class T> auto add(T const &x) -> std::void_t<decltype(visit_variables(x, std::declval<VarVisitFun>()))> {
        P::visit_(x);
    }

    template <class T> auto add(T const &x) -> std::enable_if_t<std::is_enum_v<T>> { static_cast<void>(x); }

    void add(std::string const &x) { static_cast<void>(x); }

    void add(Projection const &x) { static_cast<void>(x); }

    template <class T> void add(Util::immutable_value<T> const &ptr) { add(*ptr); }

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

    template <class A, class B, class... C> void add(A const &a, B const &b, C const &...args) {
        add(a);
        add(b);
        (add(args), ...);
    }

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

struct no_such {};

class VarCounterHelper {
  public:
    VarCounterHelper(VarOccCounts const &global) : global_{global} {}

    [[nodiscard]] operator VarOccCounts const &() { return local_; }

  protected:
    template <class T> void visit_(T const &x) {
        visit_variables(x, [this](std::string const &var) {
            if (!global_.contains(var)) {
                ++local_[var];
            }
        });
    }
    static void add(no_such x) { static_cast<void>(x); };

  private:
    VarOccCounts const &global_;
    VarOccCounts local_;
};

class VarVisitorHelper {
  public:
    VarVisitorHelper(VarVisitFun const &fun) : fun_{fun} {}
    [[nodiscard]] auto visitor() const -> VarVisitFun const & { return fun_; }

  protected:
    template <class T> void visit_(T const &var) { visit_variables(var, fun_); }
    static void add(no_such x) { static_cast<void>(x); };

  private:
    VarVisitFun const &fun_;
};

} // namespace Detail

using VarCounter = Detail::VarVisitHelper<Detail::VarCounterHelper>;
using VarVisitor = Detail::VarVisitHelper<Detail::VarVisitorHelper>;

} // namespace Gringo::Input
