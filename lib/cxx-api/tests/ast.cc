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

    [[nodiscard]] auto parse_blit(const std::string_view val) const -> N {
        return AST::parse(lib, val, AST::ParseType::body_literal);
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

TEST_CASE_METHOD(Fixture, "ast term fstring", "[cxx][ast][term_fstring]") {
    auto t = parse_term("f\"a{X:x}b\"");
    REQUIRE(t.to_string() == "f\"a{X:x}b\"");
    REQUIRE(t.type() == T::term_format_string);

    auto elems = t.nodes(A::elements);
    REQUIRE(elems.size() == 3);
    REQUIRE(elems[0].to_string() == "a");
    REQUIRE(elems[0].type() == T::format_field_literal);
    REQUIRE(elems[1].to_string() == "{X:x}");
    REQUIRE(elems[1].type() == T::format_field_expression);
    REQUIRE(elems[2].to_string() == "b");
    REQUIRE(elems[2].type() == T::format_field_literal);

    auto const &expr = elems[1];
    REQUIRE(expr.node(A::left).to_string() == "X");
    REQUIRE(expr.string(A::right) == ":x");

    REQUIRE(node<T::term_format_string>(loc, elems).to_string() == "f\"a{X:x}b\"");
    REQUIRE(node<T::format_field_literal>(loc, "a").to_string() == "a");
    REQUIRE(node<T::format_field_expression>(loc, parse_term("X"), ":x").to_string() == "{X:x}");
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
    auto const *s = "not p(X)";
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

TEST_CASE_METHOD(Fixture, "ast body sort", "[cxx][ast][body_sort]") {
    auto outputs = parse_term("(X,Y)");
    auto value = parse_term("Z");
    auto condition = parse_lit("p(Z)");
    auto elem = node<T::body_aggregate_element>(loc, std::array{value}, std::array{condition});
    auto sort = node<T::body_sort>(loc, AST::Sign::no_sign, outputs, std::array{elem});

    REQUIRE((sort.location(A::location) == loc));
    REQUIRE(sort.number(A::sign) == AST::Sign::no_sign);
    REQUIRE(sort.node(A::outputs) == outputs);
    REQUIRE(std::ranges::equal(sort.nodes(A::elements), std::array{elem}));
    REQUIRE(sort.to_string() == "(X,Y) = #sort { Z: p(Z) }");

    auto parsed = parse_blit("(X,Y) = #sort { Z: p(Z) }");
    REQUIRE(parsed.type() == T::body_sort);
    REQUIRE(parsed.node(A::outputs).to_string() == "(X,Y)");
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

TEST_CASE_METHOD(Fixture, "ast statement optimize", "[cxx][ast][statement_optimize]") {
    auto weight = parse_term("5");
    auto priority = parse_term("2");

    std::vector terms{parse_term("X"), parse_term("Y")};

    auto t1 = node<T::optimize_tuple>(weight, std::nullopt, terms);
    auto t2 = node<T::optimize_tuple>(weight, priority, terms);

    REQUIRE(t1.node(A::weight) == weight);
    REQUIRE(!t1.optional_node(A::priority).has_value());
    REQUIRE(std::ranges::equal(t1.nodes(A::terms), terms));
    REQUIRE(t2.node(A::priority) == priority);
    REQUIRE(t1.to_string() == "5,X,Y");
    REQUIRE(t2.to_string() == "5@2,X,Y");

    auto l1 = parse_lit("p(X)");
    auto l2 = parse_lit("q(X)");

    auto e1 = node<T::optimize_element>(t1, std::vector{l1, l2});
    auto e2 = node<T::optimize_element>(t2, std::vector{l1, l2});

    REQUIRE(e1.node(A::tuple) == t1);
    REQUIRE(std::ranges::equal(e1.nodes(A::condition), std::vector{l1, l2}));
    REQUIRE(e1.to_string() == "5,X,Y: p(X), q(X)");
    REQUIRE(e2.to_string() == "5@2,X,Y: p(X), q(X)");

    auto so1 = node<T::statement_optimize>(loc, std::vector{e1, e2}, AST::OptimizeType::minimize);
    REQUIRE((so1.location(A::location) == loc));
    REQUIRE(std::ranges::equal(so1.nodes(A::elements), std::vector{e1, e2}));
    REQUIRE(so1.number(A::optimize_type) == AST::OptimizeType::minimize);
    REQUIRE(so1.to_string() == "#minimize { 5,X,Y: p(X), q(X); 5@2,X,Y: p(X), q(X) }.");

    std::vector body{node<T::body_simple_literal>(l1), node<T::body_simple_literal>(l2)};

    auto sw1 = node<T::statement_weak_constraint>(loc, body, t1);
    REQUIRE(std::ranges::equal(sw1.nodes(A::body), body));
    REQUIRE(sw1.node(A::tuple) == t1);
    REQUIRE(sw1.to_string() == " :~ p(X); q(X). [5,X,Y]");
}

TEST_CASE_METHOD(Fixture, "ast statement show", "[cxx][ast][statement_show]") {
    auto t1 = parse_term("-p(X)");
    auto l1 = parse_blit("q(X)");
    auto l2 = parse_blit("p(X)");

    auto s1 = node<T::statement_show>(loc, t1, std::vector{l1, l2});
    REQUIRE((s1.location(A::location) == loc));
    REQUIRE(s1.node(A::term) == t1);
    REQUIRE(std::ranges::equal(s1.nodes(A::body), std::vector{l1, l2}));
    REQUIRE(s1.to_string() == "#show -p(X): q(X); p(X).");

    auto s2 = node<T::statement_show_signature>(loc, "p", 2, false, true);
    auto s3 = node<T::statement_show_signature>(loc, "q", 2, true, true);
    auto s4 = node<T::statement_show_signature>(loc, "p", 2, false, false);
    REQUIRE((s2.location(A::location) == loc));
    REQUIRE(s2.string(A::name) == "p");
    REQUIRE(s2.number(A::arity) == 2);
    REQUIRE(s2.number(A::sign) == 0);
    REQUIRE(s2.number(A::value) == 1);
    REQUIRE(s3.number(A::sign) == 1);
    REQUIRE(s3.number(A::value) == 1);
    REQUIRE(s4.number(A::value) == 0);
    REQUIRE(s2.to_string() == "#show p/2. [true]");
    REQUIRE(s3.to_string() == "#show -q/2. [true]");
    REQUIRE(s4.to_string() == "#show p/2. [false]");

    auto s5 = node<T::statement_show_nothing>(loc);
    REQUIRE(s5.to_string() == "#show.");
}

TEST_CASE_METHOD(Fixture, "ast statement project", "[cxx][ast][statement_project]") {
    auto t1 = parse_term("-p(X)");
    auto l1 = parse_blit("q(X)");
    auto l2 = parse_blit("p(X)");

    auto s1 = node<T::statement_project>(loc, t1, std::vector{l1, l2});
    REQUIRE((s1.location(A::location) == loc));
    REQUIRE(s1.node(A::atom) == t1);
    REQUIRE(std::ranges::equal(s1.nodes(A::body), std::vector{l1, l2}));
    REQUIRE(s1.to_string() == "#project -p(X): q(X); p(X).");

    auto s2 = node<T::statement_project_signature>(loc, "p", 2, false);
    auto s3 = node<T::statement_project_signature>(loc, "q", 2, true);
    REQUIRE((s2.location(A::location) == loc));
    REQUIRE(s2.string(A::name) == "p");
    REQUIRE(s2.number(A::arity) == 2);
    REQUIRE(s2.number(A::sign) == 0);
    REQUIRE(s3.number(A::sign) == 1);
    REQUIRE(s2.to_string() == "#project p/2.");
    REQUIRE(s3.to_string() == "#project -q/2.");
}

TEST_CASE_METHOD(Fixture, "ast statement defined", "[cxx][ast][statement_defined]") {
    auto s2 = node<T::statement_defined>(loc, "p", 2, false);
    auto s3 = node<T::statement_defined>(loc, "q", 2, true);

    REQUIRE((s2.location(A::location) == loc));
    REQUIRE(s2.string(A::name) == "p");
    REQUIRE(s2.number(A::arity) == 2);
    REQUIRE(s2.number(A::sign) == 0);
    REQUIRE(s3.number(A::sign) == 1);
    REQUIRE(s2.to_string() == "#defined p/2.");
    REQUIRE(s3.to_string() == "#defined -q/2.");
}

TEST_CASE_METHOD(Fixture, "ast statement external", "[cxx][ast][statement_external]") {
    auto t1 = parse_term("-p(X)");
    auto t2 = parse_term("true");
    auto l1 = parse_blit("q(X)");
    auto l2 = parse_blit("p(X)");

    auto s1 = node<T::statement_external>(loc, t1, std::vector{l1, l2}, std::nullopt);
    auto s2 = node<T::statement_external>(loc, t1, std::vector{l1, l2}, t2);

    REQUIRE((s1.location(A::location) == loc));
    REQUIRE(s1.node(A::atom) == t1);
    REQUIRE(std::ranges::equal(s1.nodes(A::body), std::vector{l1, l2}));
    REQUIRE(!s1.optional_node(A::external_type).has_value());

    REQUIRE(s2.node(A::external_type) == t2);

    REQUIRE(s1.to_string() == "#external -p(X): q(X); p(X).");
    REQUIRE(s2.to_string() == "#external -p(X): q(X); p(X). [true]");
}

TEST_CASE_METHOD(Fixture, "ast statement edge", "[cxx][ast][statement_edge]") {
    auto u1 = parse_term("u");
    auto v1 = parse_term("v");
    auto u2 = parse_term("x");
    auto v2 = parse_term("y");

    auto e1 = node<T::edge>(u1, v1);
    auto e2 = node<T::edge>(u2, v2);

    REQUIRE(e1.node(A::u) == u1);
    REQUIRE(e1.node(A::v) == v1);
    REQUIRE(e2.node(A::u) == u2);
    REQUIRE(e2.node(A::v) == v2);
    REQUIRE(e1.to_string() == "u,v");
    REQUIRE(e2.to_string() == "x,y");

    auto l1 = parse_blit("q(X)");
    auto l2 = parse_blit("p(X)");

    auto s1 = node<T::statement_edge>(loc, std::vector{e1, e2}, std::vector{l1, l2});
    REQUIRE((s1.location(A::location) == loc));
    REQUIRE(std::ranges::equal(s1.nodes(A::pool), std::vector{e1, e2}));
    REQUIRE(std::ranges::equal(s1.nodes(A::body), std::vector{l1, l2}));
    REQUIRE(s1.to_string() == "#edge (u,v;x,y): q(X); p(X).");
}

TEST_CASE_METHOD(Fixture, "ast statement heuristic", "[cxx][ast][statement_heuristic]") {
    auto a = parse_term("a");
    auto w = parse_term("w");
    auto p = parse_term("p");
    auto m = parse_term("m");

    auto l1 = parse_blit("q(X)");
    auto l2 = parse_blit("p(X)");

    auto s1 = node<T::statement_heuristic>(loc, a, std::vector{l1, l2}, w, m, std::nullopt);
    auto s2 = node<T::statement_heuristic>(loc, a, std::vector{l1, l2}, w, m, p);

    REQUIRE(s1.node(A::atom) == a);
    REQUIRE(s1.node(A::weight) == w);
    REQUIRE(s1.node(A::modifier) == m);
    REQUIRE(!s1.optional_node(A::priority).has_value());

    REQUIRE(s2.node(A::priority) == p);

    REQUIRE(s1.to_string() == "#heuristic a: q(X); p(X). [w,m]");
    REQUIRE(s2.to_string() == "#heuristic a: q(X); p(X). [w@p,m]");
}

TEST_CASE_METHOD(Fixture, "ast statement include", "[cxx][ast][statement_include]") {
    auto s1 = node<T::statement_include>(loc, "file", AST::IncludeType::system);

    REQUIRE((s1.location(A::location) == loc));
    REQUIRE(s1.string(A::value) == "file");
    REQUIRE(s1.number(A::include_type) == AST::IncludeType::system);
    REQUIRE(s1.to_string() == R"(#include "file".)");
}

TEST_CASE_METHOD(Fixture, "ast statement program", "[cxx][ast][statement_program]") {
    auto args = std::array<std::string_view, 2>{"t", "k"};
    auto s1 = node<T::statement_program>(loc, "step", args);

    REQUIRE((s1.location(A::location) == loc));
    REQUIRE(s1.string(A::name) == "step");
    REQUIRE(std::ranges::equal(s1.strings(A::arguments), args));
    REQUIRE(s1.to_string() == "#program step(t,k).");
}

TEST_CASE_METHOD(Fixture, "ast statement script", "[cxx][ast][statement_script]") {
    auto s1 = node<T::statement_script>(loc, "def p(x): return x", "python");

    REQUIRE((s1.location(A::location) == loc));
    REQUIRE(s1.string(A::value) == "def p(x): return x");
    REQUIRE(s1.string(A::script_type) == "python");
    REQUIRE(s1.to_string() == "#script (python)def p(x): return x#end.");
}

TEST_CASE_METHOD(Fixture, "ast statement const", "[cxx][ast][statement_const]") {
    auto t1 = parse_term("f(2+3)");
    auto s1 = node<T::statement_const>(loc, "x", t1, AST::Precedence::override);

    REQUIRE((s1.location(A::location) == loc));
    REQUIRE(s1.string(A::name) == "x");
    REQUIRE(s1.node(A::value) == t1);
    REQUIRE(s1.number(A::precedence) == AST::Precedence::override);
    REQUIRE(s1.to_string() == "#const x=f(2+3). [override]");
}

TEST_CASE_METHOD(Fixture, "ast statement parts", "[cxx][ast][statement_parts]") {
    auto t1 = parse_sym("1");
    auto t2 = parse_sym("2");
    auto terms = std::array{t1, t2};

    auto e1 = node<T::program_part>("base", std::array<Symbol, 0>{});
    auto e2 = node<T::program_part>("step", terms);

    auto s1 = node<T::statement_parts>(loc, std::array{e1, e2}, AST::Precedence::override);

    REQUIRE((s1.location(A::location) == loc));
    REQUIRE(e1.string(A::name) == "base");
    REQUIRE(e1.symbols(A::arguments).empty());
    REQUIRE(e2.string(A::name) == "step");
    REQUIRE(std::ranges::equal(e2.symbols(A::arguments), terms));
    REQUIRE(s1.number(A::precedence) == AST::Precedence::override);
    REQUIRE(std::ranges::equal(s1.nodes(A::elements), std::vector{e1, e2}));
    REQUIRE(s1.to_string() == "#parts base,step(1,2). [override]");
}

TEST_CASE_METHOD(Fixture, "ast statement comment", "[cxx][ast][statement_comment]") {
    auto s1 = node<T::statement_comment>(loc, "% something arbitrary", AST::CommentType::line);

    REQUIRE((s1.location(A::location) == loc));
    REQUIRE(s1.string(A::value) == "% something arbitrary");
    REQUIRE(s1.number(A::comment_type) == AST::CommentType::line);
    REQUIRE(s1.to_string() == "% something arbitrary");
}

TEST_CASE_METHOD(Fixture, "ast parse", "[cxx][ast][parse]") {
    auto expr = std::string_view{};

    expr = "-f(X+Y,3)";
    REQUIRE(AST::parse(lib, expr, AST::ParseType::term).to_string() == expr);

    expr = "(f ** X)";
    REQUIRE(AST::parse(lib, expr, AST::ParseType::theory_term).to_string() == expr);

    expr = "not not p(X+2)";
    REQUIRE(AST::parse(lib, expr, AST::ParseType::literal).to_string() == expr);

    expr = "a; b: c";
    REQUIRE(AST::parse(lib, expr, AST::ParseType::head_literal).to_string() == expr);

    expr = "b: c";
    REQUIRE(AST::parse(lib, expr, AST::ParseType::body_literal).to_string() == expr);

    expr = "a; b: c :- d: e.";
    REQUIRE(AST::parse(lib, expr, AST::ParseType::statement).to_string() == expr);
}

TEST_CASE_METHOD(Fixture, "ast rewrite", "[cxx][ast][rewrite]") {
    auto simp = [&](const std::string_view stm, std::initializer_list<std::string_view const> params = {}) {
        auto ctx = AST::RewriteContext{lib};
        ctx.project_anonymous(true);
        for (auto const &param : params) {
            ctx.add_param(param);
        }

        auto parsed = AST::parse(lib, stm);
        auto rewritten = AST::rewrite(ctx, parsed);

        auto res = std::vector<std::string>{};
        for (auto const &stmt : rewritten) {
            res.push_back(stmt.to_string());
        }
        return res;
    };

    std::string_view stm;

    stm = "a; b: c :- d: e.";
    REQUIRE(std::ranges::equal(simp(stm), std::array{stm}));

    stm = "p(X;Y) :- q(X,2*3); r(Y).";
    REQUIRE(std::ranges::equal(simp(stm), std::array{"p(X) :- q(X,6); r(*).", "p(Y) :- q(*,6); r(Y)."}));

    stm = "p(X;Y) :- q(X+1,2*3), r(Y,t+1).";
    REQUIRE(std::ranges::equal(simp(stm, {"t"}),
                               std::array{"p(X) :- q(1*X+1,6); r(*,1*t+1).", "p(Y) :- q(1*X+1,6); r(Y,1*t+1)."}));

    stm = "p(t).";
    REQUIRE(std::ranges::equal(simp(stm, {"t"}), std::array{"p(t)."}));

    stm = " :- not p(_).";
    REQUIRE(std::ranges::equal(simp(stm, {"t"}), std::array{" :- not p(*)."}));

    stm = "p(1;t) :- #false: p(t).";
    REQUIRE(std::ranges::equal(simp(stm, {"t"}), std::array{"p(1) :- #false: p(t).", "p(t) :- #false: p(t)."}));
}

TEST_CASE_METHOD(Fixture, "ast statement scanner", "[cxx][ast][scanner]") {
    auto res = std::vector<std::string>{};
    AST::parse(lib, "a. b. c.", [&](auto const &stm) { res.emplace_back(stm.to_string()); });
    REQUIRE(std::ranges::equal(res, std::array{"#program base.", "a.", "b.", "c."}));
}

TEST_CASE_METHOD(Fixture, "ast term variable comparison", "[cxx][ast][term_variable][cmp]") {
    auto x = node<T::term_variable>(loc, "X", false);
    auto a = node<T::term_variable>(loc, "_", true);

    REQUIRE(x == x);
    REQUIRE(a != x);

    REQUIRE(x < a);
    REQUIRE(x <= a);
    REQUIRE(a > x);
    REQUIRE(a >= x);

    auto hasher = std::hash<N>{};
    REQUIRE(hasher(x) == hasher(x));
    REQUIRE(hasher(x) != hasher(a));
}

TEST_CASE_METHOD(Fixture, "ast visit", "[cxx][ast][visit]") {
    auto stm = AST::parse(lib, "a :- b.");
    auto trail = std::vector<std::string>{};
    AST::Visitor visit = [&](AST::Node const &node) {
        trail.emplace_back(node.to_string());
        node.accept(visit);
    };
    visit(stm);
    REQUIRE(std::ranges::equal(trail, std::array{"a :- b.", "a", "a", "a", "b", "b", "b"}));
}

TEST_CASE_METHOD(Fixture, "ast transform", "[cxx][ast][transform]") {
    auto var_x = AST::Node::create<T::term_variable>(lib, loc, "X", false);
    REQUIRE(var_x.to_string() == "X");
    auto var_y = var_x.update<T::term_variable>(lib, []<A attr>() {
        if constexpr (attr == A::name) {
            return std::string_view{"Y"};
        }
    });
    REQUIRE(var_y.to_string() == "Y");

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

} // namespace Clingo::Test
