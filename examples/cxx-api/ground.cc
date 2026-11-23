#include <clingo/control.hh>
#include <clingo/ground.hh>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <tuple>

namespace {

//! Implement the external functions @inc(X) and @seq(X,Y).
class Context : public Clingo::GroundEventHandler {
  private:
    //! Report which external functions are implemented.
    auto do_callable(std::string_view name, size_t args) -> bool override {
        return (name == "inc" && args == 1) || (name == "seq" && args == 2);
    }

    //! Call the external functions.
    auto do_call([[maybe_unused]] Clingo::Location const &loc, std::string_view name, Clingo::SymbolSpan args)
        -> Clingo::SymbolVector override {
        if (name == "inc" && args.size() == 1) {
            // return the successor of the given number
            if (args.front().type() != Clingo::SymbolType::number) {
                throw std::runtime_error("inc expects a number");
            }
            auto num = args.front().number();
            if (num == std::numeric_limits<int>::max()) {
                throw std::overflow_error("inc overflows");
            }
            return {Clingo::Number(num + 1)};
        }
        if (name == "seq" && args.size() == 2) {
            // return both arguments
            return {args.begin(), args.end()};
        }
        throw std::runtime_error("unknown external function");
    }
};

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

//! Simple example showing how to ground in the background using external functions.
auto main() -> int {
    try {
        // symbols are created in a background thread, so the library has to be shared
        auto lib = Clingo::Library{Clingo::LibraryFlags::shared | Clingo::LibraryFlags::slotted};
        auto ctl = Clingo::Control{lib};
        ctl.parse_string("p(@inc(10)). q(@seq(1,2)).");

        {
            // start grounding in the background
            auto hnd = ctl.start_ground({{"base", {}}}, Context{});
            // poll whether grounding has finished (other work could be done here)
            std::cout << "grounding " << (hnd.wait(0) ? "finished" : "running") << "\n";
            // wait for grounding to finish and get the result
            auto res = hnd.get();
            std::cout << "grounding result: " << (res == Clingo::GroundResult::ok ? "ok" : "not ok") << "\n";
            // the handle is closed when it goes out of scope, which must happen before solving
        }

        // solve and print the models
        std::ignore = ctl.solve({}, ModelPrinter{});
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
