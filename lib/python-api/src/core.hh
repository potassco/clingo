#pragma once

#include <clingo.h>

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

namespace Clingo::Core {

namespace py = pybind11;

using LoggerCB = std::function<void(clingo_message_e, char const *)>;

static constexpr size_t default_message_limit = 25;

constexpr auto doc(char const *str) -> char const * {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return str + 1;
}

class Library {
  public:
    Library(bool shared, bool slotted, LoggerCB cb, size_t default_message_limit);
    Library(Library const &other) = delete;
    Library(Library &&other) = delete;
    ~Library() noexcept;
    void close() noexcept;

    operator clingo_lib_t *() const;

  private:
    static void logger_(clingo_message_t code, char const *message, void *self);

    clingo_lib_t *lib_ = nullptr;
    LoggerCB cb_;
};

inline void handle_error(clingo_lib_t *lib, bool success) {
    if (!success) {
        auto const *msg = clingo_error_message(lib);
        switch (static_cast<clingo_error_e>(clingo_error_code(lib))) {
            case clingo_error_success:
            case clingo_error_unknown:
            case clingo_error_runtime:
            case clingo_error_logic: {
                throw std::logic_error(msg);
            }
            case clingo_error_bad_alloc: {
                break;
            }
        }
        throw std::bad_alloc();
    }
}

class StringBuilder {
  public:
    StringBuilder();
    StringBuilder(StringBuilder const &other);
    auto operator=(StringBuilder const &other) -> StringBuilder &;
    ~StringBuilder() noexcept;

    [[nodiscard]] auto str() const -> std::string;

    operator clingo_string_builder_t *() { return bld_; };
    operator clingo_string_builder_t const *() const { return bld_; };

  private:
    clingo_string_builder_t *bld_ = nullptr;
};

class Position {
  public:
    explicit Position(clingo_position_t const *pos);
    Position(Library &lib, char const *file, size_t line, size_t column);
    Position(Position const &other);
    Position(Position &&other) noexcept;
    auto operator=(Position const &other) -> Position &;
    auto operator=(Position &&other) noexcept -> Position &;
    ~Position() noexcept;

    [[nodiscard]] auto file() const -> char const *;
    [[nodiscard]] auto line() const -> size_t;
    [[nodiscard]] auto column() const -> size_t;
    [[nodiscard]] auto str() const -> std::string;
    [[nodiscard]] auto repr() const -> std::string;
    [[nodiscard]] auto hash() const -> size_t;

    friend auto operator==(Position const &a, Position const &b) -> bool;
    friend auto operator<=>(Position const &a, Position const &b) -> std::strong_ordering;

    operator clingo_position_t const *() const { return pos_; };

  private:
    clingo_position_t const *pos_ = nullptr;
};

class Location {
  public:
    explicit Location(clingo_location_t const *loc);
    Location(Position const &begin, Position const &end);
    Location(Location const &other);
    Location(Location &&other) noexcept;
    auto operator=(Location const &other) -> Location &;
    auto operator=(Location &&other) noexcept -> Location &;
    ~Location() noexcept;

    [[nodiscard]] auto begin() const -> Position;
    [[nodiscard]] auto end() const -> Position;
    [[nodiscard]] auto str() const -> std::string;
    [[nodiscard]] auto repr() const -> std::string;
    [[nodiscard]] auto hash() const -> size_t;

    friend auto operator==(Location const &a, Location const &b) -> bool;
    friend auto operator<=>(Location const &a, Location const &b) -> std::strong_ordering;

    operator clingo_location_t const *() const { return loc_; };

  private:
    clingo_location_t const *loc_ = nullptr;
};

void register_module(pybind11::module &m);

} // namespace Clingo::Core
