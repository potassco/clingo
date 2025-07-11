#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <numeric>
#include <ostream>
#include <vector>

namespace CppClingo::Ground {

//! @addtogroup ground_instantiator
//! @{

//! Whether to process per step or accumulated profiling data.
enum class ProfileType : uint8_t {
    step, //!< Profile data for a single step.
    accu  //!< Profile data accumulated over all steps.
};

//! Whether to print profile data into a compact or detailed form.
enum class ProfileDetail : uint8_t {
    compact = 0,
    detailed = 1,
};

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

struct ProfileStats {
    ProfileStats() = default;

    void reset() { matches = instances = time_instantiate = time_propagate = 0; }

    //! Accumulate the given step stats into this stats object.
    //!
    //! For nested stats, timing information is not accumulated because it is
    //! assumed that the parent already contains the timing information.
    void accumulate(ProfileStats const &stats, bool nested = false) {
        matches += stats.matches;
        instances += stats.instances;
        if (!nested) {
            time_instantiate += stats.time_instantiate;
            time_propagate += stats.time_propagate;
        }
    }

    void print(std::ostream &out, ProfileIndent indent) const;

    [[nodiscard]] auto score() const -> double {
        return static_cast<double>(time_instantiate) + static_cast<double>(time_propagate);
    }

    uint64_t matches = 0;
    uint64_t instances = 0;
    uint64_t time_instantiate = 0;
    uint64_t time_propagate = 0;
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
    void print(std::ostream &out, ProfileIndent indent, ProfileDetail detail, ProfileType type) const {
        do_print(out, indent, detail, type);
    }

    //! Compare this profile node with another for equality.
    [[nodiscard]] auto equal(ProfileNode const &node) const -> bool { return do_equal(node); }

    //! Get a score for sorting profile nodes.
    [[nodiscard]] virtual auto score(ProfileType type) const -> double { return do_score(type); }

    //! Reset the per step statistics.
    virtual void begin_step() { do_begin_step(); }

    //! Accumulate the per step stats into the accumulated stats.
    virtual void end_step() { do_end_step(); }

    //! Combine stats below this node.
    virtual void combine(ProfileStats &stats, ProfileType type, bool nested) const { do_combine(stats, type, nested); }

  private:
    virtual void do_print(std::ostream &out, ProfileIndent indent, ProfileDetail detail, ProfileType type) const = 0;
    [[nodiscard]] virtual auto do_equal(ProfileNode const &node) const -> bool = 0;
    [[nodiscard]] virtual auto do_score(ProfileType type) const -> double = 0;
    virtual void do_combine(ProfileStats &stats, ProfileType type, bool nested) const = 0;
    virtual void do_begin_step() = 0;
    virtual void do_end_step() = 0;
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
    void do_print(std::ostream &out, ProfileIndent indent, ProfileDetail detail, ProfileType type) const override {
        out << indent << (nested_ ? "[" : "") << expr_ << (nested_ ? "]" : "") << "\n";
        if (detail == ProfileDetail::detailed) {
            for (auto const &child : children_) {
                child->print(out, indent + 1, detail, type);
            }
        } else {
            auto stats = ProfileStats{};
            combine(stats, type, false);
            stats.print(out, indent + 1);
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
    [[nodiscard]] auto do_score(ProfileType type) const -> double override {
        return !nested_ ? std::accumulate(children_.begin(), children_.end(), 0.0,
                                          [=](double sum, auto const &child) { return sum + child->score(type); })
                        : 0;
    }

    void do_begin_step() override {
        for (auto &child : children_) {
            child->begin_step();
        }
    }

    void do_end_step() override {
        for (auto &child : children_) {
            child->end_step();
        }
    }

    void do_combine(ProfileStats &stats, ProfileType type, bool nested) const override {
        for (auto const &child : children_) {
            child->combine(stats, type, nested || nested_);
        }
    }

    //! The expression to print.
    T expr_;
    //! The children of this profile node.
    std::vector<std::unique_ptr<ProfileNode>> children_;
    //! Whether the node is nested.
    bool nested_;
};

class ProfileData : public ProfileNode {
  public:
    //! Construct the profile data.
    ProfileData() = default;

  private:
    friend class Instantiator;

    //! Print the profile data to the given output stream.
    void do_print(std::ostream &out, ProfileIndent indent, ProfileDetail detail, ProfileType type) const override {
        std::ignore = detail;
        type == ProfileType::step ? step_.print(out, indent) : accu_.print(out, indent);
    }
    //! Compare two profile nodes.
    [[nodiscard]] auto do_equal(ProfileNode const &node) const -> bool override {
        auto const *other = dynamic_cast<ProfileData const *>(&node);
        return other != nullptr;
    }
    //! Get the score of this profile node.
    [[nodiscard]] auto do_score(ProfileType type) const -> double override {
        return type == ProfileType::step ? step_.score() : accu_.score();
    }
    //! Reset step stats.
    void do_begin_step() override { step_.reset(); }
    //! Accumulate step into accu stats.
    void do_end_step() override { accu_.accumulate(step_); }
    //! Combine the stats of this node with the given stats.
    void do_combine(ProfileStats &stats, ProfileType type, bool nested) const override {
        if (type == ProfileType::step) {
            stats.accumulate(step_, nested);
        } else {
            stats.accumulate(accu_, nested);
        }
    }

    ProfileStats step_;
    ProfileStats accu_;
};

//! @}

} // namespace CppClingo::Ground
