#include <clingo/ast.hh>

#include <catch2/catch_test_macros.hpp>

// NOLINTBEGIN(readability-magic-numbers)

namespace Clingo::Test {

TEST_CASE("cxx-ast") {
    using A = AST::Attribute;
    auto lib = Library{};
    auto stm = AST::parse(lib, "a :- b.");
    auto head = stm.node(A::head);
    auto body = stm.nodes(A::body);
    REQUIRE(stm.to_string() == "a :- b.");
    REQUIRE(head.to_string() == "a");
    REQUIRE(body.size() == 1);
    REQUIRE(body.front().to_string() == "b");

    auto loc = head.location(AST::Attribute::location);
    auto var_x = AST::Node::create<AST::NodeType::term_variable>(lib, loc, "X", false);
    REQUIRE(var_x.to_string() == "X");
    auto var_y = var_x.update<AST::NodeType::term_variable>(lib, []<AST::Attribute attr>() {
        if constexpr (attr == AST::Attribute::name) {
            return std::string_view{"Y"};
        }
    });
    REQUIRE(var_y.to_string() == "Y");

    auto trail = std::vector<std::string>{};
    stm.visit([&trail](AST::Node const &node) {
        trail.emplace_back(node.to_string());
        return true;
    });
    REQUIRE(trail == std::vector<std::string>{"a :- b.", "a", "a", "a", "b", "b", "b"});

    // TODO: I think, I am going to leave it up to the user whether to call transform recursively on a node.
    // To facilitate this process, there should be the following functions:
    // - transform(node)
    // - transform(nodes)
    // - transform(optional_node)
    auto var_z = var_y.transform(lib, [&lib](AST::Node const &node) -> std::optional<AST::Node> {
        if (node.type() == AST::NodeType::term_variable) {
            return node.update<AST::NodeType::term_variable>(lib, []<AST::Attribute attr>() {
                if constexpr (attr == AST::Attribute::name) {
                    return std::string_view{"Z"};
                }
            });
        }
        return std::nullopt;
    });
    REQUIRE(var_z.has_value());
    REQUIRE(var_z->to_string() == "Z");
}

} // namespace Clingo::Test

// NOLINTEND(readability-magic-numbers)
