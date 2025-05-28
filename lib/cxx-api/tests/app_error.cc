#include <clingo/app.hh>

#include "tempfile.hh"

#include <utility>
#include <array>

namespace Clingo::Test {

namespace {

class ErrorApp : public App {
  public:
    explicit ErrorApp(std::string mode) : mode_(std::move(mode)) {}

    auto parse_option([[maybe_unused]] std::string_view value) -> bool {
        assert(value.size() >= 0);
        if (mode_.find('o') != std::string::npos) {
            throw std::runtime_error("option");
        }
        return true;
    }

    void do_validate_options() override {
        if (mode_.find('v') != std::string::npos) {
            throw std::runtime_error("validate");
        }
        if (mode_.find('i') != std::string::npos) {
            throw std::invalid_argument("invalid");
        }
    }

    void do_register_options(Options options) override {
        if (mode_.find('r') != std::string::npos) {
            throw std::runtime_error("register");
        }
        options.add("Test", "test", "Test option.",
                    [this](std::string_view value) { return this->parse_option(value); });
    }

    void do_print_model([[maybe_unused]] ConstModel model, const ModelPrinter &printer) override {
        if (mode_.find('p') != std::string::npos) {
            throw std::runtime_error("print");
        }
        printer();
    }

    void do_main(const Control &control, std::span<const std::string_view> files) override {
        if (mode_.find('m') != std::string::npos) {
            throw std::runtime_error("main");
        }
        control.parse_files(files);
        control.main();
    }

  private:
    std::string mode_;
};

} // namespace

} // namespace Clingo::Test

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    using namespace Clingo;
    auto args = std::span{argv, static_cast<size_t>(argc)};
    try {
        if (args.size() != 2) {
            throw std::invalid_argument{"Exactly one argument expected."};
        }
        auto *str = args[1];
        constexpr std::array<std::string_view, 6> allowed = {"i", "m", "p", "r", "o", "v"};
        if (std::ranges::none_of(allowed, [&](std::string_view val) { return str == val; })) {
            throw std::invalid_argument{"One of 'i', 'm', 'p', 'r', 'o', 'v' expected."};
        }
        auto lib = Library{};
        auto app = Test::ErrorApp{str};
        auto tmp = Test::TempFile{"a."};
        main(lib, {"--test", "value", tmp.path().string()}, &app);
    } catch (std::exception const &e) {
        printf("ERROR: %s\n", e.what());
    }
    return 0;
}
