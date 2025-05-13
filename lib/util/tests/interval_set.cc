#include "clingo/util/interval_set.hh"
#include "clingo/util/print.hh"

#include <catch2/catch_test_macros.hpp>

#include <sstream>

namespace CppClingo::Util::Test {

using IS = interval_set<int>;
using LB = IS::left_bound;
using RB = IS::right_bound;
using IV = IS::interval;
using S = std::string;

namespace {

// NOLINTBEGIN(modernize-use-designated-initializers,readability-magic-numbers)

auto lb(int x, bool y) -> LB {
    return {x, y};
}
auto rb(int x, bool y) -> RB {
    return {x, y};
}

auto to_str(IS const &x) -> std::string {
    auto f = [](std::ostream &out, IV const &x) {
        out << (x.left.inclusive ? "[" : "(") << x.left.bound << "," << x.right.bound
            << (x.right.inclusive ? "]" : ")");
    };
    std::ostringstream out;
    out << "{" << p_range(x, ",", f) << "}";
    return out.str();
}

} // namespace

TEST_CASE("intervals", "[base]") {
    SECTION("boundLess") {
        REQUIRE(!(lb(2, true) < lb(1, true)));
        REQUIRE(!(lb(2, true) < lb(1, false)));
        REQUIRE(!(lb(2, false) < lb(1, true)));
        REQUIRE(!(lb(2, false) < lb(1, false)));

        REQUIRE((lb(1, true) < lb(2, true)));
        REQUIRE((lb(1, true) < lb(2, false)));
        REQUIRE((lb(1, false) < lb(2, true)));
        REQUIRE((lb(1, false) < lb(2, false)));

        REQUIRE(!(lb(1, true) < lb(1, true)));
        REQUIRE((lb(1, true) < lb(1, false)));
        REQUIRE(!(lb(1, false) < lb(1, true)));
        REQUIRE(!(lb(1, false) < lb(1, false)));
        // same for RB
        REQUIRE(!(rb(2, true) < rb(1, true)));
        REQUIRE(!(rb(2, true) < rb(1, false)));
        REQUIRE(!(rb(2, false) < rb(1, true)));
        REQUIRE(!(rb(2, false) < rb(1, false)));

        REQUIRE((rb(1, true) < rb(2, true)));
        REQUIRE((rb(1, true) < rb(2, false)));
        REQUIRE((rb(1, false) < rb(2, true)));
        REQUIRE((rb(1, false) < rb(2, false)));

        REQUIRE(!(rb(1, true) < rb(1, true)));
        REQUIRE(!(rb(1, true) < rb(1, false)));
        REQUIRE((rb(1, false) < rb(1, true)));
        REQUIRE(!(rb(1, false) < rb(1, false)));
    }

    SECTION("boundLessEq") {
        REQUIRE(!(lb(2, true) <= lb(1, true)));
        REQUIRE(!(lb(2, true) <= lb(1, false)));
        REQUIRE(!(lb(2, false) <= lb(1, true)));
        REQUIRE(!(lb(2, false) <= lb(1, false)));

        REQUIRE((lb(1, true) <= lb(2, true)));
        REQUIRE((lb(1, true) <= lb(2, false)));
        REQUIRE((lb(1, false) <= lb(2, true)));
        REQUIRE((lb(1, false) <= lb(2, false)));

        REQUIRE((lb(1, true) <= lb(1, true)));
        REQUIRE((lb(1, true) <= lb(1, false)));
        REQUIRE(!(lb(1, false) <= lb(1, true)));
        REQUIRE((lb(1, false) <= lb(1, false)));
        // same for RB
        REQUIRE(!(rb(2, true) <= rb(1, true)));
        REQUIRE(!(rb(2, true) <= rb(1, false)));
        REQUIRE(!(rb(2, false) <= rb(1, true)));
        REQUIRE(!(rb(2, false) <= rb(1, false)));

        REQUIRE((rb(1, true) <= rb(2, true)));
        REQUIRE((rb(1, true) <= rb(2, false)));
        REQUIRE((rb(1, false) <= rb(2, true)));
        REQUIRE((rb(1, false) <= rb(2, false)));

        REQUIRE((rb(1, true) <= rb(1, true)));
        REQUIRE(!(rb(1, true) <= rb(1, false)));
        REQUIRE((rb(1, false) <= rb(1, true)));
        REQUIRE((rb(1, false) <= rb(1, false)));
    }

    SECTION("boundBefore") {
        // before with gap
        // {
        //  }
        REQUIRE((lb(1, true) < rb(2, true)));
        REQUIRE((lb(1, true) < rb(2, false)));
        REQUIRE((lb(1, false) < rb(2, true)));
        REQUIRE((lb(1, false) < rb(2, false)));
        // [        {           {
        // ] - gap  ) - no gap  ) - no gap
        REQUIRE((lb(1, true) < rb(1, true)));
        REQUIRE(!(lb(1, true) < rb(1, false)));
        REQUIRE(!(lb(1, false) < rb(1, true)));
        REQUIRE(!(lb(1, false) < rb(1, false)));
        //  {
        // }
        REQUIRE(!(lb(2, true) < rb(1, true)));
        REQUIRE(!(lb(2, true) < rb(1, false)));
        REQUIRE(!(lb(2, false) < rb(1, true)));
        REQUIRE(!(lb(2, false) < rb(1, false)));
        // the other way round
        // }
        //  {
        REQUIRE((rb(1, true) < lb(2, true)));
        REQUIRE((rb(1, true) < lb(2, false)));
        REQUIRE((rb(1, false) < lb(2, true)));
        REQUIRE((rb(1, false) < lb(2, false)));
        // }           ]           )
        // [ - no gap  { - no gap  ( - gap
        REQUIRE(!(rb(1, true) < lb(1, true)));
        REQUIRE(!(rb(1, true) < lb(1, false)));
        REQUIRE(!(rb(1, false) < lb(1, true)));
        REQUIRE((rb(1, false) < lb(1, false)));
        //  }
        // {
        REQUIRE(!(rb(2, true) < lb(1, true)));
        REQUIRE(!(rb(2, true) < lb(1, false)));
        REQUIRE(!(rb(2, false) < lb(1, true)));
        REQUIRE(!(rb(2, false) < lb(1, false)));
    }

    SECTION("add") {
        auto x = IS{};
        x.add({{1, true}, {1, true}});
        REQUIRE("{[1,1]}" == to_str(x));
        x.add({{3, true}, {2, true}});
        REQUIRE("{[1,1]}" == to_str(x));
        x.add({{3, true}, {4, false}});
        REQUIRE("{[1,1],[3,4)}" == to_str(x));
        x.add({{2, false}, {3, false}});
        REQUIRE("{[1,1],(2,4)}" == to_str(x));
        x.add({{1, false}, {2, false}});
        REQUIRE("{[1,2),(2,4)}" == to_str(x));
        x.add({{2, true}, {2, true}});
        REQUIRE("{[1,4)}" == to_str(x));
        x.add({{4, true}, {5, true}});
        REQUIRE("{[1,5]}" == to_str(x));
        x.add({{8, false}, {9, true}});
        REQUIRE("{[1,5],(8,9]}" == to_str(x));
        x.add({{11, false}, {12, true}});
        REQUIRE("{[1,5],(8,9],(11,12]}" == to_str(x));
        x.add({{13, false}, {14, true}});
        REQUIRE("{[1,5],(8,9],(11,12],(13,14]}" == to_str(x));
        x.add({{10, true}, {11, false}});
        REQUIRE("{[1,5],(8,9],[10,11),(11,12],(13,14]}" == to_str(x));
        x.add({{9, true}, {11, true}});
        REQUIRE("{[1,5],(8,12],(13,14]}" == to_str(x));
        x.add({{0, true}, {13, false}});
        REQUIRE("{[0,13),(13,14]}" == to_str(x));
        x.add({{-1, true}, {42, false}});
        REQUIRE("{[-1,42)}" == to_str(x));
        x.add({{42, true}, {43, true}});
        REQUIRE("{[-1,43]}" == to_str(x));
    }

    SECTION("remove") {
        auto x = IS{};
        x.add({{1, true}, {50, true}});
        REQUIRE("{[1,50]}" == to_str(x));
        x.remove({{1, true}, {2, true}});
        REQUIRE("{(2,50]}" == to_str(x));
        x.remove({{49, false}, {50, true}});
        REQUIRE("{(2,49]}" == to_str(x));
        x.remove({{5, true}, {6, false}});
        REQUIRE("{(2,5),[6,49]}" == to_str(x));
        x.remove({{8, false}, {9, true}});
        REQUIRE("{(2,5),[6,8],(9,49]}" == to_str(x));
        auto a = IS{x};
        auto b = IS{x};
        auto c = IS{x};
        a.remove({{2, false}, {13, true}});
        REQUIRE("{(13,49]}" == to_str(a));
        b.remove({{4, false}, {8, true}});
        REQUIRE("{(2,4],(9,49]}" == to_str(b));
        c.remove({{4, false}, {13, true}});
        REQUIRE("{(2,4],(13,49]}" == to_str(c));
    }

    SECTION("contains") {
        auto x = IS{};
        x.add({{5, true}, {10, false}});
        x.add({{1, true}, {4, false}});
        REQUIRE(x.contains({{5, true}, {10, false}}));
        REQUIRE(!x.contains({{5, true}, {10, true}}));
        REQUIRE(x.contains({{7, true}, {8, true}}));
        REQUIRE(x.contains({{5, false}, {6, false}}));
    }

    SECTION("intersects") {
        auto x = IS{};
        x.add({{1, true}, {4, false}});
        x.add({{5, true}, {10, false}});
        x.add({{11, false}, {12, false}});
        REQUIRE(x.intersects({{5, true}, {10, false}}));
        REQUIRE(x.intersects({{5, true}, {10, true}}));
        REQUIRE(x.intersects({{7, true}, {8, true}}));
        REQUIRE(!x.intersects({{10, true}, {11, true}}));
        REQUIRE(x.intersects({{10, true}, {12, true}}));
        REQUIRE(x.intersects({{2, true}, {7, true}}));
        REQUIRE(x.intersects({{4, true}, {7, true}}));
        REQUIRE(!x.intersects({{0, true}, {1, false}}));
        REQUIRE(x.intersects({{0, true}, {1, true}}));
        REQUIRE(!x.intersects({{12, true}, {13, true}}));
    }

    SECTION("intersect") {
        auto x = IS{};
        auto y = IS{};
        x.add({{2, true}, {8, false}});
        x.add({{9, false}, {13, true}});
        y.add({{1, true}, {3, true}});
        y.add({{4, false}, {5, true}});
        y.add({{7, true}, {10, false}});
        y.add({{11, true}, {12, false}});
        REQUIRE("{[2,3],(4,5],[7,8),(9,10),[11,12)}" == to_str(x.intersect(y)));
        REQUIRE("{[2,3],(4,5],[7,8),(9,10),[11,12)}" == to_str(y.intersect(x)));
    }

    SECTION("difference") {
        auto x = IS{};
        auto y = IS{};
        x.add({{2, true}, {8, false}});
        x.add({{9, false}, {13, true}});

        y.add({{1, true}, {3, true}});
        y.add({{4, false}, {5, true}});
        y.add({{7, true}, {10, false}});
        y.add({{11, true}, {12, false}});
        REQUIRE("{(3,4],(5,7),[10,11),[12,13]}" == to_str(x.difference(y)));
        REQUIRE("{[1,2),[8,9]}" == to_str(y.difference(x)));
    }
}

// NOLINTEND(modernize-use-designated-initializers,readability-magic-numbers)

} // namespace CppClingo::Util::Test
