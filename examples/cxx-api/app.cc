#include <clingo/app.hh>
#include <clingo/control.hh>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

//! Application that overrides the default program part to ground.
class ExampleApp : public Clingo::App {
  private:
    auto do_program_name() noexcept -> std::string_view override { return "example"; }

    auto do_version() noexcept -> std::string_view override { return "1.0.0"; }

    void do_register_options(Clingo::Options options) override {
        options.add(
            "Example", "program", "Override the default program part to ground.",
            [this](std::string_view value) { program_ = value; }, false, "NAME");
    }

    void do_validate_options() override {
        // NOTE: Here compatibility checks can be performed. The example is
        // valid for all options.
    }

    void do_print_model([[maybe_unused]] Clingo::ConstModel model, Clingo::ModelPrinter const &printer) override {
        // NOTE: Here model printing can be customized. We simply use the
        // default model printer here.
        printer();
    }

    void do_main(Clingo::Control const &control, std::span<std::string_view const> files) override {
        control.parse_files(files);
        control.parts({Clingo::Part{program_}});
        control.main();
    }

    std::string program_ = "base";
};

} // namespace

//! Simple example showing how to extend the clingo application.
auto main(int argc, char const *argv[]) -> int {
    try {
        auto args = std::vector<std::string_view>{};
        for (auto const *arg : std::span{argv, static_cast<size_t>(argc)}.subspan(1)) {
            args.emplace_back(arg);
        }
        auto lib = Clingo::Library{Clingo::LibraryFlags::fast_release | Clingo::LibraryFlags::slotted};
        auto app = ExampleApp{};
        return Clingo::main(lib, args, &app);
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
