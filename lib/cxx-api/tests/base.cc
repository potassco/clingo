#include <clingo/base.hh>
#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

#include <array>

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

TEST_CASE_METHOD(Fixture, "term base", "[cxx][base][term]") {
    ctl.parse_string(R"(
        p(1).
        { x; p(3) }.
        #show q(3) : x.
        #show q(X) : p(X).
    )");
    ctl.ground();

    auto base = bse.terms();
    REQUIRE(base.size() == 2);

    auto fun_q = Function(lib, "q", std::array{Number(1)});
    REQUIRE(base.contains(fun_q));
    REQUIRE(!base.contains(Function(lib, "q", std::array{Number(2)})));

    auto term_q = base.get(fun_q);
    auto term_r = base.get(Function(lib, "q", std::array{Number(3)}));
    REQUIRE(term_q->symbol() == fun_q);
    REQUIRE(term_q.has_value());
    REQUIRE(term_q->condition().size() == 1);
    REQUIRE(term_r.has_value());
    REQUIRE(term_r->condition().size() == 2);
}

TEST_CASE_METHOD(Fixture, "theory base", "[cxx][base][theory]") {
    ctl.parse_string(R"(
        #theory x {
            a {
                - : 1, unary;
                + : 2, binary, left;
                - : 3, binary, right;
                + : 4, unary
            };
            b {
                * : 1, binary, left;
                / : 2, binary, right
            };
            &p/0: a, {<,>}, b, any
        }.
        &p { +f(1,"x",[1,2],(2,3),{4,5})-y }.
    )");
    ctl.ground();

    auto base = bse.theory();
    REQUIRE(base.size() == 1);

    auto atom = base.at(0);
    REQUIRE(atom.to_string() == "&p { ((+f(1,\"x\",[1,2],(2,3),{4,5}))-y) }");
    REQUIRE(atom.name().type() == TheoryTermType::symbol);
    REQUIRE(atom.name().name() == "p");
    REQUIRE(atom.name().to_string() == "p");
    REQUIRE(!atom.guard().has_value());
    REQUIRE(atom.elements().size() == 1);

    auto elem = atom.elements().front();
    REQUIRE(elem.to_string() == "((+f(1,\"x\",[1,2],(2,3),{4,5}))-y)");
    REQUIRE(elem.tuple().size() == 1);
    REQUIRE(elem.condition().empty());
    REQUIRE(elem.condition_id() >= 0);

    auto term = elem.tuple().front();
    REQUIRE(term.to_string() == "((+f(1,\"x\",[1,2],(2,3),{4,5}))-y)");
    REQUIRE(term.type() == TheoryTermType::function);
    REQUIRE(term.name() == "-");

    auto f_term = term.arguments().front().arguments().front();
    REQUIRE(f_term.name() == "f");

    auto n = f_term.arguments()[0];
    auto s = f_term.arguments()[1];
    auto l = f_term.arguments()[2];
    auto t = f_term.arguments()[3];
    auto b = f_term.arguments()[4];

    REQUIRE(n.type() == TheoryTermType::number);
    REQUIRE(n.number() == 1);
    REQUIRE(n.to_string() == "1");

    REQUIRE(s.type() == TheoryTermType::symbol);
    REQUIRE(s.name() == "\"x\"");
    REQUIRE(s.to_string() == "\"x\"");

    REQUIRE(l.type() == TheoryTermType::list);
    REQUIRE(l.arguments().size() == 2);
    REQUIRE(l.to_string() == "[1,2]");

    REQUIRE(t.type() == TheoryTermType::tuple);
    REQUIRE(t.arguments().size() == 2);
    REQUIRE(t.to_string() == "(2,3)");

    REQUIRE(b.type() == TheoryTermType::set);
    REQUIRE(b.arguments().size() == 2);
    REQUIRE(b.to_string() == "{4,5}");

    std::ignore = ctl.solve().get();
    ctl.parse_string("#program x. &p {} < q(x).");
    ctl.ground({{"x", {}}});

    base = bse.theory();
    REQUIRE(base.size() == 1);

    atom = base.at(0);
    REQUIRE(atom.to_string() == "&p { } < q(x)");

    REQUIRE(atom.guard().has_value());
    auto [op, te] = *atom.guard();
    REQUIRE(op == "<");
    REQUIRE(te.to_string() == "q(x)");
}

} // namespace Clingo::Test
