#include <input/algo/simplify.hh>
#include <input/algo/unpool.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

template <typename T>
auto call_simplify(SimplifyFlags flags, RewriteContext &ctx, T const &x) -> decltype(simplify(flags, ctx, x)) {
    auto res = unpool(ctx, x);
    if (!res.has_value()) {
        res = Util::make_vec<T>(x);
    }
    return simplify(flags, ctx, res->at(0));
}
template <typename T>
auto call_simplify(SimplifyFlags flags, RewriteContext &ctx, T const &x) -> decltype(simplify(ctx, x)) {
    static_cast<void>(flags);
    auto res = unpool(ctx, x);
    if (!res.has_value()) {
        res = Util::make_vec<T>(x);
    }
    return simplify(ctx, res->at(0));
}

template <class T>
auto simplify_str(std::optional<T> value, SimplifyFlags flags = SimplifyFlags::projectable) -> std::string {
    if (value) {
        Logger log;
        auto store = make_symbol_store(true, true);
        auto ctx = RewriteContext{log, *store, {}, "__A_"};
        auto guard = std::is_same_v<T, Statement> ? nullptr : ctx.push();
        auto [state, res] = call_simplify(flags, ctx, value.value());
        std::ostringstream oss;
        if (res.has_value()) {
            oss << res.value();
        } else {
            oss << "<unchanged>";
        }
        if (guard != nullptr) {
            for (auto const &[lhs, rhs] : ctx.aux()) {
                oss << ", " << lhs << "=" << rhs;
            }
        }
        if constexpr (std::is_same_v<decltype(state), bool>) {
            oss << ", " << (state ? "U" : "F");
        } else {
            switch (state) {
                case TruthValue::top: {
                    oss << ", T";
                    break;
                }
                case TruthValue::bot: {
                    oss << ", B";
                    break;
                }
                case TruthValue::unknown: {
                    oss << ", U";
                    break;
                }
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
    REQUIRE(simplify_str(parse_term("1..2")) == "1*__A_0+0, __A_0=1..2, U");
    REQUIRE(simplify_str(parse_term("f(1..2)")) == "f(1*__A_0+0), __A_0=1..2, U");
    REQUIRE(simplify_str(parse_term("@f")) == "__A_0, __A_0=@f, U");
    REQUIRE(simplify_str(parse_term("@f(@g(1+2))")) == "__A_1, __A_0=@g(3), __A_1=@f(__A_0), U");
}

TEST_CASE("simplify_project") {
    REQUIRE(simplify_str(parse_term("f(*,(*,b))")) == "f(*,(*,b)), U");
    REQUIRE(simplify_str(parse_term("@f(g(*))")) == "<unchanged>, F, E");
    REQUIRE(simplify_str(parse_term("f(*)"), SimplifyFlags::none) == "<unchanged>, F, E");
}

TEST_CASE("simplify_matchable") {
    auto flags = SimplifyFlags::matchable | SimplifyFlags::unfailable;
    REQUIRE(simplify_str(parse_term("f(1,X+Y)"), flags) == "f(1,__A_0), __A_0=X+Y, U");
    REQUIRE(simplify_str(parse_term("f(1,2*X)"), flags) == "f(1,__A_0), __A_0=2*X+0, U");
    REQUIRE(simplify_str(parse_term("p(X+5,@f(g(X*X)))"), flags) == "p(__A_1,__A_0), __A_0=@f(g(X*X)), __A_1=1*X+5, U");

    flags &= ~SimplifyFlags::unfailable;
    REQUIRE(simplify_str(parse_term("f(1,X+Y)"), flags) == "f(1,1*__A_0+0), __A_0=X+Y, U");
    REQUIRE(simplify_str(parse_term("f(1,2*X)"), flags) == "f(1,2*X+0), U");
    REQUIRE(simplify_str(parse_term("p(X+5,@f(g(X*X)))"), flags) == "p(1*X+5,__A_0), __A_0=@f(g(X*X)), U");
}

TEST_CASE("simplify_literal") {
    auto flags = SimplifyFlags::matchable;
    REQUIRE(simplify_str(parse_literal("1<2"), flags) == "#true, T");
    REQUIRE(simplify_str(parse_literal("2<1"), flags) == "#false, B");
    REQUIRE(simplify_str(parse_literal("X=Y+Z"), flags) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_literal("X=Y+Z=Z"), flags) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_literal("not not X=Y+Z=Z"), flags) == "X=Y+Z=Z, U");
    REQUIRE(simplify_str(parse_literal("not X=Y+Z=Z"), flags) == "not X=__A_0=Z, __A_0=Y+Z, U");
    REQUIRE(simplify_str(parse_literal("X=f(Y+Z,Z+5)<f(Y+Z,Z+5)"), flags) ==
            "X=f(1*__A_0+0,1*Z+5)<f(Y+Z,1*Z+5), __A_0=Y+Z, U");
    REQUIRE(simplify_str(parse_literal("f(X,*)<f(Y)"), flags) == "#false, B, E");
    REQUIRE(simplify_str(parse_literal("not f(X,*)<f(Y)"), flags) == "#false, B, E");

    REQUIRE(simplify_str(parse_literal("X=2<1"), flags) == "#false, B");
    REQUIRE(simplify_str(parse_literal("X=2>1"), flags) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_literal("not X!=2<1"), flags) == "#true, T");
    REQUIRE(simplify_str(parse_literal("not X!=2>1"), flags) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_literal("1<2<3<4"), flags) == "#true, T");
    REQUIRE(simplify_str(parse_literal("not 1<2<3<4"), flags) == "#false, B");

    flags = SimplifyFlags::matchable | SimplifyFlags::projectable;
    REQUIRE(simplify_str(parse_literal("p(X,*)"), flags) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_literal("p(X,@f(*))"), flags) == "#false, B, E");
    REQUIRE(simplify_str(parse_literal("not p(X,@f(*))"), flags) == "#false, B, E");
    REQUIRE(simplify_str(parse_literal("p(X+Y,Y+1)"), flags) == "p(1*__A_0+0,1*Y+1), __A_0=X+Y, U");
    REQUIRE(simplify_str(parse_literal("not p(X+Y,Y+1)"), flags) == "not p(X+Y,1*Y+1), U");
    REQUIRE(simplify_str(parse_literal("not not p(X+Y,Y+1)"), flags) == "not not p(X+Y,1*Y+1), U");
    REQUIRE(simplify_str(parse_literal("p(X,1*Y+1)"), flags) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_literal("not p(X+Y,1*Y+1)"), flags) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_literal("not not p(X+Y,1*Y+1)"), flags) == "<unchanged>, U");
}

