#include <clingo/app.hh>
#include <utility>

namespace Clingo::Test {

namespace {

class ErrorApp : public App {
  public:
    explicit ErrorApp(std::string mode) : mode_(std::move(mode)) {}

    auto parse_option(std::string_view value) -> bool {
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
        if (mode_.find('V') != std::string::npos) {
            throw std::invalid_argument("Validate");
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
        assert(files.size() == 0);
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

/*

add_test(NAME test_cxx-app-error-main COMMAND test_cxx-app-error main)
add_test(NAME test_cxx-app-error-validate COMMAND test_cxx-app-error validate)

set_tests_properties(my_app_test PROPERTIES PASS_REGULAR_EXPRESSION "Expected output")

def run_app_test(self, mode, pattern: str):
    output = subprocess.run(
        [sys.executable, __file__, "test-error-app", mode],
        capture_output=True,
        text=True,
        check=False,
        timeout=10,
    ).stderr
    return bool(re.search(pattern, output, re.DOTALL))

@pytest.mark.parametrize(
    "mode",
    [
        "main",
        "validate",
        "register",
        "print",
        "option",
    ],
)
def test_error_app(self, mode):
    msg = f"mode `{mode}` failed"
    assert self.run_app_test(mode[0], f"RuntimeError: {mode}"), msg

def test_error_validate(self):
    assert self.run_app_test("V", re.escape("*** ERROR: (test): Validate"))

def error_app_main(mode: str):
    with Library() as lib:
        clingo_main(lib, ["--test", "value"], ErrorApp(mode))
*/

} // namespace Clingo::Test

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    using namespace Clingo;
    auto args = std::span{argv, static_cast<size_t>(argc)};
    try {
        if (args.size() != 2) {
            throw std::invalid_argument{"Exactly one argument expected."};
        }
        auto lib = Library{};
        auto app = Test::ErrorApp{args[1]};
        main(lib, {"--test", "value"}, &app);
    } catch (std::exception const &e) {
        printf("ERROR: %s\n", e.what());
    }
}
