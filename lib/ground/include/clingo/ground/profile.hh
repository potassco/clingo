#pragma once

#include <cassert>
#include <cstddef>
#include <numeric>
#include <ostream>

namespace CppClingo::Ground {

//! @addtogroup ground_instantiator
//! @{

//! Helper to print indentation.
struct ProfileIndent {
  public:
    //! Construct the indent with the given amount.
    explicit ProfileIndent(size_t level, size_t width = 2) : level{level}, width{width} {}

    //! Print the given indentation to the given output stream.
    friend auto operator<<(std::ostream &out, ProfileIndent const &indent) -> std::ostream & {
        for (size_t i = 0; i < indent.level * indent.width; ++i) {
            out << ' ';
        }
        return out;
    }

    //! Add indentation to the given indent.
    friend auto operator+(ProfileIndent const &indent, size_t add) -> ProfileIndent {
        return ProfileIndent{indent.level + add, indent.width};
    }

    //! Add indentation to the current indent (inplace).
    friend auto operator+=(ProfileIndent &indent, size_t add) -> ProfileIndent & {
        indent.level += add;
        return indent;
    }

  private:
    //! The indentation level.
    size_t level;
    //! The width of the indentation.
    size_t width;
};

//! Base class for profiling data.
class ProfileNode {
  public:
    //! The default constructor.
    ProfileNode() = default;
    //! Delete the copy constructor.
    ProfileNode(ProfileNode const &other) = delete;
    //! Delete assignment operator.
    auto operator=(ProfileNode const &other) -> ProfileNode & = delete;

    //! Destructor.
    virtual ~ProfileNode() = default;

    //! Print the profiling data to the given output stream.
    //!
    //! The data is indented by the given amount. Nested profiling data is
    //! printed with increased indentation.
    void print(std::ostream &out, ProfileIndent indent) const { do_print(out, indent); }

    //! Compare this profile node with another for equality.
    [[nodiscard]] auto equal(ProfileNode const &node) const -> bool { return do_equal(node); }

    //! Get a score for sorting profile nodes.
    [[nodiscard]] virtual auto score() const -> double { return do_score(); }

  private:
    virtual void do_print(std::ostream &out, ProfileIndent indent) const = 0;
    [[nodiscard]] virtual auto do_equal(ProfileNode const &node) const -> bool = 0;
    [[nodiscard]] virtual auto do_score() const -> double = 0;
};

//! Profile node that can hold children.
class ProfileNodeInternal : public ProfileNode {
  public:
    //! Add a child profile node.
    //!
    //! Returns a reference to the child node that was added.
    template <class T> auto add_child(std::unique_ptr<T> child) -> T & {
        assert(child != nullptr);
        auto *ret = child.get();
        auto *ins = do_add_child(std::move(child));
        assert(dynamic_cast<T *>(ins) != nullptr);
        return ins != nullptr ? *static_cast<T *>(ins) : *ret;
    }

  private:
    //! Add a child profile node.
    virtual auto do_add_child(std::unique_ptr<ProfileNode> child) -> ProfileNode * = 0;
    [[nodiscard]] virtual auto do_nested() const -> bool { return false; }
};

//! A profile node that holds a printable expression and children.
template <typename T> class ProfileNodeExpression : public ProfileNodeInternal {
  public:
    //! Construct the profile node with the given expression.
    ProfileNodeExpression(T expr, bool nested = false) : expr_{std::move(expr)}, nested_{nested} {}

  private:
    //! Add a child profile node.
    auto do_add_child(std::unique_ptr<ProfileNode> child) -> ProfileNode * override {
        for (auto const &x : children_) {
            if (x->equal(*child)) {
                return x.get();
            }
        }
        children_.emplace_back(std::move(child));
        return children_.back().get();
    }

    //! Print the profiling data to the given output stream.
    void do_print(std::ostream &out, ProfileIndent indent) const override {
        out << indent << (nested_ ? "[" : "") << expr_ << (nested_ ? "]" : "") << "\n";
        for (auto const &child : children_) {
            child->print(out, indent + 1);
        }
    }

    //! Compare this profile node with another for equality.
    [[nodiscard]] auto do_equal(ProfileNode const &node) const -> bool override {
        auto const *other = dynamic_cast<ProfileNodeExpression const *>(&node);
        if constexpr (requires { expr_.get(); }) {
            return other != nullptr && nested_ == other->nested_ && expr_.get() == other->expr_.get();
        } else {
            return other != nullptr && nested_ == other->nested_ && expr_ == other->expr_;
        }
    }

    //! Accumulate the scores of all children to get a total score for this node.
    [[nodiscard]] auto do_score() const -> double override {
        return !nested_ ? std::accumulate(children_.begin(), children_.end(), 0.0,
                                          [](double sum, auto const &child) { return sum + child->score(); })
                        : 0;
    }

    //! The expression to print.
    T expr_;
    //! The children of this profile node.
    std::vector<std::unique_ptr<ProfileNode>> children_;
    //! Whether the node is nested.
    bool nested_;
};

//! @}

} // namespace CppClingo::Ground
