#include <clingo/ast.hh>
#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

namespace Clingo::Test {

auto ctx(std::string_view name, SymbolSpan params) -> SymbolVector {
    REQUIRE(params.size() == 1);
    REQUIRE(params[0].type() == SymbolType::number);
    if (name == "f") {
        return SymbolVector{Number(params[0].number() + 1)};
    }
    if (name == "g") {
        return SymbolVector{Number(params[0].number() - 1)};
    }
    return SymbolVector{};
}

TEST_CASE("control solve", "[cxx][control][solve]") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    ctl.parse_string("{a(@f(1)); b(@g(1))}.");
    ctl.ground(std::nullopt, ctx);
    auto hnd = ctl.solve();
    auto mdls = std::vector<std::string>{};
    for (auto mdl : hnd) {
        auto oss = std::ostringstream{};
        auto syms = mdl.symbols();
        std::ranges::sort(syms);
        for (auto const &sym : syms) {
            oss << sym << " ";
        }
        mdls.emplace_back(oss.str());
        if (!mdls.back().empty()) {
            mdls.back().pop_back();
        }
    }
    std::ranges::sort(mdls);
    REQUIRE(mdls == std::vector<std::string>{"", "a(2)", "a(2) b(0)", "b(0)"});
    REQUIRE(hnd.get().satisfiable());
}

TEST_CASE("control ground", "[cxx][control][ground]") {
    auto lib = Library{};
    auto ctl = Control{lib, {"--mode=ground"}};

    ctl.parse_string("a.");
    ctl.ground({{"base", {}}});
    ctl.parse_string("#program acid(k). b(k).");
    auto parts = PartVector{};
    for (int i = 0; i < 5; ++i) {
        parts.push_back({"acid", {Number(i)}});
    }
    ctl.ground(parts);

    auto prg = AST::Program{lib};
    prg.add(AST::parse(lib, "#program parse."));
    prg.add(AST::parse(lib, "c :- a."));
    ctl.join(prg);
    ctl.ground({{"parse", {}}});
    constexpr auto res = R"(a.
#show a/0.
#show.
b(0).
b(1).
b(2).
b(3).
b(4).
#show b/1.
c.
#show c/0.
)";
    REQUIRE(ctl.buffer() == res);
}

TEST_CASE("control ground", "[cxx][control][ground][context]") {
    auto lib = Library{};
    auto ctl = Control{lib, {"--mode=ground"}};

    auto ctx = [&](std::string_view name, SymbolSpan args) -> SymbolVector {
        auto res = SymbolVector{};
        if (name == "fun" && args.size() == 1) {
            return {args.front(), Number(args.front().number() + 1)};
        }
        if (name == "gun" && args.size() == 1) {
            return {Number(args.front().number() + 1)};
        }
        return res;
    };

    ctl.parse_string("p(@fun(1)).");
    ctl.parse_string("q(@gun(2)).");
    ctl.parse_string("#show.");
    ctl.ground({{"base", {}}}, ctx);
    constexpr auto res = R"(p(1).
p(2).
q(3).
#show.
)";
    REQUIRE(ctl.buffer() == res);
}

TEST_CASE("control incmode", "[cxx][control][incmode]") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    constexpr auto prg = R"(#include <incmode>.

#program base.

{a;b;c}.

#program step(k).

{ c(k) }.
q(k) :- c(k).

#program check(k).

:- not c(3), query(k).
)";
    ctl.parse_string(prg);
    ctl.main();
    auto c3 = Function(lib, "c", {Number(3)});
    {
        size_t n = 0;
        auto hnd = ctl.solve();
        for (auto &&mdl : hnd) {
            REQUIRE(mdl.contains(c3));
            ++n;
        }
        REQUIRE(n == 32);
    }
}

} // namespace Clingo::Test
