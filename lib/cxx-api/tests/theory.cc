#include <clingo/control.hh>
#include <clingo/theory.hh>

#include <catch2/catch_test_macros.hpp>

namespace Clingo::Test {

namespace {

struct Fixture {
    Library lib;
    Control ctl{lib, {"0"}};
};

/*
struct TestTheory {
    static auto info(void *self, clingo_string_t *name, int *major, int *minor, int *revision) -> bool {}
    static void destroy(void *self) {}
    static auto register_theory(void *self, clingo_control_t *control) -> bool {}
    static auto rewrite_ast(void *self, clingo_ast_t *statement, clingo_theory_ast_callback_t callback, void *data)
        -> bool {}
    static auto prepare(void *self, clingo_control_t *control) -> bool {}
    static auto register_options(void *self, clingo_options_t *options) -> bool {}
    static auto validate_options(void *self) -> bool {}
    static auto configure(void *self, char const *key, size_t key_size, char const *value, size_t value_size) -> bool {}
    static auto on_model(void *self, clingo_model_t *model) -> bool {}
    static auto on_stats(void *self, clingo_stats_t *stats) -> bool {}
    static auto lookup_symbol(void *self, clingo_symbol_t symbol, size_t *index, bool *found) -> bool {}
    static auto assignment_next(void *self, uint32_t thread_id, bool *init, size_t *index, bool *has_value) -> bool {}
    static auto assignment_get_value(void *self, uint32_t thread_id, size_t index, clingo_symbol_t *symbol,
                                     clingo_theory_value_t *value, bool *has_value) -> bool {}
};
*/

} // namespace

TEST_CASE_METHOD(Fixture, "theory", "[cxx][theory]") {
}

} // namespace Clingo::Test
