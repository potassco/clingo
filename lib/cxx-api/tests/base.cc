#include <clingo/base.hh>
#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

namespace Clingo::Test {

namespace {

struct Fixture {
    Library lib;
    Control ctl{lib};
    Base bse{ctl.base()};
};

} // namespace

TEST_CASE_METHOD(Fixture, "base misc", "[cxx][base][misc]") {
    ctl.parse_string("{p(1)}. p(2). q(3).");
    ctl.ground();
    REQUIRE(bse.size() == 2);
    REQUIRE(bse.contains(Function(lib, "p", {Number(1)})));
    REQUIRE(bse.contains(Function(lib, "p", {Number(2)})));
    REQUIRE(!bse.contains(Function(lib, "p", {Number(3)})));
    auto p1 = bse.get(Function(lib, "p", {Number(1)}));
    auto p2 = bse.get(Function(lib, "p", {Number(2)}));
    REQUIRE(p1.has_value());
    REQUIRE(p2.has_value());
    REQUIRE(!bse.is_fact(p1->literal()));
    REQUIRE(bse.is_fact(p2->literal()));
}

TEST_CASE_METHOD(Fixture, "base atom", "[cxx][base][atom]") {
    ctl.parse_string(R"(-r(1).
p(1).
{ p(3) }.
#external p(1..3).
q(X) :- p(X).
)");
    ctl.ground();

    REQUIRE(bse.size() == 3);

    auto fun_r = Function(lib, "r", std::array{Number(1)}, false);
    auto sig_r = fun_r.signature();
    REQUIRE(sig_r.has_value());
    REQUIRE(bse.contains(*sig_r));
    REQUIRE(!bse.contains({std::get<0>(*sig_r), std::get<1>(*sig_r)}));
    REQUIRE(bse.get(fun_r)->symbol() == fun_r);
    REQUIRE(bse.get(*sig_r)->get(fun_r)->symbol() == fun_r);

    auto fun_p = Function(lib, "p", std::array{Number(2)}, true);
    auto sig_p = fun_p.signature();
    REQUIRE(sig_p.has_value());
    REQUIRE(bse.contains(*sig_p));
    REQUIRE(bse.contains({std::get<0>(*sig_p), std::get<1>(*sig_p)}));
    REQUIRE(bse.contains(fun_p));
    REQUIRE(bse.get(fun_p)->symbol() == fun_p);

    auto bse_p = bse.get(*sig_p);
    auto bse_ps = bse.get({std::get<0>(*sig_p), std::get<1>(*sig_p)});
    REQUIRE(bse_p->size() == bse_ps->size());
    REQUIRE(bse_p->contains(fun_p));
    REQUIRE(!bse_p->contains(Function(lib, "p", std::array{Number(4)}, true)));
    REQUIRE(!bse.contains(Function(lib, "p", std::array{Number(4)}, true)));

    auto res_sig = std::vector<std::tuple<std::string_view, size_t, bool>>{};
    res_sig.reserve(bse.size());
    std::ranges::transform(bse, std::back_inserter(res_sig), [](auto const &x) { return x.first; });
    std::ranges::sort(res_sig);
    REQUIRE(std::ranges::equal(res_sig, std::array<std::tuple<std::string_view, size_t, bool>, 3>{
                                            {{"p", 1, true}, {"q", 1, true}, {"r", 1, false}}}));

    auto res_sym = std::vector<std::tuple<std::string, bool, bool>>{};
    res_sym.reserve(bse_p->size());
    std::ranges::transform(*bse_p, std::back_inserter(res_sym), [&](auto const &x) {
        return std::tuple{x.first.to_string(), bse.is_fact(x.second.literal()), bse.is_external(x.second.literal())};
    });
    std::ranges::sort(res_sym);
    REQUIRE(std::ranges::equal(res_sym, std::array<std::tuple<std::string, bool, bool>, 3>{
                                            {{"p(1)", true, false}, {"p(2)", false, true}, {"p(3)", false, false}}}));
    REQUIRE(bse_p->get(fun_p)->literal() > 0);
}

// TODO:
// - term base
// - theory base

} // namespace Clingo::Test
