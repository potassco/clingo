#include <clingo/app.hh>
#include <clingo/control.hh>
#include <clingo/theory.hh>

#include <catch2/catch_test_macros.hpp>

#include "cbs.hh"
#include "tempfile.hh"

#include <array>

namespace Clingo::Test {

class AppTest : public App {
  public:
    AppTest() { events.reserve(5); }
    auto parse_test(std::string_view value) -> bool {
        events.emplace_back("parse");
        REQUIRE(value == "x");
        return true;
    }

    auto do_program_name() noexcept -> std::string_view override { return "test"; }
    auto do_version() noexcept -> std::string_view override { return "1.2.3"; }
    void do_main(Control const &control, std::span<std::string_view const> files) override {
        events.emplace_back("main");
        control.parse_files(files);
        control.ground();
        std::ignore = control.solve(MCB{models}).get();
    }
    void do_register_options(Options options) override {
        events.emplace_back("register");
        options.add("Clingo.Test", "test", "test description",
                    [this](std::string_view val) { return this->parse_test(val); });
        options.add_flag("Clingo.Test", "flag", "test description", flag);
    }

    void do_validate_options() override {
        events.emplace_back("validate");
        REQUIRE(flag);
    }

    MV models;
    std::vector<std::string> events;
    bool flag = false;
};

TEST_CASE("app", "[cxx][app]") {
    auto tmp = TempFile{"1 {a; b; c(1/0)}."};
    auto app = AppTest{};
    auto str = tmp.path().string();
    auto arg = std::array<std::string_view, 5>({str, "--outf=3", "--test=x", "--flag", "0"});

    auto logger = [&](MessageCode code, std::string_view msg) {
        if (code == MessageCode::operation_undefined) {
            app.events.emplace_back("logger");
            REQUIRE(msg.find("operation undefined") != std::string::npos);
        }
    };

    auto lib = Library{LibraryFlags::none, logger};
    auto ret = Clingo::main(lib, arg, &app);

    REQUIRE(ret == 30);
    REQUIRE(app.events == std::vector<std::string>{"register", "parse", "validate", "main", "logger"});
    REQUIRE(app.models == MV{{"a"}, {"a", "b"}, {"b"}});
}

} // namespace Clingo::Test
