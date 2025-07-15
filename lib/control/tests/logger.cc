#include <clingo/control/grounder.hh>

#include <clingo/output/text.hh>

#include <clingo/util/algorithm.hh>

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Test {

TEST_CASE("logger_test") {
    auto store = make_symbol_store(true, false);
    using V = std::vector<std::string>;
    auto ground = [&](char const *str) {
        auto msgs = V{};
        auto opts = Input::RewriteOptions{};
        auto log = Logger{[&msgs]([[maybe_unused]] MessageCode code, std::string_view msg) { msgs.emplace_back(msg); }};
        log.set_level(LogLevel::info);
        auto buf = Util::OutputBuffer{};
        auto out = Output::make_text_output(buf);
        Control::Grounder grd{log, *store, opts, *out, false};
        grd.parse(str);
        auto params = Input::ProgramParamVec{{store->string("base"), {}}};
        REQUIRE(grd.ground(params));
        return msgs;
    };
    SECTION("term") {
        REQUIRE(ground("p(a). q(X*X) :- p(X).") == V{"<string>:1:9-10: info: number expected (got a)"});
        REQUIRE(ground("p(a). q(|X|) :- p(X).") == V{"<string>:1:10-11: info: number expected (got a)"});
        REQUIRE(ground("p(()). q(-X) :- p(X).") == V{"<string>:1:11-12: info: number or function expected (got ())"});
        REQUIRE(ground("p(a). q(~X) :- p(X).") == V{"<string>:1:10-11: info: number expected (got a)"});
        REQUIRE(ground("p(a). q(X+1) :- p(X).") == V{"<string>:1:9-10: info: number expected (got a)"});
    }
    SECTION("aggregate") {
        // head
        REQUIRE(ground("p(a). #sum { X: q(X): p(X) } >= 1.") ==
                V{"<string>:1:14-15: info: non-negative number expected (got a)"});
        REQUIRE(ground("p(a). #count { : q(X): p(X) } >= 1.").empty());
        REQUIRE(ground("p(a). #min { X : q(X): p(X) } >= 1.").empty());
        // body
        REQUIRE(ground("p(a). x :- #sum { X: p(X) } >= 1.") ==
                V{"<string>:1:19-20: info: non-negative number expected (got a)"});
        REQUIRE(ground("p(a). x :- #count { : p(X) } >= 1.").empty());
        REQUIRE(ground("p(a). x :- #min { X : p(X) } >= 1.").empty());
        // assign
        REQUIRE(ground("p(a). x(Y) :- Y = #sum { X: p(X) }.") ==
                V{"<string>:1:26-27: info: non-negative number expected (got a)"});
        REQUIRE(ground("p(a). x(Y) :- Y = #count { : p(X) }.").empty());
        REQUIRE(ground("p(a). x(Y) :- Y = #min { X : p(X) }.").empty());
    }
    SECTION("stm") {
        REQUIRE(ground("p(1). #heuristic p(X). [1@2,snarf]") ==
                V{"<string>:1:29-35: info: unexpected heuristic modifier (got snarf)"});
        REQUIRE(ground("p(1). #heuristic p(X). [foo@2,true]") ==
                V{"<string>:1:25-29: info: number expected (got foo)"});
        REQUIRE(ground("p(1). #heuristic p(X). [1@bar,true]") ==
                V{"<string>:1:27-31: info: number expected (got bar)"});
        REQUIRE(ground("#external a. [snarf]") == V{"<string>:1:15-21: info: unexpected external type (got snarf)"});
        REQUIRE(ground("p(a). #minimize { X: p(X) }.") == V{"<string>:1:19-20: info: number expected (a)"});
        REQUIRE(ground("p(a). #minimize { 1@X: p(X) }.") == V{"<string>:1:21-22: info: number expected (a)"});
    }
    // Note: the current implementation takes two iterations to clean up everything
    auto res = store->gc();
    REQUIRE(std::get<0>(res) == 0); // no owners
    res = store->gc();
    REQUIRE(std::get<0>(res) == 0); // no owners
    REQUIRE(std::get<1>(res) == 0); // no symbols
}

} // namespace CppClingo::Test
