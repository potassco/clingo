#include "clingo/util/index_sequence.hh"

#include <catch2/catch_test_macros.hpp>

#include <sstream>

namespace CppClingo::Util::Test {

TEST_CASE("index sequence", "[base]") {
    auto x = index_sequence<size_t>{};
    x.add(1);
    x.add(0);
    REQUIRE(x.find(1) == 0);
    REQUIRE(x.find(0) == 1);
    REQUIRE(x[0] == 1);
    REQUIRE(x[1] == 0);
}

// NOLINTEND(modernize-use-designated-initializers,readability-magic-numbers)

} // namespace CppClingo::Util::Test
