#include <clingo/ast.hh>

#include <catch2/catch_test_macros.hpp>

// NOLINTBEGIN(readability-magic-numbers)

namespace Clingo::Test {

TEST_CASE("cxx-ast") {
    using A = AST::Attribute;
    auto lib = Library{};
    auto node = AST::parse(lib, "a :- b.");
    auto head = node.node(A::head);
    auto body = node.nodes(A::body);
    REQUIRE(node.to_string() == "a :- b.");
    REQUIRE(head.to_string() == "a");
    REQUIRE(body.size() == 1);
    REQUIRE(body.front().to_string() == "b");

    auto loc = head.location(AST::Attribute::location);
    auto var = AST::Node(lib, AST::NodeType::term_variable, loc, "X", false);
    REQUIRE(var.to_string() == "X");
}

} // namespace Clingo::Test

// NOLINTEND(readability-magic-numbers)
