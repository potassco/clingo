#pragma once

#include <clingo/core.hh>
#include <clingo/symbol.hh>

#include <clingo/control.h>

#include <cassert>
#include <span>

namespace Clingo {

using StringSpan = std::span<char const *const>;

struct Part {
    Part(std::string name, SymbolVector params = {}) : name{std::move(name)}, params(std::move(params)) {
        assert(!this->name.empty());
    }
    std::string name;
    SymbolVector params;
};
using PartSpan = std::span<Part const>;

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

    Control(Library const &lib, std::initializer_list<char const *> arguments)
        : Clingo::Control{lib, StringSpan{arguments.begin(), arguments.size()}} {}
    Control(Library const &lib, StringSpan arguments = {}) {
        Detail::handle_error(clingo_control_new(c_cast(lib), arguments.data(), arguments.size(), &rep_));
    }
    explicit Control(clingo_control_t *rep, bool acquire) : rep_{rep} {
        if (acquire) {
            clingo_control_acquire(rep_);
        }
    }

    [[nodiscard]] friend auto c_cast(Control const &ctl) -> clingo_control_t * { return ctl.rep_; }

    void parse_string(char const *program) { Detail::handle_error(clingo_control_parse_string(rep_, program)); }

    void ground(std::optional<PartSpan> parts = std::nullopt) {
        // TODO:
        // - need context
        std::vector<clingo_part_t> c_parts;
        if (parts) {
            c_parts.reserve(parts->size());
            for (auto const &part : *parts) {
                c_parts.emplace_back(part.name.c_str(), c_cast(part.params.data()), part.params.size());
            }
        } else {
            c_parts.reserve(1);
            c_parts.emplace_back("base", nullptr, 0);
        }
        Detail::handle_error(clingo_control_ground(rep_, c_parts.data(), c_parts.size(), nullptr, nullptr));
    }

  private:
    clingo_control_t *rep_ = nullptr;
};

} // namespace Clingo
