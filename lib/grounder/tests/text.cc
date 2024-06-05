#include <gringo/grounder/grounder.hh>

#include <gringo/output/text.hh>

#include <gringo/util/algorithm.hh>

#include <catch2/catch_test_macros.hpp>

#include <sstream>

namespace Gringo::Test {

TEST_CASE("grounder_text") {
    std::ostringstream oss;
    auto opts = Input::RewriteOptions{};
    auto log = Gringo::Logger{};
    log.set_level(LogLevel::error);
    auto store = Gringo::make_symbol_store(true, false);
    auto out = Gringo::Output::make_text_output(oss);
    Gringo::Grounder grd{log, *store, opts, *out};
    auto params = Input::ProgramParamVec{{store->string_ref("base"), {}}};

    SECTION("fact") {
        grd.parse("a.");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(oss.str() == "a.\n");
    }
    SECTION("basic") {
        grd.parse("a. b :- a.");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(oss.str() == "a.\n"
                             "b.\n");
    }
    SECTION("normal") {
        grd.parse("a :- not b. b :- not a.");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(oss.str() == "b :- not a.\n"
                             "a :- not b.\n");
    }
    SECTION("condlit_strat") {
        grd.parse("a :- not b. b :- not a. c :- a : b.");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(oss.str() == "b :- not a.\n"
                             "a :- not b.\n"
                             "c :- #false: b, not a.\n");
    }
    SECTION("condlit_rec") {
        grd.parse("b :- b : a. a :- a : b.");
        grd.prepare();
        REQUIRE(grd.ground(params));
        REQUIRE(oss.str() == "b :- b: a.\n"
                             "a :- a: b.\n");
    }
}

} // namespace Gringo::Test
