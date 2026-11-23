#include <clingo/control.hh>
#include <clingo/solve.hh>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string_view>

namespace {

//! Print the given model.
void print_model(std::string_view prefix, Clingo::ConstModel const &model) {
    std::cout << prefix << ":";
    for (auto const &sym : model.symbols()) {
        std::cout << " " << sym;
    }
    std::cout << "\n";
}

//! Print the models found during solving.
class ModelPrinter : public Clingo::SolveEventHandler {
  private:
    auto do_model(Clingo::Model model) -> bool override {
        print_model("async model", model);
        return true;
    }
};

//! Iterate over the models of the grounded program.
void solve_yield(Clingo::Control const &ctl) {
    auto hnd = ctl.start_solve({}, Clingo::SolveFlags::yield);
    for (auto const &model : hnd) {
        print_model("yield model", model);
    }
    std::cout << "yield result: " << hnd.get().to_string() << "\n";
}

//! Solve the grounded program in the background.
void solve_async(Clingo::Control const &ctl) {
    auto hnd = ctl.start_solve({}, Clingo::SolveFlags::async, ModelPrinter{});
    // other work could be done here while polling the search
    while (!hnd.wait(0.01)) {
    }
    std::cout << "async result: " << hnd.get().to_string() << "\n";
}

} // namespace

//! Simple example showing how to solve iteratively and asynchronously.
auto main() -> int {
    try {
        // models are printed from a background thread, so the library has to be shared
        auto lib = Clingo::Library{Clingo::LibraryFlags::shared | Clingo::LibraryFlags::slotted};
        auto ctl = Clingo::Control{lib, {"0"}};
        ctl.parse_string("1 {a; b} 1.");
        ctl.ground();
        solve_yield(ctl);
        solve_async(ctl);
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
