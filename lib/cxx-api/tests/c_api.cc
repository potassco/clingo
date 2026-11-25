#include <clingo/ast.h>
#include <clingo/core.h>
#include <clingo/symbol.h>

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

TEST_CASE("c-api-symbol") {
    clingo_lib_t *lib = nullptr;
    REQUIRE(clingo_lib_new(0, clingo_log_level_info, nullptr, nullptr, 0, &lib));
    auto sym = clingo_symbol_t{0};
    REQUIRE(failed_invalid(clingo_symbol_create_string(lib, nullptr, 1, &sym), "clingo_symbol_create_string"));
    REQUIRE(failed_invalid(clingo_symbol_create_tuple(lib, nullptr, 1, &sym), "clingo_symbol_create_tuple"));
    REQUIRE(failed_invalid(clingo_symbol_create_function(lib, "f", 1, nullptr, 2, true, &sym),
                           "clingo_symbol_create_function"));
    REQUIRE(failed_invalid(clingo_parse_term(nullptr, "f", 1, &sym), "clingo_parse_term"));
    REQUIRE(clingo_parse_term(lib, "f(1,2)", 6, &sym));
    auto size = size_t{0};
    REQUIRE(clingo_symbol_arguments(sym, nullptr, &size));
    REQUIRE(size == 2);
    REQUIRE(clingo_symbol_arguments(sym, nullptr, nullptr));
    auto num = int32_t{0};
    REQUIRE(failed_invalid(clingo_symbol_number(sym, &num), "clingo_symbol_number"));
    auto str = clingo_string_t{nullptr, 0};
    REQUIRE(failed_invalid(clingo_symbol_string(sym, &str), "clingo_symbol_string"));
    REQUIRE(failed_invalid(clingo_symbol_name(sym, nullptr), "clingo_symbol_name"));
    REQUIRE(failed_invalid(clingo_symbol_to_string(sym, nullptr), "clingo_symbol_to_string"));
    clingo_symbol_acquire(0);
    clingo_symbol_release(0);
    clingo_symbol_release(sym);
    clingo_lib_release(lib);
}

TEST_CASE("c-api-ast") {
    clingo_lib_t *lib = nullptr;
    REQUIRE(clingo_lib_new(0, clingo_log_level_info, nullptr, nullptr, 0, &lib));
    clingo_ast_t *ast = nullptr;
    REQUIRE(failed_invalid(clingo_ast_parse_expression(nullptr, clingo_ast_parse_type_term, "a", 1, &ast),
                           "clingo_ast_parse_expression"));
    REQUIRE(failed_invalid(clingo_ast_parse_expression(lib, 42, "a", 1, &ast), "clingo_ast_parse_expression"));
    REQUIRE(clingo_ast_parse_expression(lib, clingo_ast_parse_type_statement, "a.", 2, &ast));
    clingo_ast_t *cpy = ast;
    REQUIRE(clingo_ast_copy(nullptr, &cpy));
    REQUIRE(cpy == nullptr);
    clingo_ast_rewrite_context_t *ctx = nullptr;
    REQUIRE(failed_invalid(clingo_ast_rewrite_context_create(nullptr, &ctx), "clingo_ast_rewrite_context_create"));
    REQUIRE(clingo_ast_rewrite_context_create(lib, &ctx));
    REQUIRE(
        failed_invalid(clingo_ast_rewrite_context_add_param(nullptr, "a", 1), "clingo_ast_rewrite_context_add_param"));
    REQUIRE(
        failed_invalid(clingo_ast_rewrite_context_add_theory(ctx, nullptr), "clingo_ast_rewrite_context_add_theory"));
    clingo_ast_t **res = nullptr;
    auto size = size_t{0};
    REQUIRE(failed_invalid(clingo_ast_rewrite(nullptr, ast, &res, &size), "clingo_ast_rewrite"));
    REQUIRE(failed_invalid(clingo_ast_rewrite(ctx, ast, nullptr, &size), "clingo_ast_rewrite"));
    REQUIRE(clingo_ast_rewrite(ctx, ast, &res, &size));
    clingo_ast_array_free(res, size);
    clingo_program_t *prg = nullptr;
    REQUIRE(failed_invalid(clingo_program_new(nullptr, &prg), "clingo_program_new"));
    REQUIRE(clingo_program_new(lib, &prg));
    REQUIRE(failed_invalid(clingo_program_add(prg, nullptr), "clingo_program_add"));
    REQUIRE(clingo_program_add(prg, ast));
    clingo_program_free(prg);
    clingo_program_free(nullptr);
    clingo_ast_rewrite_context_free(ctx);
    clingo_ast_rewrite_context_free(nullptr);
    clingo_ast_free(ast);
    clingo_ast_free(nullptr);
    clingo_ast_array_free(nullptr, 0);
    clingo_lib_release(lib);
}

} // namespace Clingo::Test
