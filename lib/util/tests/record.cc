#include <clingo/util/record.hh>

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Util::Test {

namespace {

struct TestValue {
    TestValue(int &moves, int &copies) : moves{&moves}, copies{&copies} {}
    TestValue(TestValue const &x) : moves{x.moves}, copies{x.copies} { ++*copies; }
    TestValue(TestValue &&x) noexcept : moves{x.moves}, copies{x.copies} { ++*moves; }
    int *moves;
    int *copies;
};

constexpr auto a_val = Util::Record::AttributeName<1>{};

struct TestRecord : public Record::Base<TestRecord> {
  public:
    static constexpr auto attributes() { return std::tuple{a_val = &TestRecord::val}; }

    template <std::convertible_to<TestValue> T> explicit TestRecord(T &&val) : val{std::forward<T>(val)} {}

    TestValue val;
};

} // namespace

TEST_CASE("record") {
    int moves = 0;
    int copies = 0;
    auto x = TestRecord(TestValue{moves, copies});
    REQUIRE(copies == 0);
    REQUIRE(moves == 1);
    std::ignore = x.update(a_val = TestValue{moves, copies});
    REQUIRE(copies == 0);
    REQUIRE(moves == 2);
}

} // namespace CppClingo::Util::Test
