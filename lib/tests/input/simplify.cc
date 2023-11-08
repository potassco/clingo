#include <input/algo/simplify.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

auto simplify_str(std::optional<Term> value, SimplifyFlags flags = SimplifyFlags::projectable) -> std::string {
    if (value) {
        Logger log;
        auto store = make_symbol_store(true, true);
        NameGen gen{*store, {}, "__Aux_"};
        AuxTermVec aux;
        std::ostringstream oss;
        std::visit(
            [&](auto &&res) {
                GRINGO_MATCH(res, SimplifyFail) { oss << "<undefined>"; }
                GRINGO_MATCH(res, SimplifyUnchanged) { oss << value.value(); }
                GRINGO_MATCH(res, Term) { oss << res; }
            },
            simplify(flags, {log, *store, gen, aux}, value.value()));
        for (auto const &[lhs, rhs] : aux) {
            oss << ", " << lhs << "=" << rhs;
        }
        if (log.has_error()) {
            oss << ", E";
        }
        return std::move(oss).str();
    }
    return "<failed>";
}

template <class T>
auto simplify_str(std::optional<T> value, SimplifyFlags flags = SimplifyFlags::projectable) -> std::string {
    if (value) {
        Logger log;
        auto store = make_symbol_store(true, true);
        NameGen gen{*store, {}, "__Aux_"};
        AuxTermVec aux;
        std::ostringstream oss;
        auto [state, res] = simplify(flags, {log, *store, gen, aux}, value.value());
        if (res.has_value()) {
            oss << res.value();
        } else {
            oss << "<unchanged>";
        }
        for (auto const &[lhs, rhs] : aux) {
            oss << ", " << lhs << "=" << rhs;
        }
        switch (state) {
            case SimplifyState::top: {
                oss << ", T";
                break;
            }
            case SimplifyState::bot: {
                oss << ", B";
                break;
            }
            case SimplifyState::unknown: {
                oss << ", U";
                break;
            }
            case SimplifyState::fail: {
                oss << ", F";
                break;
            }
        }
        if (log.has_error()) {
            oss << ", E";
        }
        return std::move(oss).str();
    }
    return "<failed>";
}

TEST_CASE("simplify_unary") {
    // numeric
    REQUIRE(simplify_str(parse_term("-1")) == "-1");
    REQUIRE(simplify_str(parse_term("-X+1")) == "-1*X+1");
    REQUIRE(simplify_str(parse_term("|-1|")) == "1");
    // any
    REQUIRE(simplify_str(parse_term("-X")) == "-X");
    REQUIRE(simplify_str(parse_term("--X")) == "-(-X)");
    REQUIRE(simplify_str(parse_term("---X")) == "-X");
    REQUIRE(simplify_str(parse_term("|-X|")) == "|-X|");
    REQUIRE(simplify_str(parse_term("|1*X+0|")) == "|X|");
    // symbolic
    REQUIRE(simplify_str(parse_term("--f")) == "f");
    REQUIRE(simplify_str(parse_term("---f")) == "-f");
    REQUIRE(simplify_str(parse_term("-f(-|X|)")) == "-f(-|X|)");
    // fail
    REQUIRE(simplify_str(parse_term("~a")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("-(1,2)")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("-(1,X)")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("|()|")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("|(X,)|")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("|f|")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("|f(X)|")) == "<undefined>");
}

TEST_CASE("simplify_binary") {
    // evaluate constant
    REQUIRE(simplify_str(parse_term("1+2")) == "3");
    // keep variables
    REQUIRE(simplify_str(parse_term("X")) == "X");
    // variable to linear
    REQUIRE(simplify_str(parse_term("X+0")) == "1*X+0");
    // linear + constant
    REQUIRE(simplify_str(parse_term("(2*X+3)+2")) == "2*X+5");
    REQUIRE(simplify_str(parse_term("(2*X+3)-2")) == "2*X+1");
    REQUIRE(simplify_str(parse_term("(2*X+3)*2")) == "4*X+6");
    REQUIRE(simplify_str(parse_term("(2*X+3)/2")) == "(2*X+3)/2");
    REQUIRE(simplify_str(parse_term("(1*X+0)/2")) == "X/2");
    REQUIRE(simplify_str(parse_term("(1*X+0)*0")) == "X*0");
    // constant + linear
    REQUIRE(simplify_str(parse_term("2*(2*X+3)")) == "4*X+6");
    REQUIRE(simplify_str(parse_term("2+(2*X+3)")) == "2*X+5");
    REQUIRE(simplify_str(parse_term("2-(2*X+3)")) == "-2*X+(-1)");
    REQUIRE(simplify_str(parse_term("2/(2*X+3)")) == "2/(2*X+3)");
    REQUIRE(simplify_str(parse_term("2/(1*X+0)")) == "2/X");
    REQUIRE(simplify_str(parse_term("0*(1*X+0)")) == "0*X");
    // linear + linear
    REQUIRE(simplify_str(parse_term("(2*X+3)+(3*X+5)")) == "5*X+8");
    REQUIRE(simplify_str(parse_term("(2*X+3)-(3*X+5)")) == "-1*X+(-2)");
    REQUIRE(simplify_str(parse_term("(2*X+3)-(2*X+5)")) == "0*X+(-2)");
    REQUIRE(simplify_str(parse_term("(2*X+3)+(3*Y+5)")) == "2*X+(3*Y+8)");
    REQUIRE(simplify_str(parse_term("(2*X+3)-(3*Y+5)")) == "2*X-(3*Y+2)");
    // unchanged + unchanged
    REQUIRE(simplify_str(parse_term("(X/2)-(Y/2)")) == "X/2-Y/2");
    // changed + changed
    REQUIRE(simplify_str(parse_term("(X/(2+0))-(Y/(2+0))")) == "X/2-Y/2");
    // fail
    REQUIRE(simplify_str(parse_term("1+a")) == "<undefined>");
}

