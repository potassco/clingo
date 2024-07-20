#include <catch2/catch_test_macros.hpp>

namespace Gringo::Input {

void test();

}

namespace Gringo::Input::Test {

TEST_CASE("lex_test") { test(); }

} // namespace Gringo::Input::Test
