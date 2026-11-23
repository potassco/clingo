#include <clingo/config.hh>
#include <clingo/control.hh>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <tuple>

namespace {

//! Print the models found during solving.
class ModelPrinter : public Clingo::SolveEventHandler {
  private:
    auto do_model(Clingo::Model model) -> bool override {
        std::cout << "Model:";
        for (auto const &sym : model.symbols()) {
            std::cout << " " << sym;
        }
        std::cout << "\n";
        return true;
    }
};

} // namespace

//! Simple example showing how to inspect and modify the solver configuration.
auto main() -> int {
    try {
        auto lib = Clingo::Library{};
        auto ctl = Clingo::Control{lib};

        // print the solve options
        auto cfg = ctl.config();
        std::cout << "solve options:\n" << cfg["solve"].to_string() << "\n";

        // look up the number of models to compute
        auto models = cfg["solve"]["models"];
        std::cout << "description: " << models.description() << "\n";
        if (auto value = models.value(); value) {
            std::cout << "value: " << *value << "\n";
        }

        // enumerate all models
        models.value("0");

        // ground and solve
        ctl.parse_string("1 {a; b} 1.");
        ctl.ground();
        std::ignore = ctl.solve({}, ModelPrinter{});
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