TEST_CASE("simplify_symbolic") {
    REQUIRE(simplify_str(parse_term("-f(-|1-2|)")) == "-f(-1)");
    REQUIRE(simplify_str(parse_term("-f(1+2+X,-X)")) == "-f(1*X+3,-X)");
    REQUIRE(simplify_str(parse_term("(1+2+X,-X)")) == "(1*X+3,-X)");
    REQUIRE(simplify_str(parse_term("f(1+a)")) == "<undefined>");
    REQUIRE(simplify_str(parse_term("f(X+a)")) == "<undefined>");
}

TEST_CASE("simplify_aux") {
    REQUIRE(simplify_str(parse_term("1..2")) == "1*__Aux_0+0, __Aux_0=1..2");
    REQUIRE(simplify_str(parse_term("f(1..2)")) == "f(1*__Aux_0+0), __Aux_0=1..2");
    REQUIRE(simplify_str(parse_term("@f")) == "__Aux_0, __Aux_0=@f");
    REQUIRE(simplify_str(parse_term("@f(@g(1+2))")) == "__Aux_1, __Aux_0=@g(3), __Aux_1=@f(__Aux_0)");
}

TEST_CASE("simplify_project") {
    REQUIRE(simplify_str(parse_term("f(*,(*,b))")) == "f(*,(*,b))");
    REQUIRE(simplify_str(parse_term("@f(g(*))")) == "<undefined>, E");
    REQUIRE(simplify_str(parse_term("f(*)"), SimplifyFlags::none) == "<undefined>, E");
}

TEST_CASE("simplify_matchable") {
    auto flags = SimplifyFlags::matchable | SimplifyFlags::unfailable;
    REQUIRE(simplify_str(parse_term("f(1,X+Y)"), flags) == "f(1,__Aux_0), __Aux_0=X+Y");
    REQUIRE(simplify_str(parse_term("f(1,2*X)"), flags) == "f(1,__Aux_0), __Aux_0=2*X+0");
    REQUIRE(simplify_str(parse_term("p(X+5,@f(g(X*X)))"), flags) ==
            "p(__Aux_1,__Aux_0), __Aux_0=@f(g(X*X)), __Aux_1=1*X+5");

    flags &= ~SimplifyFlags::unfailable;
    REQUIRE(simplify_str(parse_term("f(1,X+Y)"), flags) == "f(1,__Aux_0), __Aux_0=X+Y");
    REQUIRE(simplify_str(parse_term("f(1,2*X)"), flags) == "f(1,2*X+0)");
    REQUIRE(simplify_str(parse_term("p(X+5,@f(g(X*X)))"), flags) == "p(1*X+5,__Aux_0), __Aux_0=@f(g(X*X))");
}

TEST_CASE("simplify_literal") {
    auto flags = SimplifyFlags::matchable;
    REQUIRE(simplify_str(parse_literal("1<2"), flags) == "#true, T");
    REQUIRE(simplify_str(parse_literal("2<1"), flags) == "#false, B");
    REQUIRE(simplify_str(parse_literal("X=Y+Z"), flags) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_literal("X=Y+Z=Z"), flags) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_literal("not not X=Y+Z=Z"), flags) == "X=Y+Z=Z, U");
    REQUIRE(simplify_str(parse_literal("not X=Y+Z=Z"), flags) == "not X=__Aux_0=Z, __Aux_0=Y+Z, U");
    REQUIRE(simplify_str(parse_literal("X=f(Y+Z,Z+5)<f(Y+Z,Z+5)"), flags) ==
            "X=f(__Aux_0,1*Z+5)<f(Y+Z,1*Z+5), __Aux_0=Y+Z, U");
    REQUIRE(simplify_str(parse_literal("f(X,*)<f(Y)"), flags) == "<unchanged>, F, E");
    REQUIRE(simplify_str(parse_literal("not f(X,*)<f(Y)"), flags) == "<unchanged>, F, E");

    flags = SimplifyFlags::matchable | SimplifyFlags::projectable;
    REQUIRE(simplify_str(parse_literal("p(X,*)"), flags) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_literal("p(X,@f(*))"), flags) == "<unchanged>, F, E");
    REQUIRE(simplify_str(parse_literal("not p(X,@f(*))"), flags) == "<unchanged>, F, E");
    REQUIRE(simplify_str(parse_literal("p(X+Y,Y+1)"), flags) == "p(__Aux_0,1*Y+1), __Aux_0=X+Y, U");
    REQUIRE(simplify_str(parse_literal("not p(X+Y,Y+1)"), flags) == "not p(X+Y,1*Y+1), U");
    REQUIRE(simplify_str(parse_literal("not not p(X+Y,Y+1)"), flags) == "not not p(X+Y,1*Y+1), U");
    REQUIRE(simplify_str(parse_literal("p(X,1*Y+1)"), flags) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_literal("not p(X+Y,1*Y+1)"), flags) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_literal("not not p(X+Y,1*Y+1)"), flags) == "<unchanged>, U");
}

} // namespace Gringo::Input::Test
