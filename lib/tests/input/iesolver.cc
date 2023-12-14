#include <input/iesolver.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

namespace {}

TEST_CASE("iesolver") {
    auto log = Logger{};
    log.set_level(LogLevel::trace);
    auto &store = default_store();
    auto solver = IESolver{};
    auto a = IE{{{Number{3}, store.string("x")}}, Number{5}};
    auto b = IE{{{Number{-5}, store.string("x")}}, Number{-17}};
    solver.add(std::move(a));
    solver.add(std::move(b));
    solver.compute(log);
};

} // namespace Gringo::Input::Test
