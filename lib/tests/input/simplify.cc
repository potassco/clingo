#include <input/algo/simplify.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

template <typename T>
auto call_simplify(SimplifyFlags flags, SimplifyContext ctx, T const &x) -> decltype(simplify(flags, ctx, x)) {
    return simplify(flags, ctx, x);
}
template <typename T>
auto call_simplify(SimplifyFlags flags, SimplifyContext ctx, T const &x) -> decltype(simplify(ctx, x)) {
    static_cast<void>(flags);
    return simplify(ctx, x);
}
auto call_simplify(SimplifyFlags flags, SimplifyContext ctx, Statement const &x) -> SimplifyResult<Statement> {
    static_cast<void>(flags);
    return simplify(ctx.log, ctx.store, x);
}

template <class T>
auto simplify_str(std::optional<T> value, SimplifyFlags flags = SimplifyFlags::projectable) -> std::string {
    if (value) {
        Logger log;
        auto store = make_symbol_store(true, true);
        NameGen gen{*store, {}, "__Aux_"};
        AuxTermVec aux;
        std::ostringstream oss;
        auto [state, res] = call_simplify(flags, {log, *store, gen, aux}, value.value());
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
    REQUIRE(simplify_str(parse_term("-1")) == "-1, U");
    REQUIRE(simplify_str(parse_term("-X+1")) == "-1*X+1, U");
    REQUIRE(simplify_str(parse_term("|-1|")) == "1, U");
    // any
    REQUIRE(simplify_str(parse_term("-X")) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_term("--X")) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_term("---X")) == "-X, U");
    REQUIRE(simplify_str(parse_term("|-X|")) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_term("|1*X+0|")) == "|X|, U");
    // symbolic
    REQUIRE(simplify_str(parse_term("--f")) == "f, U");
    REQUIRE(simplify_str(parse_term("---f")) == "-f, U");
    REQUIRE(simplify_str(parse_term("-f(-|X|)")) == "<unchanged>, U");
    // fail
    REQUIRE(simplify_str(parse_term("~a")) == "<unchanged>, F");
    REQUIRE(simplify_str(parse_term("-(1,2)")) == "<unchanged>, F");
    REQUIRE(simplify_str(parse_term("-(1,X)")) == "<unchanged>, F");
    REQUIRE(simplify_str(parse_term("|()|")) == "<unchanged>, F");
    REQUIRE(simplify_str(parse_term("|(X,)|")) == "<unchanged>, F");
    REQUIRE(simplify_str(parse_term("|f|")) == "<unchanged>, F");
    REQUIRE(simplify_str(parse_term("|f(X)|")) == "<unchanged>, F");
}

TEST_CASE("simplify_binary") {
    // evaluate constant
    REQUIRE(simplify_str(parse_term("1+2")) == "3, U");
    // keep variables
    REQUIRE(simplify_str(parse_term("X")) == "<unchanged>, U");
    // variable to linear
    REQUIRE(simplify_str(parse_term("X+0")) == "1*X+0, U");
    // linear + constant
    REQUIRE(simplify_str(parse_term("(2*X+3)+2")) == "2*X+5, U");
    REQUIRE(simplify_str(parse_term("(2*X+3)-2")) == "2*X+1, U");
    REQUIRE(simplify_str(parse_term("(2*X+3)*2")) == "4*X+6, U");
    REQUIRE(simplify_str(parse_term("(2*X+3)/2")) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_term("(1*X+0)/2")) == "X/2, U");
    REQUIRE(simplify_str(parse_term("(1*X+0)*0")) == "X*0, U");
    // constant + linear
    REQUIRE(simplify_str(parse_term("2*(2*X+3)")) == "4*X+6, U");
    REQUIRE(simplify_str(parse_term("2+(2*X+3)")) == "2*X+5, U");
    REQUIRE(simplify_str(parse_term("2-(2*X+3)")) == "-2*X+(-1), U");
    REQUIRE(simplify_str(parse_term("2/(2*X+3)")) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_term("2/(1*X+0)")) == "2/X, U");
    REQUIRE(simplify_str(parse_term("0*(1*X+0)")) == "0*X, U");
    // linear + linear
    REQUIRE(simplify_str(parse_term("(2*X+3)+(3*X+5)")) == "5*X+8, U");
    REQUIRE(simplify_str(parse_term("(2*X+3)-(3*X+5)")) == "-1*X+(-2), U");
    REQUIRE(simplify_str(parse_term("(2*X+3)-(2*X+5)")) == "0*X+(-2), U");
    REQUIRE(simplify_str(parse_term("(2*X+3)+(3*Y+5)")) == "2*X+(3*Y+8), U");
    REQUIRE(simplify_str(parse_term("(2*X+3)-(3*Y+5)")) == "2*X-(3*Y+2), U");
    // unchanged + unchanged
    REQUIRE(simplify_str(parse_term("(X/2)-(Y/2)")) == "<unchanged>, U");
    // changed + changed
    REQUIRE(simplify_str(parse_term("(X/(2+0))-(Y/(2+0))")) == "X/2-Y/2, U");
    // fail
    REQUIRE(simplify_str(parse_term("1+a")) == "<unchanged>, F");
}

