#include <clingo/ast.hh>

#include <catch2/catch_test_macros.hpp>

namespace Clingo::Test {

TEST_CASE("cxx-ast") {
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

} // namespace Clingo::Test
