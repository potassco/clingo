#include <clingo/control.hh>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <span>
#include <string_view>
#include <tuple>
#include <vector>

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

//! Simple example showing how to ground and solve a logic program.
//!
//! The command line arguments are passed on to the control object.
auto main(int argc, char const *argv[]) -> int {
    try {
        auto args = std::vector<std::string_view>{};
        for (auto const *arg : std::span{argv, static_cast<size_t>(argc)}.subspan(1)) {
            args.emplace_back(arg);
        }

        // create a library and a control object
        auto lib = Clingo::Library{};
        auto ctl = Clingo::Control{lib, args};

        // parse and ground the program
        ctl.parse_string("1 {a; b} 1.");
        ctl.ground();

        // solve printing models with the event handler
        std::ignore = ctl.solve({}, ModelPrinter{});
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
