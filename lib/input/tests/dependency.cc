#include "test.hh"

#include <clingo/input/rewrite/dependency.hh>
#include <clingo/input/rewrite/simplify.hh>

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Input::Test {

namespace {

auto unify_terms(std::string_view a, std::string_view b) -> bool {
    ParseHelper ph;
    auto simplify_term = [&ph](std::string_view str) -> Term {
        auto term = ph.term(str).value();
        auto res = simplify(SimplifyTermFlags::none, ph, term);
        REQUIRE(res.state);
        return std::move(res).value.value_or(std::move(term));
    };
    return unify(ph, simplify_term(a), simplify_term(b));
}

} // namespace

TEST_CASE("dependency") {
    SECTION("unify variable") {
        REQUIRE(unify_terms("X", "Y"));
        REQUIRE(unify_terms("X", "a"));
        REQUIRE(unify_terms("X", "f(Y)"));
        REQUIRE(!unify_terms("X", "f(X)"));
        REQUIRE(unify_terms("X", "(Y,)"));
        REQUIRE(!unify_terms("X", "(X,)"));
        REQUIRE(unify_terms("X", "-X"));
        REQUIRE(unify_terms("X", "|X|"));
        REQUIRE(unify_terms("X", "X+X"));
        REQUIRE(unify_terms("X", "1*X+0"));
        CHECK(!unify_terms("X", "1*X+1"));
    }
    SECTION("unify symbol") {
        REQUIRE(unify_terms("f(x)", "f(X)"));
        REQUIRE(!unify_terms("f(x)", "g(X)"));
        REQUIRE(unify_terms("(x,)", "(X,)"));
        REQUIRE(!unify_terms("(x,)", "(x,X,)"));
        REQUIRE(unify_terms("x", "x"));
        REQUIRE(!unify_terms("x", "y"));
        REQUIRE(unify_terms("1", "-X"));
        REQUIRE(unify_terms("1", "~X"));
        REQUIRE(unify_terms("-1", "-X"));
        REQUIRE(unify_terms("1", "|X|"));
        REQUIRE(!unify_terms("-1", "|X|"));
        REQUIRE(unify_terms("4", "2*X+2"));
        REQUIRE(!unify_terms("1", "2*X+2"));
        REQUIRE(unify_terms("1", "X*X"));
    }
    SECTION("unify tuple") {
        REQUIRE(unify_terms("(X,)", "(X,)"));
        REQUIRE(unify_terms("(X,)", "(Y,)"));
        REQUIRE(!unify_terms("(x,)", "(y,)"));
        REQUIRE(!unify_terms("(X,)", "-X"));
        REQUIRE(!unify_terms("(X,)", "|X|"));
        REQUIRE(!unify_terms("(X,)", "X*X"));
        REQUIRE(!unify_terms("(X,)", "1*X+1"));
    }
    SECTION("unify function") {
        REQUIRE(unify_terms("f(X)", "f(X)"));
        REQUIRE(unify_terms("f(X)", "f(Y)"));
        REQUIRE(!unify_terms("f(x)", "f(y)"));
        REQUIRE(!unify_terms("f(x)", "g(x)"));
        REQUIRE(!unify_terms("f(X)", "-X"));
        REQUIRE(!unify_terms("f(X)", "|X|"));
        REQUIRE(!unify_terms("f(X)", "X*X"));
        REQUIRE(!unify_terms("f(X)", "1*X+1"));
    }
    SECTION("unify absolute") {
        REQUIRE(unify_terms("|X|", "|X|"));
        REQUIRE(unify_terms("|X|", "|Y|"));
        REQUIRE(unify_terms("|X|", "|X+1|"));
        REQUIRE(unify_terms("|X|", "|Y+1|"));
        REQUIRE(unify_terms("|X|", "-X"));
        REQUIRE(unify_terms("|X|", "-Y"));
        REQUIRE(unify_terms("|X|", "X+1"));
        REQUIRE(unify_terms("|X|", "Y+1"));
        REQUIRE(unify_terms("|X|", "X*X"));
        REQUIRE(unify_terms("|X|", "Y*Y"));
    }
    SECTION("unify binary") {
        REQUIRE(unify_terms("2*X", "3*X"));
        REQUIRE(unify_terms("2*X", "3*Y"));
        REQUIRE(unify_terms("(-1*X,X)", "(-X,5)"));
        REQUIRE(unify_terms("(-1*X,X)", "(--X,0)"));
        REQUIRE(!unify_terms("(-1*X,X)", "(--X,1)"));
        REQUIRE(unify_terms("(2*X+1,X)", "(3*X+2,-1)"));
        REQUIRE(!unify_terms("(2*X+1,X)", "(3*X+2,1)"));
        REQUIRE(unify_terms("(2*X,X,Y)", "(4*Y,4,2)"));
        REQUIRE(unify_terms("(4*X,X,Y)", "(2*Y,2,4)"));
        REQUIRE(!unify_terms("(2*X,X,Y)", "(4*Y,2,5)"));
        REQUIRE(!unify_terms("(4*X,X,Y)", "(2*Y,5,2)"));
        REQUIRE(!unify_terms("(X,X+1)", "(a,X+1)"));
    }
    SECTION("unify unary") {
        REQUIRE(unify_terms("-X", "-Y"));
        REQUIRE(unify_terms("-X", "-X"));
        REQUIRE(unify_terms("-X", "--X"));
        REQUIRE(unify_terms("~X", "~Y"));
        REQUIRE(unify_terms("~X", "~X"));
        REQUIRE(unify_terms("~X", "~~X"));
        REQUIRE(!unify_terms("~X", "a"));
        REQUIRE(unify_terms("-X", "a"));
        REQUIRE(unify_terms("(-X,X)", "(a,-a)"));
        REQUIRE(!unify_terms("(-X,X)", "(-a,-a)"));
    }
    SECTION("analyze1") {
        ParseHelper ph;
        std::vector<Stm> stms;
        stms.emplace_back(opt_value(ph.statement("y.")));
        stms.emplace_back(opt_value(ph.statement("a :- x, y.")));
        stms.emplace_back(opt_value(ph.statement("x :- a.")));
        stms.emplace_back(opt_value(ph.statement("b :- not c, a.")));
        stms.emplace_back(opt_value(ph.statement("c :- not b.")));
        stms.emplace_back(opt_value(ph.statement("d :- e.")));
        stms.emplace_back(opt_value(ph.statement("e :- d, c.")));
        auto comps = analyze(ph, stms);
        REQUIRE(comps.size() == 4);
        REQUIRE(comps[0].size() == 1);
        REQUIRE(comps[0][0].stms.size() == 1);
        REQUIRE(comps[0][0].type == (ComponentType::positive | ComponentType::single_pass));
        REQUIRE(comps[1].size() == 1);
        REQUIRE(comps[1][0].stms.size() == 2);
        REQUIRE(comps[1][0].type == ComponentType::positive);
        REQUIRE(comps[2].size() == 2);
        REQUIRE(comps[2][0].stms.size() == 1);
        REQUIRE(comps[2][0].type == ComponentType::single_pass);
        REQUIRE(comps[2][1].stms.size() == 1);
        REQUIRE(comps[2][1].type == (ComponentType::positive | ComponentType::single_pass));
        REQUIRE(comps[3].size() == 1);
        REQUIRE(comps[3][0].stms.size() == 2);
        REQUIRE(comps[3][0].type == ComponentType::positive);
    }
    SECTION("analyze_rename") {
        auto ph = ParseHelper{};
        std::vector<Stm> stms;
        stms.emplace_back(opt_value(ph.statement("p(X,a) :- f(X).")));
        stms.emplace_back(opt_value(ph.statement("f(X) :- p(b,X).")));
        REQUIRE(analyze(ph, stms).size() == 1);
    }
    SECTION("analyze_program") {
        struct Builder : DependencyBuilder {
            void flush() {
                res.emplace_back(oss.str());
                oss.str("");
            }
            void do_param(ProgramParam const &param) override {
                oss << "#program_" << *param.first << "(";
                bool comma = false;
                for (auto const &sym : param.second) {
                    if (comma) {
                        oss << ", ";
                    } else {
                        comma = true;
                    }
                    oss << *sym;
                }
                oss << ").";
                flush();
            }
            void do_meta(std::vector<Stm> const &stms) override {
                for (auto const &stm : stms) {
                    oss << stm;
                    flush();
                }
            }
            void do_fact(std::vector<Symbol> const &facts) override {
                for (auto const &fact : facts) {
                    oss << fact << ".";
                    flush();
                }
            }
            auto do_components(Components const &comps) -> bool override {
                for (auto const &ref_comps : comps) {
                    oss << "% component";
                    flush();
                    for (auto const &ref_comp : ref_comps) {
                        oss << "% refined component";
                        flush();
                        for (auto const &stm : ref_comp.stms) {
                            oss << *stm;
                            flush();
                        }
                    }
                }
                return true;
            }
            std::ostringstream oss;
            std::vector<std::string> res;
        };
        auto bld = Builder{};
        auto ph = ParseHelper();
        auto uprg = UnprocessedProgram{};
        uprg.add(ph, opt_value(ph.statement("#show p/2.")));
        uprg.add(ph, opt_value(ph.statement("#program p(a).")));
        uprg.add(ph, opt_value(ph.statement("p(a).")));
        uprg.add(ph, opt_value(ph.statement("p(b).")));
        uprg.add(ph, opt_value(ph.statement("p(X,a) :- f(X).")));
        auto prg = Program{ph.ctx().options()};
        prg.join(ph, ph, uprg);
        std::ignore = prg.analyze(ph, {ProgramParam{ph.store().string("p"), {ph.store().num(Number(1))}}}, bld);
        REQUIRE(bld.res == std::vector<std::string>{"#show p/2.", "p(b).", "#program_p(1).", "% component",
                                                    "% refined component", "p($0) :- #program_p($0).", "% component",
                                                    "% refined component", "p(X,$0) :- #program_p($0); f(X)."});
    }
}

} // namespace CppClingo::Input::Test
