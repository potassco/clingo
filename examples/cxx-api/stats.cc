#include <clingo/control.hh>
#include <clingo/solve.hh>
#include <clingo/stats.hh>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <tuple>

namespace {

//! Print models and add user statistics after solving.
class Handler : public Clingo::SolveEventHandler {
  private:
    auto do_model(Clingo::Model model) -> bool override {
        std::cout << "Model:";
        for (auto const &sym : model.symbols()) {
            std::cout << " " << sym;
        }
        std::cout << "\n";
        return true;
    }

    //! Step statistics are reset for each solve call while accumulated
    //! statistics are kept and can be updated.
    void do_stats(Clingo::Stats step, Clingo::Stats accu) override {
        // set a step statistic
        step.map().insert("example", Clingo::StatsType::value).value(21);
        // increment an accumulated statistic (insert returns an existing entry)
        auto example = accu.map().insert("example", Clingo::StatsType::value);
        example.value(example.value() + 21);
    }
};

} // namespace

//! Simple example showing how to add and inspect statistics.
auto main() -> int {
    try {
        // without option --stats only basic statistics are gathered
        auto lib = Clingo::Library{};
        auto ctl = Clingo::Control{lib, {"0", "--stats"}};
        ctl.parse_string("1 {a; b} 1.");
        ctl.ground();
        std::ignore = ctl.solve({}, Handler{});

        // look up a single value
        auto stats = ctl.stats();
        std::cout << "enumerated models: " << stats["summary"]["models"]["enumerated"].value() << "\n";

        // print all statistics
        std::cout << stats.to_string() << "\n";
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
