#include <clingo/core.h>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>

namespace Clingo::Test {

namespace {

//! Get the code of the current error.
auto error_code() -> clingo_result_t {
    auto code = clingo_result_t{clingo_result_success};
    clingo_get_error(&code, nullptr);
    return code;
}

//! Get the message of the current error.
auto error_message() -> std::string_view {
    auto msg = clingo_string_t{nullptr, 0};
    clingo_get_error(nullptr, &msg);
    return {msg.data, msg.size};
}

//! Check that the last call failed with invalid arguments of the given function.
auto failed_invalid(bool res, std::string_view name) -> bool {
    auto ok = !res && error_code() == clingo_result_invalid &&
              error_message() == std::string{"invalid arguments: "}.append(name);
    clingo_clear_error();
    return ok;
}

//! Logger counting how often its data was freed.
struct Logger {
    static void log([[maybe_unused]] clingo_message_t code, [[maybe_unused]] char const *message,
                    [[maybe_unused]] size_t size, [[maybe_unused]] void *data) {}
    static void free(void *data) { ++*static_cast<int *>(data); }
};

} // namespace

TEST_CASE("c-api-error") {
    REQUIRE(!clingo_set_error(clingo_result_runtime, "abc", 2));
    REQUIRE(error_code() == clingo_result_runtime);
    REQUIRE(error_message() == "ab");
    REQUIRE(!clingo_set_error(clingo_result_logic, nullptr, 3));
    REQUIRE(error_code() == clingo_result_logic);
    REQUIRE(error_message().empty());
    clingo_get_error(nullptr, nullptr);
    clingo_clear_error();
    REQUIRE(error_code() == clingo_result_success);
}

TEST_CASE("c-api-core") {
    SECTION("version") {
        auto major = 0;
        clingo_version(&major, nullptr, nullptr);
        REQUIRE(major == CLINGO_VERSION_MAJOR);
    }
    SECTION("lib") {
        auto freed = 0;
        auto logger = clingo_logger_t{Logger::log, Logger::free};
        clingo_lib_t *lib = nullptr;
        // the logger data is freed also if creating the library fails
        REQUIRE(
            failed_invalid(clingo_lib_new(0, clingo_log_level_info, &logger, &freed, 0, nullptr), "clingo_lib_new"));
        REQUIRE(freed == 1);
        REQUIRE(failed_invalid(clingo_lib_new(0, 42, &logger, &freed, 0, &lib), "clingo_lib_new"));
        REQUIRE(freed == 2);
        REQUIRE(failed_invalid(clingo_lib_new(8, clingo_log_level_info, &logger, &freed, 0, &lib), "clingo_lib_new"));
        REQUIRE(freed == 3);
        auto no_log = clingo_logger_t{nullptr, Logger::free};
        REQUIRE(failed_invalid(clingo_lib_new(0, clingo_log_level_info, &no_log, &freed, 0, &lib), "clingo_lib_new"));
        REQUIRE(freed == 4);
        REQUIRE(clingo_lib_new(0, clingo_log_level_info, &logger, &freed, 0, &lib));
        REQUIRE(freed == 4);
        REQUIRE(failed_invalid(clingo_lib_set_owner(lib, nullptr, nullptr), "clingo_lib_set_owner"));
        REQUIRE(failed_invalid(clingo_lib_add_finalizer(lib, nullptr, nullptr), "clingo_lib_add_finalizer"));
        clingo_lib_release(lib);
        REQUIRE(freed == 5);
        clingo_lib_release(nullptr);
        clingo_lib_acquire(nullptr);
    }
    SECTION("string builder") {
        clingo_string_builder_t *bld = nullptr;
        REQUIRE(failed_invalid(clingo_string_builder_new(nullptr), "clingo_string_builder_new"));
        REQUIRE(clingo_string_builder_new(&bld));
        REQUIRE(clingo_string_builder_append(bld, nullptr, 0));
        REQUIRE(failed_invalid(clingo_string_builder_append(bld, nullptr, 1), "clingo_string_builder_append"));
        REQUIRE(failed_invalid(clingo_string_builder_string(bld, nullptr), "clingo_string_builder_string"));
        REQUIRE(failed_invalid(clingo_string_builder_copy(bld, nullptr), "clingo_string_builder_copy"));
        clingo_string_builder_free(bld);
        clingo_string_builder_free(nullptr);
    }
    SECTION("location") {
        clingo_lib_t *lib = nullptr;
        REQUIRE(clingo_lib_new(0, clingo_log_level_info, nullptr, nullptr, 0, &lib));
        clingo_position_t const *pos = nullptr;
        REQUIRE(failed_invalid(clingo_position_new(lib, nullptr, 1, 1, 1, &pos), "clingo_position_new"));
        REQUIRE(failed_invalid(clingo_position_new(nullptr, "a", 1, 1, 1, &pos), "clingo_position_new"));
        REQUIRE(clingo_position_new(lib, nullptr, 0, 1, 1, &pos));
        clingo_position_t const *cpy = pos;
        REQUIRE(clingo_position_copy(nullptr, &cpy));
        REQUIRE(cpy == nullptr);
        clingo_location_t const *loc = nullptr;
        REQUIRE(failed_invalid(clingo_location_new(pos, nullptr, &loc), "clingo_location_new"));
        REQUIRE(clingo_location_new(pos, pos, &loc));
        REQUIRE(failed_invalid(clingo_location_to_string(loc, nullptr), "clingo_location_to_string"));
        REQUIRE(failed_invalid(clingo_position_to_string(nullptr, nullptr), "clingo_position_to_string"));
        clingo_location_free(loc);
        clingo_position_free(pos);
        clingo_location_free(nullptr);
        clingo_position_free(nullptr);
        clingo_lib_release(lib);
    }
}

} // namespace Clingo::Test