TEST_CASE("simplify_bug") {
    REQUIRE(simplify_str(parse_statement("X=Y+Z=Z: cond.")) == "#or { X=__A_0=Z, __A_0!=Y+Z: cond }., U");
}

TEST_CASE("simplify_head_cond_lit") {
    REQUIRE(simplify_str(parse_statement("X=Y+Z=Z: cond.")) == "#or { X=__A_0=Z, __A_0!=Y+Z: cond }., U");
    REQUIRE(simplify_str(parse_statement("not not X=Y+Z=Z: cond.")) == "#or { X=__A_0=Z, __A_0!=Y+Z: cond }., U");
    REQUIRE(simplify_str(parse_statement("not X=Y+Z=Z: cond.")) == "not X=Y+Z=Z: cond., U");
    REQUIRE(simplify_str(parse_statement("not X=Y+Z=Z: cond(Z).")) == "<unchanged>, U");
}

TEST_CASE("simplify_body_cond_lit") {
    REQUIRE(simplify_str(parse_statement("x :- X=Y+Z=Z: cond.")) == "x :- X=Y+Z=Z: cond., U");
    REQUIRE(simplify_str(parse_statement("x(X) :- X=Y+Z=Z: cond(Z).")) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_statement("x :- not not X=Y+Z=Z: cond.")) == "x :- X=Y+Z=Z: cond., U");
    REQUIRE(simplify_str(parse_statement("x :- not X=Y+Z=Z: cond.")) ==
            "x :- #and { not X=__A_0=Z, __A_0=Y+Z: cond }., U");
}

TEST_CASE("simplify_head_set_aggregate") {
    // it looks like set aggregates have to be rewritten before unpooling!
    // the rewriting can be applied before unpooling
    REQUIRE(simplify_str(parse_statement("{ not 2<1<X: p(X) }.")) == "#count { 3,X: #true: p(X) }., T");
    REQUIRE(simplify_str(parse_statement("X+a <= {a} :- x.")) == "#true., T");
    REQUIRE(simplify_str(parse_statement("X+Y <= {a}.")) == "X+Y <= #count { 0,a: a }., U");
    REQUIRE(simplify_str(parse_statement("@f(X) <= {a}.")) == "__A_0 <= #count { 0,a: a } :- __A_0=@f(X)., U");
    REQUIRE(simplify_str(parse_statement("{a; not a; not not a; X+Y<Z: p(U)} <= 1.")) ==
            "#count { 0,a: a; 1,a: not a; 2,a: not not a; 3,X,Y,Z: X+Y<Z: p(U) } <= 1., U");
    REQUIRE(simplify_str(parse_statement("1..2 <= {a(3..4,X+Y): b(A+B)} <= 5..6.")) ==
            "1*__A_0+0 <= #count { "
            "0,a(__A_2,__A_3): a(__A_2,__A_3): "
            "b(1*__A_4+0), __A_2=3..4, __A_3=X+Y, __A_4=A+B "
            "} <= 1*__A_1+0 :- __A_0=1..2; __A_1=5..6., U");
    REQUIRE(simplify_str(parse_statement("X <= {1!=2;2!=2;#true;#false} <= Y.")) == "X<=2<=Y., U");
    REQUIRE(simplify_str(parse_statement("1 <= {1!=2;2!=2;#true;#false} <= 2.")) == "#true., T");
    REQUIRE(simplify_str(parse_statement("1 <= {1!=2;2!=2;#true;#false} <= 1.")) == "#false., B");
}

