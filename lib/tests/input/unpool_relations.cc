#include <input/algo/unpool_relations.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

namespace {

template <class T> auto unpool_str(ParseHelper &ph, std::optional<T> value, char const *sep = ", ") -> std::string {
    if (value) {
        ConstMap const_map;
        ParamMap param_map;
        RewriteContext ctx{ph, ph, param_map, const_map, {}, "__A_"};
        auto unpooled = unpool_relations(ctx, value.value());
        if (ph.logger().has_error()) {
            throw std::runtime_error("error while unpooling");
        }
        if (!unpooled.has_value()) {
            unpooled = Util::make_vec<T>(value.value());
        }
        return to_str(unpooled.value(), sep);
    }
    return "<failed>";
}

auto unpool_statement(std::string const &str) -> std::string {
    ParseHelper ph;
    return unpool_str(ph, ph.statement(str), " ");
}

} // namespace

TEST_CASE("unpool_relations_head") {
    REQUIRE(unpool_statement("1<=X<=Y.") == "[1<=X. X<=Y.]");
    REQUIRE(unpool_statement("not 1<=X<=Y.") == "[:- 1<=X; X<=Y.]");
}

} // namespace Gringo::Input::Test
