#include <clingo/base.hh>
#include <clingo/control.hh>

#include <cstdlib>
#include <exception>
#include <iostream>

//! Simple example showing how to inspect the theory atoms of a grounded program.
auto main() -> int {
    try {
        auto lib = Clingo::Library{};
        auto ctl = Clingo::Control{lib};
        ctl.parse_string(R"(
            #theory t {
              term { + : 1, binary, left };
              &a/0 : term, any;
              &b/1 : term, {=}, term, any
            }.
            { x; y }.
            &a { 1+2: x }.
            &b(3) { 4 } = 17 :- y.
        )");
        ctl.ground();

        // iterate over the grounded theory atoms
        auto theory = ctl.base().theory();
        std::cout << "number of grounded theory atoms: " << theory.size() << "\n";
        for (auto const &atom : theory) {
            // print the whole atom and its program literal
            std::cout << "theory atom: " << atom.to_string() << "\n";
            std::cout << "  literal: " << atom.literal() << "\n";
            // print the elements of the atom
            for (auto const &elem : atom.elements()) {
                std::cout << "  element: " << elem.to_string() << "\n";
            }
            // print the guard of the atom
            if (auto guard = atom.guard(); guard) {
                std::cout << "  guard: " << guard->first << " " << guard->second.to_string() << "\n";
            }
        }
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