TEST_CASE("simplify_body_set_aggregate") {
    REQUIRE(simplify_str(parse_statement("x :- X+a <= {a}.")) == "#true., T");
    REQUIRE(simplify_str(parse_statement("x :- X+Y <= {a}.")) == "x :- X+Y <= #count { 0,a: a }., U");
    REQUIRE(simplify_str(parse_statement("x :- @f(X) <= {a}.")) == "x :- __A_0 <= #count { 0,a: a }; __A_0=@f(X)., U");
    REQUIRE(simplify_str(parse_statement("x :- {a; not a; not not a; X+Y<Z: p(U)} <= 1.")) ==
            "x :- #count { 0,a: a; 1,a: not a; 2,a: not not a; 3,X,Y,Z: p(U), X+Y<Z } <= 1., U");
    REQUIRE(simplify_str(parse_statement("x :- 1..2 <= {a(3..4,X+Y): b(A+B)} <= 5..6.")) ==
            "x :- 1*__A_0+0 <= #count { "
            "0,a(1*__A_2+0,1*__A_3+0): "
            "b(1*__A_4+0), __A_2=3..4, __A_3=X+Y, __A_4=A+B, a(1*__A_2+0,1*__A_3+0) "
            "} <= 1*__A_1+0; __A_0=1..2; __A_1=5..6., U");
    REQUIRE(simplify_str(parse_statement("x :- X <= {1!=2;2!=2;#true;#false} <= Y.")) == "x :- X<=2<=Y., U");
    REQUIRE(simplify_str(parse_statement("x :- 1 <= {1!=2;2!=2;#true;#false} <= 2.")) == "x., U");
    REQUIRE(simplify_str(parse_statement("x :- 1 <= {1!=2;2!=2;#true;#false} <= 1.")) == "#true., T");
}

TEST_CASE("simplify_head_aggregate") {}

TEST_CASE("simplify_body_aggregate") {}

TEST_CASE("simplify_rule") {
    REQUIRE(simplify_str(parse_statement("p(X+1) :- q(2*X,Y+Z).")) == "p(1*X+1) :- q(2*X+0,1*__A_0+0); __A_0=Y+Z., U");
    REQUIRE(simplify_str(parse_statement("p(X+1) | p(X+Y).")) == "p(1*X+1); p(X+Y)., U");
    REQUIRE(simplify_str(parse_statement("p(X) | p(X+Y).")) == "<unchanged>, U");
    REQUIRE(simplify_str(parse_statement("p(X+1) | p(X+Y): q(X+Y).")) ==
            "p(1*X+1); p(X+Y): q(1*__A_0+0), __A_0=X+Y., U");
    REQUIRE(simplify_str(parse_statement("#false | p(X+Y): q(X+Y).")) == "p(X+Y): q(1*__A_0+0), __A_0=X+Y., U");
    REQUIRE(simplify_str(parse_statement("#false | #true: q(X+Y).")) == "#true: q(1*__A_0+0), __A_0=X+Y., U");
    REQUIRE(simplify_str(parse_statement("#false | #false: q(X+Y).")) == "#false., B");
    REQUIRE(simplify_str(parse_statement("#true | p(X+Y): q(X+Y).")) == "#true., T");
    REQUIRE(simplify_str(parse_statement("x :- #and { #true; p(X+Y): q(X+Y) }.")) ==
            "x :- #and { p(1*__A_0+0), __A_0=X+Y: q(1*__A_1+0), __A_1=X+Y }., U");
    REQUIRE(simplify_str(parse_statement("x :- #and { #true; not p(X+Y): q(X+Y) }.")) ==
            "x :- not p(X+Y): q(1*__A_0+0), __A_0=X+Y., U");
    REQUIRE(simplify_str(parse_statement("x :- #and { #true; #false: q(X+Y) }.")) ==
            "x :- #false: q(1*__A_0+0), __A_0=X+Y., U");
    REQUIRE(simplify_str(parse_statement("x :- #and { #true; #true: q(X+Y) }.")) == "x., U");
    REQUIRE(simplify_str(parse_statement("x :- #and { #false; p(X+Y): q(X+Y) }.")) == "#true., T");
    REQUIRE(simplify_str(parse_statement("#false.")) == "<unchanged>, B");
    REQUIRE(simplify_str(parse_statement("#false :- #true.")) == "#false., B");
}

} // namespace Gringo::Input::Test
