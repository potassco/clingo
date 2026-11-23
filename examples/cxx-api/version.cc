#include <clingo/core.hh>

#include <iostream>

//! Simple example showing how to get version information.
auto main() -> int {
    auto [major, minor, revision] = Clingo::version();
    std::cout << "Hello, this is clingo version " << major << "." << minor << "." << revision << ".\n";
}
