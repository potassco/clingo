#include <clingo/app.hh>
#include <clingo/control.hh>

#include "cbs.hh"
#include "tempfile.hh"

#include <iostream>
#include <string>

#define ASSERT_MSG(cond, msg)                                                                                          \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::cerr << "Assertion failed: " << msg << "\n";                                                          \
            std::cerr.flush();                                                                                         \
            std::abort();                                                                                              \
        }                                                                                                              \
    } while (0)

namespace Clingo::Test {

class AppTest : public App {
  public:
    AppTest() { events.reserve(5); }
    auto parse_test(std::string_view value) -> bool {
        events.emplace_back("parse");
        ASSERT_MSG(value == "x", "parse_test: expected value 'x', got '" << value << "'");
        return true;
    }

    auto do_program_name() noexcept -> std::string_view override { return "test"; }
    auto do_version() noexcept -> std::string_view override { return "1.2.3"; }
    void do_main(Control const &control, std::span<std::string_view const> files) override {
        events.emplace_back("main");
        control.parse_files(files);
        control.ground();
        auto mcb = MCB{models};
        std::ignore = control.solve(mcb).get();
    }
    void do_register_options(Options options) override {
        events.emplace_back("register");
        options.add("Clingo.Test", "test", "test description",
                    [this](std::string_view val) { return this->parse_test(val); });
        options.add_flag("Clingo.Test", "flag", "test description", flag);
    }

    void do_validate_options() override {
        events.emplace_back("validate");
        ASSERT_MSG(flag, "validate_options: flag must be true");
    }

    std::vector<std::vector<std::string>> models;
    std::vector<std::string> events;
    bool flag = false;
};

} // namespace Clingo::Test

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    using namespace Clingo;
    using namespace Clingo::Test;

    try {
        auto tmp = TempFile{"1 {a; b; c(1/0)}."};
        auto app = AppTest{};
        auto arg = std::array<std::string_view, 5>{tmp.path().c_str(), "--outf=3", "--test=x", "--flag", "0"};

        auto logger = [&](MessageCode code, std::string_view msg) {
            if (code == MessageCode::operation_undefined) {
                app.events.emplace_back("logger");
                ASSERT_MSG(msg.find("operation undefined") != std::string::npos,
                           "logger: expected 'operation undefined' in message, got: " << msg);
            }
        };

        auto lib = Library(LibraryFlags::none, logger);
        auto ret = Clingo::main(lib, arg, &app);

        ASSERT_MSG(ret == 30, "Expected return code 30, got " << ret);
        ASSERT_MSG(app.events.size() >= 5, "Expected at least 5 events, got " << app.events.size());
        ASSERT_MSG(app.events[0] == "register", "First event should be 'register', got '" << app.events[0] << "'");
        ASSERT_MSG(app.events[1] == "parse", "Second event should be 'parse', got '" << app.events[1] << "'");
        ASSERT_MSG(app.events[2] == "validate", "Third event should be 'validate', got '" << app.events[2] << "'");
        ASSERT_MSG(app.events[3] == "main", "Fourth event should be 'main', got '" << app.events[3] << "'");
        ASSERT_MSG(app.events[4] == "logger", "Fifth event should be 'logger', got '" << app.events[4] << "'");
        auto res = std::vector<std::vector<std::string>>{{"a"}, {"a", "b"}, {"b"}};
        ASSERT_MSG(app.models == res, "Model results do not match expected output.");

        std::cout << "All assertions passed!\n";
    } catch (std::exception const &e) {
        std::cerr << "Unexpected exception: " << e.what() << "\n";
        std::cerr.flush();
        std::abort();
    }

    return 0;
}
