#include <clingo/script.hh>

#include <catch2/catch_test_macros.hpp>

#include "cbs.hh"

namespace Clingo::Test {

namespace {

class TestScript : public Script {
  public:
    void do_execute(std::string_view code) override {
        REQUIRE(code.find("needle") != std::string_view::npos);
        executed = true;
    }
    auto do_call([[maybe_unused]] Library &lib, std::string_view name, SymbolSpan arguments) -> SymbolVector override {
        REQUIRE(name == "fun");
        REQUIRE(arguments.size() == 1);
        return {Number(arguments.front().number() + 1)};
    }
    auto do_callable(std::string_view name, size_t arguments) -> bool override {
        return (name == "main" && arguments == 0) || (name == "fun" && arguments == 1);
    }
    void do_main(Library &lib, const Control &ctl) override {
        ctl.parts({
            {"one", {Number(1)}},
            {"ext", {Number(lib, "1923841239")}},
        });
        ctl.main();
    }
    auto do_name() -> std::string_view override { return "test"; }
    auto do_version() -> std::string_view override { return "1.2.3"; }

    bool executed = false;
};

struct Fixture {
    Library lib;
    TestScript *script = &register_script(lib, std::make_unique<TestScript>());
    MV models;
};

} // namespace

TEST_CASE_METHOD(Fixture, "script ground", "[cxx][script][ground]") {
    auto ctl = Control(lib, {"--convert=text"});
    ctl.parse_string(R"(#script(test)
There is a needle in this sentence!
#end.

#program one(k).
p(k).
#program ext(k).
p(@fun(k)).
)");
    ctl.main();
    REQUIRE(script->executed);
    REQUIRE(ctl.buffer() == R"(p(1).
p(1923841240).
#show p/1.
#show.
)");
}

TEST_CASE_METHOD(Fixture, "script solve", "[cxx][script][solve]") {
    auto ctl = Control(lib);
    ctl.parse_string(R"(#program one(k).
p(k).
#program ext(k).
p(@fun(k)).
)");

    ctl.ground({{"one", {Number(1)}}});
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == MV{{"p(1)"}});

    ctl.ground({{"ext", {Number(2)}}});
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == MV{{"p(1)", "p(3)"}});

    REQUIRE(!script->executed);
}

} // namespace Clingo::Test
