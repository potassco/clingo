#include <clingo/core/number.hh>

#include <catch2/catch_test_macros.hpp>

// NOLINTBEGIN(readability-magic-numbers)

namespace CppClingo::Test {

TEST_CASE("number_construct") {
    Number a{1};
    Number b{2};
    Number c{"123456789012345678901234567890"};
    REQUIRE(a.as_int() == 1);
    REQUIRE(b.as_int() == 2);
    REQUIRE(a.as_string() == "1");
    REQUIRE(b.as_string() == "2");
    REQUIRE(c.as_string() == "123456789012345678901234567890");
    REQUIRE(c.as_int() == std::nullopt);
}

TEST_CASE("number_cross") {
    Number a{1};
    Number b{2};
    Number c{"123456789012345678901234567890"};

    // int + int
    REQUIRE(a + a == 2);
    REQUIRE(a + b == 3);
    REQUIRE(b + b == 4);
    // int + big
    REQUIRE((a + c).as_string() == "123456789012345678901234567891");
    // big + int
    REQUIRE((c + a).as_string() == "123456789012345678901234567891");
    // big + big
    REQUIRE((c + c).as_string() == "246913578024691357802469135780");
    // big - big = int
    REQUIRE((c - c).as_int() == 0);

    Number x = c;
    auto repr = Number::to_repr(x);
    auto res = Number::to_repr(std::move(x) + a);
    REQUIRE(res == repr);

    x = c;
    repr = Number::to_repr(x);
    res = Number::to_repr(a + std::move(x));
    REQUIRE(res == repr);

    x = c;
    Number y = c;
    repr = Number::to_repr(x);
    res = Number::to_repr(std::move(x) + std::move(y));
    REQUIRE(res == repr);
}

TEST_CASE("number_op") {
    auto big = Number{"123456789012345678901234567890"};
    // plus
    REQUIRE(Number{1} + Number(2) == 3);
    REQUIRE(big + Number{1} == Number("123456789012345678901234567891"));
    REQUIRE(Number{1} + big == Number("123456789012345678901234567891"));
    REQUIRE(big + big == Number("246913578024691357802469135780"));
    // minus
    REQUIRE(Number{1} - Number(2) == -1);
    REQUIRE(big - Number{1} == Number("123456789012345678901234567889"));
    REQUIRE(Number{1} - big == Number("-123456789012345678901234567889"));
    REQUIRE(big - big == Number("0"));
    // plus
    REQUIRE(Number{2} * Number(3) == 6);
    REQUIRE(big * Number{3} == Number("370370367037037036703703703670"));
    REQUIRE(Number{3} * big == Number("370370367037037036703703703670"));
    REQUIRE(big * big == Number("15241578753238836750495351562536198787501905199875019052100"));
    // div
    REQUIRE(Number{1} / Number(2) == 0);
    REQUIRE(Number{-1} / Number(-2) == 0);
    REQUIRE(Number{1} / Number(-2) == -1);
    REQUIRE(Number{-1} / Number(2) == -1);
    REQUIRE(Number{1} / big == 0);
    REQUIRE(Number{-1} / big == -1);
    REQUIRE(-big / (big + Number{1}) == -1);
    REQUIRE(big / (-big - Number{1}) == -1);
    REQUIRE(-big / (-big - Number{1}) == 0);
    REQUIRE(big / (big + Number{1}) == 0);
    REQUIRE_THROWS(Number{1} / Number(0));
    REQUIRE_THROWS(Number{0} / Number(0));
    // mod
    REQUIRE(Number{1} % Number(2) == 1);
    REQUIRE(Number{-1} % Number(-2) == -1);
    REQUIRE(Number{1} % Number(-2) == -1);
    REQUIRE(Number{-1} % Number(2) == 1);
    REQUIRE(Number{1} % big == 1);
    REQUIRE(Number{-1} % big == Number("123456789012345678901234567889"));
    auto z = -big % (big + Number{1});
    REQUIRE(-big % (big + Number{1}) == 1);
    REQUIRE(big % (-big - Number{1}) == -1);
    REQUIRE(-big % (-big - Number{1}) == Number("-123456789012345678901234567890"));
    REQUIRE(big % (big + Number{1}) == Number("123456789012345678901234567890"));
    REQUIRE_THROWS(Number{1} % Number(0));
    REQUIRE_THROWS(Number{0} % Number(0));
    // unary minus
    REQUIRE(-Number{std::numeric_limits<int32_t>::min()} == Number("2147483648"));
    REQUIRE(-Number{2} == -2);
    REQUIRE(-Number{-2} == 2);
    REQUIRE(-Number{big} == Number("-123456789012345678901234567890"));
    // complement
    REQUIRE(~Number{0} == (~0));
    REQUIRE(~Number{-10} == (~(-10)));
    REQUIRE(~Number{10} == (~10));
    REQUIRE(~Number{std::numeric_limits<int32_t>::min()} == std::numeric_limits<int32_t>::max());
    REQUIRE(~Number{std::numeric_limits<int32_t>::max()} == std::numeric_limits<int32_t>::min());
    REQUIRE(~Number{big} == Number("-123456789012345678901234567891"));
    REQUIRE(~(-Number{big}) == Number("123456789012345678901234567889"));
    // binary and
    REQUIRE((Number(25786) & Number(14952)) == 8232);
    REQUIRE((Number(25786) & Number(-14952)) == 17560);
    REQUIRE((Number(-25786) & Number(14952)) == 6720);
    REQUIRE((Number(big) & (Number(big) * 3)) == Number("43532284884637414222710243410"));
    REQUIRE((-Number(big) & (Number(big) * 3)) == Number("326838082152399622480993460262"));
    REQUIRE((Number(big) & (-Number(big) * 3)) == Number("79924504127708264678524324482"));
    REQUIRE((Number(big) & 25786) == Number("146"));
    REQUIRE((Number(big) & -25786) == Number("123456789012345678901234567746"));
    // binary or
    REQUIRE((Number(25786) | Number(14952)) == 32506);
    REQUIRE((Number(25786) | Number(-14952)) == -6726);
    REQUIRE((Number(-25786) | Number(14952)) == -17554);
    REQUIRE((Number(big) | (Number(big) * 3)) == Number("450294871164745301382228028150"));
    REQUIRE((-Number(big) | (Number(big) * 3)) == Number("-79924504127708264678524324482"));
    REQUIRE((Number(big) | (-Number(big) * 3)) == Number("-326838082152399622480993460262"));
    REQUIRE((Number(big) | 25786) == Number("123456789012345678901234593530"));
    REQUIRE((Number(big) | -25786) == Number("-25642"));
    // binary xor
    REQUIRE((Number(25786) ^ Number(14952)) == 24274);
    REQUIRE((Number(25786) ^ Number(-14952)) == -24286);
    REQUIRE((Number(-25786) ^ Number(14952)) == -24274);
    REQUIRE((Number(big) ^ (Number(big) * 3)) == Number("406762586280107887159517784740"));
    REQUIRE((-Number(big) ^ (Number(big) * 3)) == Number("-406762586280107887159517784744"));
    REQUIRE((Number(big) ^ (-Number(big) * 3)) == Number("-406762586280107887159517784744"));
    REQUIRE((Number(big) ^ 25786) == Number("123456789012345678901234593384"));
    REQUIRE((Number(big) ^ -25786) == Number("-123456789012345678901234593388"));
    // exponentiation
    REQUIRE(pow(Number(3), Number(7)) == 2187);
    REQUIRE(pow(Number(-3), Number(7)) == -2187);
    REQUIRE(pow(Number(3), Number(65)) == Number("10301051460877537453973547267843"));
    REQUIRE(pow(Number(1), big) == 1);
    REQUIRE(pow(big, Number(3)) ==
            Number("1881676372353657772546716040589641726257477229849409426207693797722198701224860897069000"));
    REQUIRE_THROWS(pow(big, Number(-3)));
    REQUIRE_THROWS(pow(Number(2), Number(-3)));
    // absolute value
    REQUIRE(abs(Number(3)) == 3);
    REQUIRE(abs(Number(-3)) == 3);
    REQUIRE(abs(Number(std::numeric_limits<int32_t>::min())) == Number("2147483648"));
    REQUIRE(abs(Number(std::numeric_limits<int32_t>::max())) == 2147483647);
    REQUIRE(abs(-big) == big);
    REQUIRE(abs(big) == big);
    // sign
    REQUIRE(get_sign(Number(3)) == 1);
    REQUIRE(get_sign(Number(0)) == 0);
    REQUIRE(get_sign(Number(-3)) == -1);
    REQUIRE(get_sign(big) == 1);
    REQUIRE(get_sign(-big) == -1);
}

} // namespace CppClingo::Test

// NOLINTEND(readability-magic-numbers)
