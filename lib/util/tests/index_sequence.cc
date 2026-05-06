#include "clingo/util/index_sequence.hh"

#include <catch2/catch_test_macros.hpp>

#include <random>

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

TEST_CASE("index sequence random", "[base]") {
    static constexpr size_t n = 1000;
    auto vec = std::vector<size_t>(n);
    size_t i = 0;
    for (auto &x : vec) {
        x = i;
        ++i;
    }
    auto rd = std::random_device{};
    auto gen = std::mt19937_64{rd()};
    std::ranges::shuffle(vec, gen);
    auto seq = index_sequence<size_t>{};
    for (auto const &x : vec) {
        seq.add(x);
    }
    i = 0;
    for (auto const &x : vec) {
        REQUIRE(seq[i] == x);
        REQUIRE(seq.find(x) == i);
        ++i;
    }
    auto it = vec.begin();
    for (auto const &x : seq) {
        REQUIRE(x == *it);
        ++it;
    }
}

// NOLINTEND(modernize-use-designated-initializers,readability-magic-numbers)

} // namespace CppClingo::Util::Test
