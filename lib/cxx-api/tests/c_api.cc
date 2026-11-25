#include <clingo/core.h>

#include <catch2/catch_test_macros.hpp>

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

} // namespace Clingo::Test
