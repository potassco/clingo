#include <clingo/symbol.hh>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

// NOLINTBEGIN(readability-magic-numbers)

namespace Clingo::Test {

TEST_CASE("number symbols", "[cxx][symbol][number]") {
    auto lib = Library{};
    auto small = 13;
    auto big = std::string_view{"904785927514189326478963487364879364981736498173641"};

    REQUIRE(Number(small).number() == small);
    REQUIRE(Number(lib, big).to_string() == big);
}

TEST_CASE("extreme symbols", "[cxx][symbol][extreme]") {
    REQUIRE(Infimum().to_string() == "#inf");
    REQUIRE(Supremum().to_string() == "#sup");
}

TEST_CASE("string symbols", "[cxx][symbol][string]") {
    auto lib = Library{};
    auto val = std::string_view{"sh2354n\nshoeu\"insh"};
    auto sym = String(lib, val);
    REQUIRE(sym.string() == val);
    REQUIRE(sym.to_string() == "\"sh2354n\\nshoeu\\\"insh\"");
}

TEST_CASE("tuple symbols", "[cxx][symbol][string]") {
    auto lib = Library{};
    auto a = Number(1);
    auto b = String(lib, "a");
    auto args = std::vector<Symbol>{a, b};
    Symbol tup = Tuple(lib, args);
    REQUIRE(tup.arguments().size() == 2);
    REQUIRE(tup.arguments()[0] == a);
    REQUIRE(tup.arguments()[1] == b);
}

TEST_CASE("function symbols", "[cxx][symbol][string]") {
    auto lib = Library{};
    auto name = std::string_view{"f"};
    auto a = Number(1);
    auto b = String(lib, "a");
    auto args = std::vector<Symbol>{a, b};

    for (bool is_positive : {true, false}) {
        auto f = Function(lib, name, args, is_positive);
        REQUIRE(f.name() == name);
        REQUIRE(f.arguments().size() == args.size());
        REQUIRE(f.arguments()[0] == a);
        REQUIRE(f.arguments()[1] == b);
        REQUIRE(f.is_positive() == is_positive);
        REQUIRE(f.is_negative() == !is_positive);
    }
}

TEST_CASE("match symbols", "[cxx][symbol][match]") {
    auto lib = Library{};
    auto a = Number(2);
    auto b = Function(lib, "f", {a}, true);
    auto c = Tuple(lib, {a, b});

    REQUIRE(!a.match("1", 0));
    REQUIRE(!a.match("f", 1));

    REQUIRE(b.match("f", 1, true));
    REQUIRE(!b.match("f", 1, false));

    REQUIRE(c.arguments().size() == 2);
    REQUIRE(c.match(2));
    REQUIRE(!c.match(1));
}

TEST_CASE("parse symbols", "[cxx][symbol][parse]") {
    auto lib = Library{};
    auto str = std::string_view{"f((1,),2,a,-b,(1,2),#inf,#sup)"};
    auto sym = parse_term(lib, str);
    REQUIRE(sym.to_string() == str);
}

TEST_CASE("compare symbols", "[cxx][symbol][compare]") {
    auto a = Number(1);
    auto b = Number(2);

    REQUIRE(a == a);
    REQUIRE(a != b);
    REQUIRE(a < b);
    REQUIRE(a <= b);
    REQUIRE(b > a);
    REQUIRE(b >= a);
}

TEST_CASE("hash functions", "[cxx][symbol][hash]") {
    auto a = Number(1);
    auto b = Number(2);
    auto hasher = std::hash<Symbol>{};
    REQUIRE(hasher(a) != hasher(b));
}

} // namespace Clingo::Test

// NOLINTEND(readability-magic-numbers)
