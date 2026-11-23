#include <clingo/symbol.hh>

#include <array>
#include <cstdlib>
#include <exception>
#include <iostream>

//! Simple example showing how to create, print, and compare symbols.
auto main() -> int {
    try {
        auto lib = Clingo::Library{};

        // create a number, an identifier (function without arguments), and a function symbol
        auto num = Clingo::Number(42);
        auto id = Clingo::Function(lib, "x");
        auto fun = Clingo::Function(lib, "x", {num, id});

        // print the symbols along with their hash values
        for (auto const &sym : std::array{num, id, fun}) {
            std::cout << "the hash of " << sym << " is " << sym.hash() << "\n";
        }

        // equal to comparison
        for (auto const &arg : fun.arguments()) {
            std::cout << num << (num == arg ? " is equal to " : " is not equal to ") << arg << "\n";
        }

        // less than comparison
        std::cout << num << (num < id ? " is less than " : " is not less than ") << id << "\n";
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
