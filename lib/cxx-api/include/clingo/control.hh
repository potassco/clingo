#pragma once

#include <clingo/core.hh>

#include <clingo/control.h>

#include <span>

namespace Clingo {

using StringSpan = std::span<char const *>;

class Control {
  public:
    ~Control() { clingo_control_release(rep_); }

    Control(Control const &other) noexcept : rep_{other.rep_} { clingo_control_acquire(rep_); }
    auto operator=(Control const &other) noexcept -> Control & {
        clingo_control_acquire(other.rep_);
        clingo_control_release(rep_);
        rep_ = other.rep_;
        return *this;
    }

    Control(Control &&other) noexcept : rep_{std::exchange(other.rep_, nullptr)} {}
    auto operator=(Control &&other) noexcept -> Control & {
        if (rep_ != other.rep_) {
            clingo_control_release(rep_);
            rep_ = std::exchange(other.rep_, nullptr);
        }
        return *this;
    }

    Control(Library const &lib, StringSpan arguments = {}) {
        Detail::handle_error(clingo_control_new(c_cast(lib), arguments.data(), arguments.size(), &rep_));
    }
    explicit Control(clingo_control_t *rep, bool acquire) : rep_{rep} {
        if (acquire) {
            clingo_control_acquire(rep_);
        }
    }

    [[nodiscard]] friend auto c_cast(Control const &ctl) -> clingo_control_t * { return ctl.rep_; }

  private:
    clingo_control_t *rep_ = nullptr;
};

} // namespace Clingo
