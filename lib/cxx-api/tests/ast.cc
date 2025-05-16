#include <clingo/ast.hh>

#include <catch2/catch_test_macros.hpp>

namespace Clingo::Test {

namespace {

struct Fixture {
    using N = AST::Node;
    using T = AST::NodeType;
    using A = AST::Attribute;

    Fixture() = default;

    template <T Type, class... As> auto node(As &&...args) { return N::create<Type>(lib, std::forward<As>(args)...); }

    [[nodiscard]] auto parse_sym(const std::string_view val) const -> Symbol { return Clingo::parse_term(lib, val); }

    [[nodiscard]] auto parse_term(const std::string_view val) const -> N {
        return AST::parse(lib, val, AST::ParseType::term);
    }

    [[nodiscard]] auto parse_lit(const std::string_view val) const -> N {
        return AST::parse(lib, val, AST::ParseType::literal);
    }

    Library lib;
    Location loc{Position{lib, "<a>", 1, 2}, Position{lib, "<b>", 3, 4}};
};

} // namespace

TEST_CASE("ast misc", "[cxx][ast][misc]") {
    using T = AST::NodeType;
    using A = AST::Attribute;

    auto lib = Library{};
    auto stm = AST::parse(lib, "a :- b.");
    auto head = stm.node(A::head);
    auto body = stm.nodes(A::body);
    REQUIRE(stm.to_string() == "a :- b.");
    REQUIRE(head.to_string() == "a");
    REQUIRE(body.size() == 1);
    REQUIRE(body.front().to_string() == "b");

    auto loc = head.location(A::location);
    auto var_x = AST::Node::create<T::term_variable>(lib, loc, "X", false);
    REQUIRE(var_x.to_string() == "X");
    auto var_y = var_x.update<T::term_variable>(lib, []<A attr>() {
        if constexpr (attr == A::name) {
            return std::string_view{"Y"};
        }
    });
    REQUIRE(var_y.to_string() == "Y");

    auto trail = std::vector<std::string>{};
    AST::Visitor visit = [&](AST::Node const &node) {
        trail.emplace_back(node.to_string());
        node.accept(visit);
    };
    visit(stm);
    REQUIRE(trail == std::vector<std::string>{"a :- b.", "a", "a", "a", "b", "b", "b"});

    AST::Transformer trans = [&](AST::Node const &node) -> std::optional<AST::Node> {
        if (node.type() == T::term_variable) {
            return node.update<T::term_variable>(lib, [&]<A attr>() {
                if constexpr (attr == A::name) {
                    return node.string(attr) == "X" ? "Y" : "Z";
                }
            });
        }
        return node.accept(lib, trans);
    };
    auto var_z = trans(var_y);
    REQUIRE(var_z.has_value());
    REQUIRE(var_z->to_string() == "Z");

    auto stm_xy = AST::parse(lib, "a(X) :- b(Y).");
    REQUIRE(stm_xy.to_string() == "a(X) :- b(Y).");
    auto stm_yz = trans(stm_xy);
    REQUIRE(stm_yz.has_value());
    REQUIRE(stm_yz->to_string() == "a(Y) :- b(Z).");
}

// NOTE: better in core
TEST_CASE_METHOD(Fixture, "ast location", "[cxx][ast][misc]") {
    REQUIRE(loc.begin().file() == "<a>");
    REQUIRE(loc.begin().line() == 1);
    REQUIRE(loc.begin().column() == 2);
    REQUIRE(loc.end().file() == "<b>");
    REQUIRE(loc.end().line() == 3);
    REQUIRE(loc.end().column() == 4);

    REQUIRE(loc.begin() == loc.begin());
    REQUIRE(loc.begin() != loc.end());
    REQUIRE(loc.begin() < loc.end());

    REQUIRE(loc.to_string() == "<a>:1:2-<b>:3:4");
    REQUIRE(loc.begin().to_string() == "<a>:1:2");
    REQUIRE(loc.end().to_string() == "<b>:3:4");
}

TEST_CASE_METHOD(Fixture, "ast projection", "[cxx][ast][projection]") {
    auto p = node<T::projection>(loc);
    REQUIRE((p.location(A::location) == loc));
    REQUIRE(p.to_string() == "*");
}

TEST_CASE_METHOD(Fixture, "ast variable", "[cxx][ast][variable]") {
    auto x = node<T::term_variable>(loc, "X", false);
    auto a = node<T::term_variable>(loc, "_", true);

    REQUIRE((x.location(A::location) == loc));
    REQUIRE(x.string(A::name) == "X");
    REQUIRE_FALSE(x.number(A::anonymous) == 1);
    REQUIRE((a.string(A::name) == "_"));
    REQUIRE_FALSE(a.number(A::anonymous) == 0);

    REQUIRE(x.to_string() == "X");
    REQUIRE(a.to_string() == "_");
}

TEST_CASE_METHOD(Fixture, "ast term symbolic", "[cxx][ast][term_symbolic]") {
    auto s = parse_sym("f(1,2)");
    auto p = node<T::term_symbolic>(loc, s);

    REQUIRE((p.location(A::location) == loc));
    REQUIRE(p.symbol(A::symbol) == s);
    REQUIRE(p.to_string() == "f(1,2)");
}

TEST_CASE_METHOD(Fixture, "ast term absolute", "[cxx][ast][term_absolute]") {
    auto p = node<T::term_absolute>(loc, std::array{parse_term("1"), parse_term("-2")});

    REQUIRE((p.location(A::location) == loc));
    REQUIRE(p.nodes(A::pool).size() == 2);
    REQUIRE(p.to_string() == "|1;-2|");
}

TEST_CASE_METHOD(Fixture, "ast term unary", "[cxx][ast][term_unary]") {
    auto p = node<T::term_unary_operation>(loc, AST::UnaryOperator::minus, parse_term("-2"));
    REQUIRE((p.location(A::location) == loc));
    REQUIRE(p.number(A::operator_type) == AST::UnaryOperator::minus);
    REQUIRE(p.node(A::right) == parse_term("-2"));
    REQUIRE(p.to_string() == "-(-2)");
}

TEST_CASE_METHOD(Fixture, "ast term binary", "[cxx][ast][term_binary]") {
    auto p = node<T::term_binary_operation>(loc, parse_term("1"), AST::BinaryOperator::plus, parse_term("-2"));
    REQUIRE((p.location(A::location) == loc));
    REQUIRE(p.node(A::left).to_string() == "1");
    REQUIRE(p.number(A::operator_type) == AST::BinaryOperator::plus);
    REQUIRE(p.node(A::right).to_string() == "-2");
    REQUIRE(p.to_string() == "1+(-2)");
}

TEST_CASE_METHOD(Fixture, "ast term tuple", "[cxx][ast][term_tuple]") {
    auto a = std::array{node<T::argument_tuple>(std::array{parse_term("1"), parse_term("2")}), parse_term("3")};
    auto p = node<T::term_tuple>(loc, a);
    REQUIRE((p.location(A::location) == loc));
    REQUIRE(std::ranges::equal(p.nodes(A::pool), a));
    REQUIRE(p.to_string() == "(1,2;3)");
}

TEST_CASE_METHOD(Fixture, "ast term function", "[cxx][ast][term_tuple]") {
    auto a = std::array{node<T::argument_tuple>(std::vector{parse_term("1"), parse_term("2")}),
                        node<T::argument_tuple>(std::vector{parse_term("3"), node<T::projection>(loc)})};
    auto p = node<T::term_function>(loc, "f", a, true);
    auto q = node<T::term_function>(loc, "f", std::array<N, 0>{}, false);
    REQUIRE((p.location(A::location) == loc));
    REQUIRE(p.string(A::name) == "f");
    REQUIRE(std::ranges::equal(p.nodes(A::pool), a));
    REQUIRE(p.number(A::external));
    REQUIRE(!q.number(A::external));
    REQUIRE(p.to_string() == "@f(1,2;3,*)");
}

TEST_CASE_METHOD(Fixture, "ast theory variable", "[cxx][ast][theory_variable]") {
    auto x = node<T::theory_term_variable>(loc, "X", false);
    auto a = node<T::theory_term_variable>(loc, "_", true);

    REQUIRE((x.location(A::location) == loc));
    REQUIRE(x.string(A::name) == "X");
    REQUIRE_FALSE(x.number(A::anonymous) == 1);
    REQUIRE((a.string(A::name) == "_"));
    REQUIRE_FALSE(a.number(A::anonymous) == 0);

    REQUIRE(x.to_string() == "X");
    REQUIRE(a.to_string() == "_");
}

TEST_CASE_METHOD(Fixture, "ast theory term symbolic", "[cxx][ast][theory_term_symbolic]") {
    auto s = parse_sym("f(1,2)");
    auto p = node<T::theory_term_symbolic>(loc, s);

    REQUIRE((p.location(A::location) == loc));
    REQUIRE(p.symbol(A::symbol) == s);
    REQUIRE(p.to_string() == "f(1,2)");
}

TEST_CASE_METHOD(Fixture, "ast theory term tuple", "[cxx][ast][theory_term_tuple]") {
    auto p = node<T::theory_term_symbolic>(loc, parse_sym("p(1,2)"));
    auto q = node<T::theory_term_symbolic>(loc, parse_sym("q"));
    auto a = std::array{p, q};
    auto b = node<T::theory_term_tuple>(loc, AST::TheoryTupleType::set, a);

    REQUIRE((b.location(A::location) == loc));
    REQUIRE(std::ranges::equal(b.nodes(A::arguments), a));
    REQUIRE(b.number(A::tuple_type) == AST::TheoryTupleType::set);
    REQUIRE(b.to_string() == "{p(1,2),q}");
}

TEST_CASE_METHOD(Fixture, "ast theory term function", "[cxx][ast][theory_term_function]") {
    auto p = node<T::theory_term_symbolic>(loc, parse_sym("p(1,2)"));
    auto q = node<T::theory_term_symbolic>(loc, parse_sym("q"));
    auto a = std::array{p, q};
    auto b = node<T::theory_term_function>(loc, "a", a);
    auto c = node<T::theory_term_function>(loc, "++", a);
    auto d = node<T::theory_term_function>(loc, "not", std::array{q});

    REQUIRE((b.location(A::location) == loc));
    REQUIRE((c.location(A::location) == loc));
    REQUIRE(b.string(A::name) == "a");
    REQUIRE(c.string(A::name) == "++");
    REQUIRE(d.string(A::name) == "not");
    REQUIRE(std::ranges::equal(b.nodes(A::arguments), a));
    REQUIRE(std::ranges::equal(c.nodes(A::arguments), a));
    REQUIRE(b.to_string() == "a(p(1,2),q)");
    REQUIRE(c.to_string() == "(p(1,2) ++ q)");
    REQUIRE(d.to_string() == "(not q)");
}

TEST_CASE_METHOD(Fixture, "ast theory term unparsed", "[cxx][ast][theory_term_unparsed]") {
    auto p = node<T::theory_term_symbolic>(loc, parse_sym("p(1,2)"));
    auto q = node<T::theory_term_symbolic>(loc, parse_sym("q"));
    auto a = node<T::unparsed_element>(std::array{"+", "-"}, p);
    auto b = node<T::unparsed_element>(std::array{"*"}, q);
    auto x = node<T::theory_term_unparsed>(loc, std::array{a, b});

    REQUIRE((x.location(A::location) == loc));
    REQUIRE((p.location(A::location) == loc));
    REQUIRE((q.location(A::location) == loc));
    REQUIRE(std::ranges::equal(a.strings(A::operators), std::array{"+", "-"}));
    REQUIRE(std::ranges::equal(b.strings(A::operators), std::array{"*"}));
    REQUIRE(a.node(A::term) == p);
    REQUIRE(b.node(A::term) == q);
    REQUIRE(std::ranges::equal(x.nodes(A::elements), std::array{a, b}));
    REQUIRE(a.to_string() == "+ - p(1,2)");
    REQUIRE(b.to_string() == "* q");
    REQUIRE(x.to_string() == "(+ - p(1,2) * q)");
}

TEST_CASE_METHOD(Fixture, "ast literal boolean", "[cxx][ast][literal_boolean]") {
    auto p = node<T::literal_boolean>(loc, AST::Sign::single, true);

    REQUIRE((p.location(A::location) == loc));
    REQUIRE(p.number(A::sign) == AST::Sign::single);
    REQUIRE(p.number(A::value) == 1);
    REQUIRE(p.to_string() == "not #true");
}

TEST_CASE_METHOD(Fixture, "ast literal symbolic", "[cxx][ast][literal_symbolic]") {
    auto a = parse_term("-f(X)");
    auto p = node<T::literal_symbolic>(loc, AST::Sign::single, a);

    REQUIRE((p.location(A::location) == loc));
    REQUIRE(p.number(A::sign) == AST::Sign::single);
    REQUIRE(p.node(A::atom) == a);
    REQUIRE(p.to_string() == "not -f(X)");
}

TEST_CASE_METHOD(Fixture, "ast literal comparison", "[cxx][ast][literal_comparison]") {
    auto a = parse_term("X");
    auto b = node<T::right_guard>(AST::Relation::less, parse_term("Y"));
    auto c = node<T::right_guard>(AST::Relation::less_equal, parse_term("Z"));
    auto p = node<T::literal_comparison>(loc, AST::Sign::single, a, std::array{b, c});

    REQUIRE((p.location(A::location) == loc));
    REQUIRE(p.number(A::sign) == AST::Sign::single);
    REQUIRE(p.node(A::left) == a);
    REQUIRE(std::ranges::equal(p.nodes(A::right), std::array{b, c}));
    REQUIRE(p.to_string() == "not X<Y<=Z");
}

TEST_CASE_METHOD(Fixture, "ast body simple literal", "[cxx][ast][body_simple_literal]") {
    const auto *s = "not p(X)";
    auto lit = parse_lit(s);
    auto p = node<T::body_simple_literal>(lit);

    REQUIRE(p.node(A::literal) == lit);
    REQUIRE(p.to_string() == s);
}

TEST_CASE_METHOD(Fixture, "ast body conditional literal", "[cxx][ast][body_conditional_literal]") {
    auto s = parse_lit("not p(X)");
    auto t = parse_lit("r(X)");
    auto p = node<T::body_conditional_literal>(loc, s, std::array{t});

    REQUIRE((p.location(A::location) == loc));
    REQUIRE(p.node(A::literal) == s);
    REQUIRE(std::ranges::equal(p.nodes(A::condition), std::array{t}));
    REQUIRE(p.to_string() == "not p(X): r(X)");
}

TEST_CASE_METHOD(Fixture, "ast body set aggregate", "[cxx][ast][body_set_aggregate]") {
    auto t1 = parse_term("5");
    auto l1 = parse_lit("not p(X)");
    auto l2 = parse_lit("r(X)");
    auto e1 = node<T::set_aggregate_element>(loc, l1, std::array{l2});
    auto lg1 = node<T::left_guard>(t1, AST::Relation::less);
    auto rg1 = node<T::right_guard>(AST::Relation::less_equal, t1);
    auto a1 = node<T::body_set_aggregate>(loc, AST::Sign::single, std::nullopt, std::array{e1}, std::nullopt);
    auto a2 = node<T::body_set_aggregate>(loc, AST::Sign::no_sign, std::make_optional(lg1), std::array{e1}, rg1);

    REQUIRE((e1.location(A::location) == loc));
    REQUIRE(e1.node(A::literal) == l1);
    REQUIRE(std::ranges::equal(e1.nodes(A::condition), std::array{l2}));
    REQUIRE(lg1.node(A::term) == t1);
    REQUIRE(lg1.number(A::relation) == AST::Relation::less);
    REQUIRE(rg1.node(A::term) == t1);
    REQUIRE(rg1.number(A::relation) == AST::Relation::less_equal);
    REQUIRE((a1.location(A::location) == loc));
    REQUIRE(a1.number(A::sign) == AST::Sign::single);
    REQUIRE(!a1.optional_node(A::left).has_value());
    REQUIRE(std::ranges::equal(a1.nodes(A::elements), std::array{e1}));
    REQUIRE(!a1.optional_node(A::right).has_value());
    REQUIRE((a2.location(A::location) == loc));
    REQUIRE(a2.number(A::sign) == AST::Sign::no_sign);
    REQUIRE(a2.node(A::left) == lg1);
    REQUIRE(std::ranges::equal(a2.nodes(A::elements), std::array{e1}));
    REQUIRE(a2.node(A::right) == rg1);
    REQUIRE(e1.to_string() == "not p(X): r(X)");
    REQUIRE(lg1.to_string() == "5 < ");
    REQUIRE(rg1.to_string() == " <= 5");
    REQUIRE(a1.to_string() == "not { not p(X): r(X) }");
    REQUIRE(a2.to_string() == "5 < { not p(X): r(X) } <= 5");
}

TEST_CASE_METHOD(Fixture, "ast body aggregate", "[cxx][ast][body_aggregate]") {
    auto t1 = parse_term("5");
    auto t2 = parse_term("X");
    auto l1 = parse_lit("not p(X)");
    auto l2 = parse_lit("r(X)");
    auto e1 = node<T::body_aggregate_element>(loc, std::array{t1, t2}, std::array{l1, l2});
    auto lg1 = node<T::left_guard>(t1, AST::Relation::less);
    auto rg1 = node<T::right_guard>(AST::Relation::less_equal, t1);

    auto a1 = node<T::body_aggregate>(loc, AST::Sign::single, std::nullopt, AST::AggregateFunction::count,
                                      std::array{e1}, std::nullopt);
    auto a2 = node<T::body_aggregate>(loc, AST::Sign::no_sign, lg1, AST::AggregateFunction::sum, std::array{e1}, rg1);

    REQUIRE((e1.location(A::location) == loc));
    REQUIRE(std::ranges::equal(e1.nodes(A::tuple), std::array{t1, t2}));
    REQUIRE(std::ranges::equal(e1.nodes(A::condition), std::array{l1, l2}));
    REQUIRE((a1.location(A::location) == loc));
    REQUIRE(a1.number(A::sign) == AST::Sign::single);
    REQUIRE(!a1.optional_node(A::left).has_value());
    REQUIRE(a1.number(A::function) == AST::AggregateFunction::count);
    REQUIRE(std::ranges::equal(a1.nodes(A::elements), std::array{e1}));
    REQUIRE(!a1.optional_node(A::right).has_value());
    REQUIRE((a2.location(A::location) == loc));
    REQUIRE(a2.number(A::sign) == AST::Sign::no_sign);
    REQUIRE(a2.node(A::left) == lg1);
    REQUIRE(a2.number(A::function) == AST::AggregateFunction::sum);
    REQUIRE(std::ranges::equal(a2.nodes(A::elements), std::array{e1}));
    REQUIRE(a2.node(A::right) == rg1);
    REQUIRE(e1.to_string() == "5,X: not p(X), r(X)");
    REQUIRE(a1.to_string() == "not #count { 5,X: not p(X), r(X) }");
    REQUIRE(a2.to_string() == "5 < #sum { 5,X: not p(X), r(X) } <= 5");
}

TEST_CASE_METHOD(Fixture, "ast body theory atom", "[cxx][ast][body_theory_atom]") {
    auto t1 = parse_term("f(X)");
    auto tt1 = node<T::theory_term_symbolic>(loc, parse_sym("f(1,2)"));
    auto tt2 = node<T::theory_term_symbolic>(loc, parse_sym("5"));
    auto l1 = parse_lit("not p(X)");
    auto l2 = parse_lit("r(X)");
    auto e1 = node<T::theory_atom_element>(loc, std::array{tt1, tt2}, std::array{l1, l2});
    auto rg1 = node<T::theory_right_guard>("<>", tt2);

    auto a1 = node<T::body_theory_atom>(loc, AST::Sign::single, t1, std::array{e1}, rg1);
    auto a2 = node<T::body_theory_atom>(loc, AST::Sign::single, t1, std::array{e1}, std::nullopt);

    REQUIRE(rg1.string(A::theory_operator) == "<>");
    REQUIRE(rg1.node(A::term) == tt2);
    REQUIRE((e1.location(A::location) == loc));
    REQUIRE(std::ranges::equal(e1.nodes(A::tuple), std::array{tt1, tt2}));
    REQUIRE(std::ranges::equal(e1.nodes(A::condition), std::array{l1, l2}));
    REQUIRE((a1.location(A::location) == loc));
    REQUIRE(a1.number(A::sign) == AST::Sign::single);
    REQUIRE(a1.node(A::name) == t1);
    REQUIRE(std::ranges::equal(a1.nodes(A::elements), std::array{e1}));
    REQUIRE(a1.node(A::right) == rg1);
    REQUIRE(!a2.optional_node(A::right).has_value());
    REQUIRE(e1.to_string() == "f(1,2),5: not p(X), r(X)");
    REQUIRE(rg1.to_string() == " <> 5");
    REQUIRE(a1.to_string() == "not &f(X) { f(1,2),5: not p(X), r(X) } <> 5");
}

TEST_CASE_METHOD(Fixture, "ast head simple literal", "[cxx][ast][head_simple_literal]") {
    auto lit = parse_lit("p(X)");
    auto h = node<T::head_simple_literal>(lit);

    REQUIRE(h.node(A::literal) == lit);
    REQUIRE(h.to_string() == "p(X)");
}

TEST_CASE_METHOD(Fixture, "ast head disjunction", "[cxx][ast][head_disjunction]") {
    auto l1 = parse_lit("not p(X)");
    auto l2 = parse_lit("r(X)");
    auto l3 = node<T::head_conditional_literal>(loc, l2, std::array{l1});

    auto p = node<T::head_disjunction>(loc, std::array{l2, l3});

    REQUIRE((l3.location(A::location) == loc));
    REQUIRE(l3.node(A::literal) == l2);
    REQUIRE(std::ranges::equal(l3.nodes(A::condition), std::array{l1}));

    REQUIRE((p.location(A::location) == loc));
    REQUIRE(std::ranges::equal(p.nodes(A::elements), std::array{l2, l3}));
    REQUIRE(p.to_string() == "r(X); r(X): not p(X)");
}

TEST_CASE_METHOD(Fixture, "ast head set aggregate", "[cxx][ast][head_set_aggregate]") {
    auto t1 = parse_term("5");
    auto l1 = parse_lit("not p(X)");
    auto l2 = parse_lit("r(X)");
    auto e1 = node<T::set_aggregate_element>(loc, l1, std::array{l2});
    auto lg1 = node<T::left_guard>(t1, AST::Relation::less);
    auto rg1 = node<T::right_guard>(AST::Relation::less_equal, t1);
    auto a1 = node<T::head_set_aggregate>(loc, std::nullopt, std::array{e1}, std::nullopt);
    auto a2 = node<T::head_set_aggregate>(loc, std::make_optional(lg1), std::array{e1}, rg1);

    REQUIRE((e1.location(A::location) == loc));
    REQUIRE(e1.node(A::literal) == l1);
    REQUIRE(std::ranges::equal(e1.nodes(A::condition), std::array{l2}));
    REQUIRE(lg1.node(A::term) == t1);
    REQUIRE(lg1.number(A::relation) == AST::Relation::less);
    REQUIRE(rg1.node(A::term) == t1);
    REQUIRE(rg1.number(A::relation) == AST::Relation::less_equal);
    REQUIRE((a1.location(A::location) == loc));
    REQUIRE(!a1.optional_node(A::left).has_value());
    REQUIRE(std::ranges::equal(a1.nodes(A::elements), std::array{e1}));
    REQUIRE(!a1.optional_node(A::right).has_value());
    REQUIRE((a2.location(A::location) == loc));
    REQUIRE(a2.node(A::left) == lg1);
    REQUIRE(std::ranges::equal(a2.nodes(A::elements), std::array{e1}));
    REQUIRE(a2.node(A::right) == rg1);
    REQUIRE(e1.to_string() == "not p(X): r(X)");
    REQUIRE(lg1.to_string() == "5 < ");
    REQUIRE(rg1.to_string() == " <= 5");
    REQUIRE(a1.to_string() == "{ not p(X): r(X) }");
    REQUIRE(a2.to_string() == "5 < { not p(X): r(X) } <= 5");
}

TEST_CASE_METHOD(Fixture, "ast head aggregate", "[cxx][ast][head_aggregate]") {
    auto t1 = parse_term("5");
    auto t2 = parse_term("X");
    auto l1 = parse_lit("not p(X)");
    auto l2 = parse_lit("r(X)");
    auto l3 = parse_lit("q(X)");
    auto e1 = node<T::head_aggregate_element>(loc, std::array{t1, t2}, l3, std::array{l1, l2});
    auto lg1 = node<T::left_guard>(t1, AST::Relation::less);
    auto rg1 = node<T::right_guard>(AST::Relation::less_equal, t1);

    auto a1 = node<T::head_aggregate>(loc, std::nullopt, AST::AggregateFunction::count, std::array{e1}, std::nullopt);
    auto a2 = node<T::head_aggregate>(loc, lg1, AST::AggregateFunction::sum, std::array{e1}, rg1);

    REQUIRE((e1.location(A::location) == loc));
    REQUIRE(std::ranges::equal(e1.nodes(A::tuple), std::array{t1, t2}));
    REQUIRE(e1.node(A::literal) == l3);
    REQUIRE(std::ranges::equal(e1.nodes(A::condition), std::array{l1, l2}));
    REQUIRE((a1.location(A::location) == loc));
    REQUIRE(!a1.optional_node(A::left).has_value());
    REQUIRE(a1.number(A::function) == AST::AggregateFunction::count);
    REQUIRE(std::ranges::equal(a1.nodes(A::elements), std::array{e1}));
    REQUIRE(!a1.optional_node(A::right).has_value());
    REQUIRE((a2.location(A::location) == loc));
    REQUIRE(a2.node(A::left) == lg1);
    REQUIRE(a2.number(A::function) == AST::AggregateFunction::sum);
    REQUIRE(std::ranges::equal(a2.nodes(A::elements), std::array{e1}));
    REQUIRE(a2.node(A::right) == rg1);
    REQUIRE(e1.to_string() == "5,X: q(X): not p(X), r(X)");
    REQUIRE(a1.to_string() == "#count { 5,X: q(X): not p(X), r(X) }");
    REQUIRE(a2.to_string() == "5 < #sum { 5,X: q(X): not p(X), r(X) } <= 5");
}

TEST_CASE_METHOD(Fixture, "ast head theory atom", "[cxx][ast][head_theory_atom]") {
    auto t1 = parse_term("f(X)");
    auto tt1 = node<T::theory_term_symbolic>(loc, parse_sym("f(1,2)"));
    auto tt2 = node<T::theory_term_symbolic>(loc, parse_sym("5"));
    auto l1 = parse_lit("not p(X)");
    auto l2 = parse_lit("r(X)");
    auto e1 = node<T::theory_atom_element>(loc, std::array{tt1, tt2}, std::array{l1, l2});
    auto rg1 = node<T::theory_right_guard>("<>", tt2);

    auto a1 = node<T::head_theory_atom>(loc, t1, std::array{e1}, rg1);
    auto a2 = node<T::head_theory_atom>(loc, t1, std::array{e1}, std::nullopt);

    REQUIRE(rg1.string(A::theory_operator) == "<>");
    REQUIRE(rg1.node(A::term) == tt2);
    REQUIRE((e1.location(A::location) == loc));
    REQUIRE(std::ranges::equal(e1.nodes(A::tuple), std::array{tt1, tt2}));
    REQUIRE(std::ranges::equal(e1.nodes(A::condition), std::array{l1, l2}));
    REQUIRE((a1.location(A::location) == loc));
    REQUIRE(a1.node(A::name) == t1);
    REQUIRE(std::ranges::equal(a1.nodes(A::elements), std::array{e1}));
    REQUIRE(a1.node(A::right) == rg1);
    REQUIRE(!a2.optional_node(A::right).has_value());
    REQUIRE(e1.to_string() == "f(1,2),5: not p(X), r(X)");
    REQUIRE(rg1.to_string() == " <> 5");
    REQUIRE(a1.to_string() == "&f(X) { f(1,2),5: not p(X), r(X) } <> 5");
}

TEST_CASE_METHOD(Fixture, "ast statement rule", "[cxx][ast][statement_rule]") {
    auto h = node<T::head_simple_literal>(parse_lit("not q(X)"));
    auto b = node<T::body_simple_literal>(parse_lit("p(X)"));
    auto r = node<T::statement_rule>(loc, h, std::array{b});

    REQUIRE((r.location(A::location) == loc));
    REQUIRE(r.node(A::head) == h);
    REQUIRE(std::ranges::equal(r.nodes(A::body), std::array{b}));
    REQUIRE(r.to_string() == "not q(X) :- p(X).");
}

TEST_CASE_METHOD(Fixture, "ast statement theory", "[cxx][ast][statement_theory]") {
    auto od1 = node<T::theory_operator_definition>(loc, "+", 3, AST::TheoryOperatorType::binary_left);
    REQUIRE((od1.location(A::location) == loc));
    REQUIRE(od1.string(A::name) == "+");
    REQUIRE(od1.number(A::priority) == 3);
    REQUIRE(od1.number(A::operator_type) == AST::TheoryOperatorType::binary_left);
    REQUIRE(od1.to_string() == "+ : 3, binary, left");

    auto td1 = node<T::theory_term_definition>(loc, "t", std::array{od1});
    REQUIRE((td1.location(A::location) == loc));
    REQUIRE(td1.string(A::name) == "t");
    REQUIRE(std::ranges::equal(td1.nodes(A::operators), std::array{od1}));
    REQUIRE(td1.to_string() == "t { + : 3, binary, left }");

    auto gd1 = node<T::theory_guard_definition>(std::array{"+", "-"}, "t");
    REQUIRE(std::ranges::equal(gd1.strings(A::operators), std::array{"+", "-"}));
    REQUIRE(gd1.string(A::term) == "t");
    REQUIRE(gd1.to_string() == "{+,-}, t");

    auto ad1 = node<T::theory_atom_definition>(loc, "p", 1, "t", std::nullopt, AST::TheoryAtomType::directive);
    auto ad2 = node<T::theory_atom_definition>(loc, "p", 1, "t", gd1, AST::TheoryAtomType::directive);
    REQUIRE((ad1.location(A::location) == loc));
    REQUIRE(ad1.string(A::name) == "p");
    REQUIRE(ad1.number(A::arity) == 1);
    REQUIRE(ad1.string(A::term) == "t");
    REQUIRE(!ad1.optional_node(A::guard).has_value());
    REQUIRE(ad1.number(A::atom_type) == AST::TheoryAtomType::directive);
    REQUIRE(ad2.node(A::guard) == gd1);
    REQUIRE(ad1.to_string() == "&p/1: t, directive");
    REQUIRE(ad2.to_string() == "&p/1: t, {+,-}, t, directive");

    auto d1 = node<T::statement_theory>(loc, "t", std::array{td1}, std::array{ad1, ad2});
    REQUIRE((d1.location(A::location) == loc));
    REQUIRE(d1.string(A::name) == "t");
    REQUIRE(std::ranges::equal(d1.nodes(A::terms), std::array{td1}));
    REQUIRE(std::ranges::equal(d1.nodes(A::atoms), std::array{ad1, ad2}));
    REQUIRE(d1.to_string() == "#theory t {\n"
                              "  t { + : 3, binary, left };\n"
                              "  &p/1: t, directive;\n"
                              "  &p/1: t, {+,-}, t, directive\n"
                              "}.");
}

/*
    def test_statement_optimize(self):
        """
        Test optimization statements.
        """
        terms = [ast.parse_term(self.lib, "X"), ast.parse_term(self.lib, "Y")]
        weight = ast.parse_term(self.lib, "5")
        prio = ast.parse_term(self.lib, "2")
        t1 = ast.OptimizeTuple(self.lib, weight, None, terms)
        t2 = ast.OptimizeTuple(self.lib, weight, prio, terms)
        assert t1.weight == weight
        assert t1.priority is None
        assert t1.terms == terms
        assert t2.priority == prio
        assert str(t1) == "5,X,Y"
        assert str(t2) == "5@2,X,Y"

        l1 = ast.parse_literal(self.lib, "p(X)")
        l2 = ast.parse_literal(self.lib, "q(X)")
        e1 = ast.OptimizeElement(self.lib, t1, [l1, l2])
        e2 = ast.OptimizeElement(self.lib, t2, [l1, l2])
        assert e1.tuple == t1
        assert e1.condition == [l1, l2]
        assert str(e1), "5,X,Y: p(X) == q(X)"
        assert str(e2), "5@2,X,Y: p(X) == q(X)"

        so1 = ast.StatementOptimize(
            self.lib, self.loc, [e1, e2], ast.OptimizeType.Minimize
        )
        assert so1.location == self.loc
        assert so1.elements, [e1 == e2]
        assert so1.optimize_type == ast.OptimizeType.Minimize
        assert str(so1) == "#minimize { 5,X,Y: p(X), q(X); 5@2,X,Y: p(X), q(X) }."

        body = [
            ast.BodySimpleLiteral(self.lib, l1),
            ast.BodySimpleLiteral(self.lib, l2),
        ]
        sw1 = ast.StatementWeakConstraint(self.lib, self.loc, body, t1)
        assert sw1.body == body
        assert sw1.tuple == t1
        assert str(sw1) == " :~ p(X); q(X). [5,X,Y]"

    def test_statement_show(self):
        """
        Test show statements.
        """
        t1 = ast.parse_term(self.lib, "-p(X)")
        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = ast.StatementShow(self.lib, self.loc, t1, [l1, l2])
        assert s1.location == self.loc
        assert s1.term == t1
        assert s1.body == [l1, l2]
        assert str(s1) == "#show -p(X): q(X); p(X)."

        s2 = ast.StatementShowSignature(self.lib, self.loc, "p", 2)
        s3 = ast.StatementShowSignature(self.lib, self.loc, "q", 2, True)
        assert s2.location == self.loc
        assert s2.name == "p"
        assert s2.arity == 2
        assert not s2.sign
        assert s3.sign
        assert str(s2) == "#show p/2."
        assert str(s3) == "#show -q/2."

        s4 = ast.StatementShowNothing(self.lib, self.loc)
        assert str(s4) == "#show."

    def test_statement_project(self):
        """
        Test project statements.
        """
        t1 = ast.parse_term(self.lib, "-p(X)")
        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = ast.StatementProject(self.lib, self.loc, t1, [l1, l2])
        assert s1.location == self.loc
        assert s1.atom == t1
        assert s1.body == [l1, l2]
        assert str(s1) == "#project -p(X): q(X); p(X)."

        s2 = ast.StatementProjectSignature(self.lib, self.loc, "p", 2)
        s3 = ast.StatementProjectSignature(self.lib, self.loc, "q", 2, True)
        assert s2.location == self.loc
        assert s2.name == "p"
        assert s2.arity == 2
        assert not s2.sign
        assert s3.sign
        assert str(s2) == "#project p/2."
        assert str(s3) == "#project -q/2."

    def test_statement_defined(self):
        """
        Test defined statements.
        """
        s2 = ast.StatementDefined(self.lib, self.loc, "p", 2)
        s3 = ast.StatementDefined(self.lib, self.loc, "q", 2, True)
        assert s2.location == self.loc
        assert s2.name == "p"
        assert s2.arity == 2
        assert not s2.sign
        assert s3.sign
        assert str(s2) == "#defined p/2."
        assert str(s3) == "#defined -q/2."

    def test_statement_external(self):
        """
        Test external statements.
        """
        t1 = ast.parse_term(self.lib, "-p(X)")
        t2 = ast.parse_term(self.lib, "true")
        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = ast.StatementExternal(self.lib, self.loc, t1, [l1, l2])
        s2 = ast.StatementExternal(self.lib, self.loc, t1, [l1, l2], t2)
        assert s1.location == self.loc
        assert s1.atom == t1
        assert s1.body == [l1, l2]
        assert s1.external_type is None
        assert s2.external_type == t2
        assert str(s1) == "#external -p(X): q(X); p(X)."
        assert str(s2) == "#external -p(X): q(X); p(X). [true]"

    def test_statement_edge(self):
        """
        Test external statements.
        """
        u1 = ast.parse_term(self.lib, "u")
        v1 = ast.parse_term(self.lib, "v")
        u2 = ast.parse_term(self.lib, "x")
        v2 = ast.parse_term(self.lib, "y")
        e1 = ast.Edge(self.lib, u1, v1)
        e2 = ast.Edge(self.lib, u2, v2)
        assert e1.u == u1
        assert e1.v == v1
        assert e2.u == u2
        assert e2.v == v2
        assert str(e1) == "u,v"
        assert str(e2) == "x,y"

        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = ast.StatementEdge(self.lib, self.loc, [e1, e2], [l1, l2])
        assert s1.location == self.loc
        assert s1.pool == [e1, e2]
        assert s1.body == [l1, l2]
        assert str(s1) == "#edge (u,v;x,y): q(X); p(X)."

    def test_statement_heuristic(self):
        """
        Test heuristic statements.
        """
        a = ast.parse_term(self.lib, "a")
        w = ast.parse_term(self.lib, "w")
        p = ast.parse_term(self.lib, "p")
        m = ast.parse_term(self.lib, "m")

        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = ast.StatementHeuristic(self.lib, self.loc, a, [l1, l2], w, m)
        s2 = ast.StatementHeuristic(self.lib, self.loc, a, [l1, l2], w, m, p)
        assert s1.atom == a
        assert s1.weight == w
        assert s1.modifier == m
        assert s1.priority is None
        assert s2.priority == p
        assert str(s1) == "#heuristic a: q(X); p(X). [w,m]"
        assert str(s2) == "#heuristic a: q(X); p(X). [w@p,m]"

    def test_statement_include(self):
        """
        Test include statements.
        """
        s1 = ast.StatementInclude(self.lib, self.loc, "file", ast.IncludeType.System)
        assert s1.location == self.loc
        assert s1.value == "file"
        assert s1.include_type == ast.IncludeType.System
        assert str(s1) == '#include "file".'

    def test_statement_program(self):
        """
        Test program statements.
        """
        s1 = ast.StatementProgram(self.lib, self.loc, "step", ["t", "k"])
        assert s1.location == self.loc
        assert s1.name == "step"
        assert s1.arguments == ["t", "k"]
        assert str(s1) == "#program step(t,k)."

    def test_statement_script(self):
        """
        Test script statements.
        """
        s1 = ast.StatementScript(self.lib, self.loc, "def p(x): return x", "python")
        assert s1.location == self.loc
        assert s1.value == "def p(x): return x"
        assert s1.script_type == "python"
        assert str(s1) == "#script (python)def p(x): return x#end."

    def test_statement_const(self):
        """
        Test const statements.
        """
        t1 = ast.parse_term(self.lib, "f(2+3)")
        s1 = ast.StatementConst(self.lib, self.loc, "x", t1, ast.Precedence.Override)
        assert s1.location == self.loc
        assert s1.name == "x"
        assert s1.value == t1
        assert s1.precedence == ast.Precedence.Override
        assert str(s1) == "#const x=f(2+3). [override]"

    def test_statement_parts(self):
        """
        Test parts statements.
        """
        a1 = [parse_term(self.lib, "1"), parse_term(self.lib, "2")]
        e1 = ast.ProgramPart(self.lib, "base", [])
        e2 = ast.ProgramPart(self.lib, "step", a1)
        s1 = ast.StatementParts(self.lib, self.loc, [e1, e2], ast.Precedence.Override)
        assert s1.location == self.loc
        assert e1.name == "base"
        assert not e1.arguments
        assert e2.name == "step"
        assert e2.arguments == a1
        assert s1.precedence == ast.Precedence.Override
        assert s1.elements == [e1, e2]
        assert str(s1) == "#parts base,step(1,2). [override]"

    def test_statement_comment(self):
        """
        Test const statements.
        """
        s1 = ast.StatementComment(
            self.lib, self.loc, "% something arbitrary", ast.CommentType.Line
        )
        assert s1.location == self.loc
        assert s1.value == "% something arbitrary"
        assert s1.comment_type == ast.CommentType.Line
        assert str(s1) == "% something arbitrary"

    def test_parse(self):
        """
        Test parsing of asts.
        """
        term = "-f(X+Y,3)"
        assert str(ast.parse_term(self.lib, term)) == term
        theory_term = "(f ** X)"
        assert str(ast.parse_theory_term(self.lib, theory_term)) == theory_term
        lit = "not not p(X+2)"
        assert str(ast.parse_literal(self.lib, lit)) == lit
        head_lit = "a; b: c"
        assert str(ast.parse_head_literal(self.lib, head_lit)) == head_lit
        body_lit = "b: c"
        assert str(ast.parse_body_literal(self.lib, body_lit)) == body_lit
        stm = "a; b: c :- d: e."
        assert str(ast.parse_statement(self.lib, stm)) == stm

    def test_rewrite(self):
        """
        Test rewriting of statements.
        """

        def simp(stm, params=()):
            ctx = ast.RewriteContext(self.lib)
            ctx.project_anonymous = True
            for param in params:
                ctx.add_param(param)

            return [
                str(x)
                for x in ast.rewrite_statement(ctx, ast.parse_statement(self.lib, stm))
            ]

        stm = "a; b: c :- d: e."
        assert simp(stm) == [stm]

        stm = "p(X;Y) :- q(X,2*3); r(Y)."
        assert simp(stm) == ["p(X) :- q(X,6); r(*).", "p(Y) :- q(*,6); r(Y)."]

        stm = "p(X;Y) :- q(X+1,2*3), r(Y,t+1)."
        assert simp(stm, ["t"]) == [
            "p(X) :- q(1*X+1,6); r(*,1*t+1).",
            "p(Y) :- q(1*X+1,6); r(Y,1*t+1).",
        ]

        stm = "p(t)."
        assert simp(stm, ["t"]) == ["p(t)."]

        stm = " :- not p(_)."
        assert simp(stm, ["t"]) == [" :- not p(*)."]

        stm = "p(1;t) :- #false: p(t)."
        assert simp(stm, ["t"]) == ["p(1) :- #false: p(t).", "p(t) :- #false: p(t)."]

    def test_scan(self):
        """
        Test the statement scanner.
        """
        with ast.Scanner(self.lib, "a. b. c.") as scanner:
            assert [str(stm) for stm in scanner] == ["a.", "b.", "c."]

    def test_visit(self):
        """
        Test visiting.
        """
        variables = []

        @singledispatch
        def dispatch(arg, prefix):
            arg.visit(dispatch, prefix)

        @dispatch.register
        def _(var: ast.TermVariable, prefix):
            variables.append(prefix + var.name)

        stm = ast.parse_statement(self.lib, "p(A) :- q(B,C), D = {}.")
        dispatch(stm, "_")
        assert variables == ["_A", "_B", "_C", "_D"]

    def test_transform(self):
        """
        Test transformation and update.
        """

        @singledispatch
        def dispatch(expr, prefix):
            return expr.transform(self.lib, dispatch, prefix)

        @dispatch.register
        def _(var: ast.TermVariable, prefix):
            return var.update(self.lib, name=prefix + var.name)

        stm = ast.parse_statement(self.lib, "a(X) :- b(X).")
        assert str(dispatch(stm, "_")) == "a(_X) :- b(_X)."

    def test_cmp(self):
        """
        Test comparison functions.
        """
        x = ast.TermVariable(self.lib, self.loc, "X")
        a = ast.TermVariable(self.lib, self.loc, "_", True)

        assert x == x  # pylint: disable=comparison-with-itself
        assert a != x
        assert x < a
        assert x <= a
        assert a > x
        assert a >= x

        assert hash(x) == hash(x)
        assert hash(x) != hash(a)
*/

} // namespace Clingo::Test
