#include <clingo/control.hh>
#include <clingo/observe.hh>

#include <array>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <utility>

namespace {

//! Observer printing rules, minimize constraints, and external statements.
class Printer : public Clingo::Observer {
  private:
    void do_rule(Clingo::ProgramAtomSpan head, Clingo::ProgramLiteralSpan body, bool choice) override {
        std::cout << "rule: " << (choice ? "{" : "");
        auto const *sep = "";
        for (auto atom : head) {
            std::cout << std::exchange(sep, ";") << atom;
        }
        std::cout << (choice ? "}" : "") << " :- ";
        sep = "";
        for (auto lit : body) {
            std::cout << std::exchange(sep, ",") << lit;
        }
        std::cout << ".\n";
    }

    void do_minimize(Clingo::WeightedLiteralSpan literals, Clingo::Weight priority) override {
        std::cout << "minimize: priority=" << priority;
        for (auto const &wlit : literals) {
            std::cout << " " << wlit.weight << "*" << wlit.literal;
        }
        std::cout << "\n";
    }

    void do_external(Clingo::ProgramAtom atom, Clingo::ExternalType type) override {
        static constexpr auto names = std::array{"free", "true", "false", "release"};
        std::cout << "external: " << atom << " " << names.at(static_cast<size_t>(type)) << "\n";
    }
};

} // namespace

//! Simple example showing how to inspect the ground program.
auto main() -> int {
    try {
        auto lib = Clingo::Library{};
        auto ctl = Clingo::Control{lib};
        ctl.parse_string("{ a }. b :- a. #minimize { 1 : b }. #external c.");
        ctl.ground();

        // inspect the ground program without preprocessing
        auto printer = Printer{};
        ctl.observe(printer, false);

        // write the ground program in aspif format to the output buffer
        ctl.write_aspif(std::nullopt, Clingo::WriteAspifFlags::preamble);
        std::cout << "aspif:\n" << ctl.buffer();
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
