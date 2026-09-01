#include "clingo/util/small_vector.hh"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>

namespace CppClingo::Util::Test {

// NOLINTBEGIN(readability-magic-numbers)

TEST_CASE("small_vector capacity", "[base]") {
    SECTION("small state: capacity is N and holds N elements without allocating") {
        small_vector<int, 4> v;
        REQUIRE(v.size() == 0);
        REQUIRE(v.capacity() == 4);
        REQUIRE(v.capacity() >= v.size());

        auto const *buffer = v.data();
        for (int i = 0; i < 4; ++i) {
            v.push_back(i);
            REQUIRE(v.capacity() >= v.size());
        }
        REQUIRE(v.size() == 4);
        REQUIRE(v.capacity() == 4);      // still in the small buffer
        REQUIRE(v.data() == buffer);     // no allocation while size <= N
    }

    SECTION("large state: capacity reflects the requested reserve") {
        small_vector<int> v; // N == 2
        v.reserve(100);
        REQUIRE(v.capacity() >= 100);    // the bug reported size (0) here
        REQUIRE(v.capacity() >= v.size());
        REQUIRE(v.size() == 0);
    }

    SECTION("capacity() >= size() across the small -> large crossover") {
        small_vector<int> v; // N == 2, so the third element forces a heap buffer
        for (int i = 0; i < 64; ++i) {
            v.push_back(i);
            REQUIRE(v.capacity() >= v.size());
        }
        REQUIRE(v.size() == 64);
        REQUIRE(v.capacity() >= 64);
    }
}

TEST_CASE("small_vector reserve prevents reallocation", "[base]") {
    // The direct regression for the capacity() == size() bug: once space is
    // reserved, filling up to that capacity must not reallocate. With the bug
    // capacity() reported size(), so reserve() was defeated and the first
    // push_back reallocated -- moving the storage out from under `reserved`.
    constexpr std::size_t count = 100;

    small_vector<int> v; // N == 2
    v.reserve(count);
    auto const *reserved = v.data();

    for (std::size_t i = 0; i < count; ++i) {
        v.push_back(static_cast<int>(i));
    }

    REQUIRE(v.size() == count);
    REQUIRE(v.data() == reserved); // storage never moved
    REQUIRE(v.capacity() >= count);

    for (std::size_t i = 0; i < count; ++i) {
        REQUIRE(v[i] == static_cast<int>(i)); // elements intact
    }
}

// NOLINTEND(readability-magic-numbers)

} // namespace CppClingo::Util::Test
