#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

namespace Clingo::Test {

namespace {

struct Fixture {
    Library lib;
    Control ctl = Control(lib, {"0", "--profile"});
};

} // namespace

TEST_CASE_METHOD(Fixture, "profile", "[cxx][profile]") {
    ctl.parse_string(R"(
        #const n = 10.
        { q(1..n,Y) } = 1 :- Y=1..n.
        { q(X,1..n) } = 1 :- X=1..n.
        :- q(X,Y), q(X',Y'), (X,Y)<(X',Y'), X'-X == |Y-Y'|.)");
    ctl.ground();

    auto profile = ctl.profile();
    REQUIRE(profile.size() == 3);
    // one of the generators
    auto &gen = profile.front();
    REQUIRE(std::holds_alternative<ProfileNodeInternal>(gen));
    auto &gen_children = std::get<ProfileNodeInternal>(gen).children;
    auto &gen_rew = gen_children.front();
    REQUIRE(std::holds_alternative<ProfileNodeInternal>(gen_rew));
    auto &gen_rew_children = std::get<ProfileNodeInternal>(gen_rew).children;

    //! the aggregate
    auto &gen_agg = gen_rew_children.front();
    REQUIRE(std::holds_alternative<ProfileNodeInternal>(gen_agg));
    auto &gen_agg_prof = std::get<ProfileNodeInternal>(gen_agg).children.front();
    REQUIRE(std::holds_alternative<ProfileNodeLeaf>(gen_agg_prof));
    auto leaf = std::get<ProfileNodeLeaf>(gen_agg_prof);
    REQUIRE(leaf.type == ProfileType::step);
    REQUIRE(leaf.instances == 100);
    REQUIRE(leaf.matches >= 100);
    REQUIRE(leaf.time_instantiate >= 0);
    REQUIRE(leaf.time_propagate >= 0);

    //! the generator
    auto &gen_stm_prof = gen_rew_children.back();
    REQUIRE(std::holds_alternative<ProfileNodeLeaf>(gen_stm_prof));
    leaf = std::get<ProfileNodeLeaf>(gen_stm_prof);
    REQUIRE(leaf.type == ProfileType::accu);
    REQUIRE(leaf.instances == 10);
    REQUIRE(leaf.matches >= 10);
    REQUIRE(leaf.time_instantiate >= 0);
    REQUIRE(leaf.time_propagate >= 0);

    //! the integrity constraint
    auto &cns = profile.back();
    REQUIRE(std::holds_alternative<ProfileNodeInternal>(cns));
    auto &cns_prof = std::get<ProfileNodeInternal>(cns).children.front();
    REQUIRE(std::holds_alternative<ProfileNodeLeaf>(cns_prof));
    leaf = std::get<ProfileNodeLeaf>(cns_prof);
    REQUIRE(leaf.type == ProfileType::step);
    REQUIRE(leaf.instances == 570);
    REQUIRE(leaf.matches >= 570);
    REQUIRE(leaf.time_instantiate >= 0);
    REQUIRE(leaf.time_propagate >= 0);
}

TEST_CASE_METHOD(Fixture, "profile sort literal", "[cxx][profile]") {
    ctl.parse_string("d(1..3). chain(X,Y) :- (X,Y) = #sort { Z : d(Z) }.");
    ctl.ground();

    auto profile = ctl.profile();
    REQUIRE(profile.size() == 2);
    REQUIRE(std::holds_alternative<ProfileNodeInternal>(profile.back()));

    auto const &rule = std::get<ProfileNodeInternal>(profile.back());
    REQUIRE_FALSE(rule.children.empty());
    REQUIRE(std::holds_alternative<ProfileNodeInternal>(rule.children.front()));

    auto const &sort = std::get<ProfileNodeInternal>(rule.children.front());
    REQUIRE(sort.key == "(X,Y) = #sort { Z: d(Z) }");
    REQUIRE(sort.nested);
    REQUIRE_FALSE(sort.children.empty());
    REQUIRE(std::holds_alternative<ProfileNodeLeaf>(sort.children.front()));

    auto const &leaf = std::get<ProfileNodeLeaf>(sort.children.front());
    REQUIRE(leaf.instances == 3);
    REQUIRE(leaf.matches >= 3);
}

} // namespace Clingo::Test
