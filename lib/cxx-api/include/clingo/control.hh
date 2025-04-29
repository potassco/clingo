#pragma once

#include <clingo/core.hh>
#include <clingo/symbol.hh>

#include <clingo/ast.h>
#include <clingo/control.h>
#include <clingo/observe.h>

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
    using Context = std::function<SymbolVector(char const *, SymbolSpan)>;

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

    // TODO: wrap mode
    auto mode() -> clingo_mode_e {
        clingo_mode_t mode = 0;
        Detail::handle_error(clingo_control_mode(rep_, &mode));
        return static_cast<clingo_mode_e>(mode);
    }

    // TODO: wrap program
    void join(clingo_program_t *prg) { Detail::handle_error(clingo_control_join(rep_, prg)); }

    // TODO: create enum
    void write_aspif(char const *path, bool symbols, bool append, std::optional<bool> preamble, bool preprocess) {
        clingo_write_aspif_mode_t mode = 0;
        if (symbols) {
            mode |= clingo_write_aspif_mode_symbols;
        }
        if (append) {
            mode |= clingo_write_aspif_mode_append;
        }
        if (!preamble) {
            mode |= clingo_write_aspif_mode_preamble_auto;
        } else if (*preamble) {
            mode |= clingo_write_aspif_mode_preamble;
        }
        if (preprocess) {
            mode |= clingo_write_aspif_mode_preprocess;
        }
        Detail::handle_error(clingo_control_write_aspif(rep_, path, mode));
    }

    void parse_files(std::span<std::string const> files) {
        auto cfiles = Detail::transform(files, [](auto const &x) { return x.c_str(); });
        Detail::handle_error(clingo_control_parse_files(rep_, cfiles.data(), cfiles.size()));
    }

    void parse_string(char const *program) { Detail::handle_error(clingo_control_parse_string(rep_, program)); }

    void ground(std::optional<PartSpan> parts = std::nullopt, Context ctx = nullptr) {
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
        Detail::handle_error(clingo_control_ground(rep_, c_parts.data(), c_parts.size(), ctx ? &ctx_ : nullptr, &ctx));
    }

  private:
    static auto ctx_([[maybe_unused]] clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *location,
                     char const *name, clingo_symbol_t const *arguments, size_t arguments_size, void *data,
                     clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) -> clingo_result_t {
        CLINGO_TRY {
            auto &cb = *static_cast<std::function<SymbolVector(char const *, SymbolSpan)> *>(data);
            auto syms = cb(name, {cpp_cast(arguments), arguments_size});
            auto const *c_syms = c_cast(syms.data());
            return symbol_callback(c_syms, syms.size(), symbol_callback_data);
        }
        CLINGO_CATCH;
    }

    clingo_control_t *rep_ = nullptr;
};

} // namespace Clingo