TEST_CASE("simplify_symbolic") {
    REQUIRE(simplify_str(parse_term("-f(-|1-2|)")) == "-f(-1), U");
    REQUIRE(simplify_str(parse_term("-f(1+2+X,-X)")) == "-f(1*X+3,-X), U");
    REQUIRE(simplify_str(parse_term("(1+2+X,-X)")) == "(1*X+3,-X), U");
    REQUIRE(simplify_str(parse_term("f(1+a)")) == "<unchanged>, F");
    REQUIRE(simplify_str(parse_term("f(X+a)")) == "<unchanged>, F");
}

TEST_CASE("simplify_aux") {
    REQUIRE(simplify_str(parse_term("1..2")) == "1*__Aux_0+0, __Aux_0=1..2, U");
    REQUIRE(simplify_str(parse_term("f(1..2)")) == "f(1*__Aux_0+0), __Aux_0=1..2, U");
    REQUIRE(simplify_str(parse_term("@f")) == "__Aux_0, __Aux_0=@f, U");
    REQUIRE(simplify_str(parse_term("@f(@g(1+2))")) == "__Aux_1, __Aux_0=@g(3), __Aux_1=@f(__Aux_0), U");
}

TEST_CASE("simplify_project") {
    REQUIRE(simplify_str(parse_term("f(*,(*,b))")) == "f(*,(*,b)), U");
    REQUIRE(simplify_str(parse_term("@f(g(*))")) == "<unchanged>, F, E");
    REQUIRE(simplify_str(parse_term("f(*)"), SimplifyFlags::none) == "<unchanged>, F, E");
}

TEST_CASE("simplify_matchable") {
    auto flags = SimplifyFlags::matchable | SimplifyFlags::unfailable;
    REQUIRE(simplify_str(parse_term("f(1,X+Y)"), flags) == "f(1,__Aux_0), __Aux_0=X+Y, U");
    REQUIRE(simplify_str(parse_term("f(1,2*X)"), flags) == "f(1,__Aux_0), __Aux_0=2*X+0, U");
    REQUIRE(simplify_str(parse_term("p(X+5,@f(g(X*X)))"), flags) ==
            "p(__Aux_1,__Aux_0), __Aux_0=@f(g(X*X)), __Aux_1=1*X+5, U");

    flags &= ~SimplifyFlags::unfailable;
    REQUIRE(simplify_str(parse_term("f(1,X+Y)"), flags) == "f(1,__Aux_0), __Aux_0=X+Y, U");
    REQUIRE(simplify_str(parse_term("f(1,2*X)"), flags) == "f(1,2*X+0), U");
    REQUIRE(simplify_str(parse_term("p(X+5,@f(g(X*X)))"), flags) == "p(1*X+5,__Aux_0), __Aux_0=@f(g(X*X)), U");
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

TEST_CASE("simplify_rule") {
    // TODO: consider using 1*__A_0+0 in the rule
    REQUIRE(simplify_str(parse_statement("p(X+1) :- q(2*X,Y+Z).")) == "p(1*X+1) :- q(2*X+0,__A_0); __A_0=Y+Z., U");
}

} // namespace Gringo::Input::Test
