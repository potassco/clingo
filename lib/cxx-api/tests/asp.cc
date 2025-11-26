#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

#include "cbs.hh"
#include "lp.hh"

#include <filesystem>
#include <fstream>

namespace Clingo::Test {

void run_file(std::string_view prg) {
    auto data = LPParser{prg}.parse();
    // NOTE: the solutions in the json data are sorted by symbol
    for (auto &solution : data.solutions) {
        std::ranges::sort(solution);
    }
    std::ranges::sort(data.solutions);
    auto lib = Library();
    auto ctl = Control{lib, std::vector<std::string_view>{data.options.begin(), data.options.end()}};
    ctl.parse_string(prg);
    ctl.ground();
    std::vector<std::vector<std::string>> models;
    REQUIRE(!ctl.solve({}, MCB{models}).interrupted());
    REQUIRE(models == data.solutions);
}

TEST_CASE("asp", "[cxx][asp]") {
    auto dir = std::filesystem::current_path() / "lib" / "python-api" / "tests" / "resources";
    for (auto const &entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lp") {
            auto ifs = std::ifstream(entry.path().c_str());
            auto cnt = std::string{(std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>()};
            run_file(cnt);
        }
    }
}

} // namespace Clingo::Test
