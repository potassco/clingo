#pragma once

#include <clingo.h>

#include <pybind11/pybind11.h>

// NOLINTBEGIN(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

#define CLINGO_TRY try
#define CLINGO_CATCH(lib)                                                                                              \
    catch (...) {                                                                                                      \
        return handle_error(lib);                                                                                      \
    }                                                                                                                  \
    return clingo_result_success

// NOLINTEND(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

namespace Clingo::Python {

namespace py = pybind11;

using Logger = std::function<void(clingo_message_e, char const *)>;

static constexpr size_t default_message_limit = 25;

class Library {
  public:
    Library(clingo_lib_t *lib) : lib_{lib}, own_{false} {}
    Library(bool shared, bool slotted, Logger cb, size_t default_message_limit);
    Library(Library const &other) = delete;
    Library(Library &&other) = delete;
    ~Library() noexcept;
    void close() noexcept;

    operator clingo_lib_t *() const;

  private:
    static void logger_(clingo_message_t code, char const *message, void *self) noexcept;

    clingo_lib_t *lib_ = nullptr;
    Logger cb_ = nullptr;
    bool own_{true};
};

inline auto handle_error(clingo_lib_t *lib) -> clingo_result_t {
    try {
        throw;
    } catch (std::invalid_argument const &e) {
        clingo_lib_report(lib, clingo_message_error, e.what());
        return clingo_result_invalid;
    } catch (std::range_error const &e) {
        clingo_lib_report(lib, clingo_message_error, e.what());
        return clingo_result_range;
    } catch (std::bad_alloc const &e) {
        clingo_lib_report(lib, clingo_message_error, e.what());
        return clingo_result_bad_alloc;
    } catch (std::logic_error const &e) {
        clingo_lib_report(lib, clingo_message_error, e.what());
        return clingo_result_logic;
    } catch (std::exception const &e) {
        clingo_lib_report(lib, clingo_message_error, e.what());
        return clingo_result_runtime;
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

void register_core(pybind11::module &m);

} // namespace Clingo::Python
