#include "test.hh"

#include <clingo/input/rewrite/simplify.hh>
#include <clingo/input/rewrite/unpool.hh>

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Input::Test {

namespace {

template <typename F, typename T>
auto call_simplify_(F flags, RewriteContext &ctx, T const &x) -> decltype(simplify(flags, ctx, x)) {
    return simplify(flags, ctx, x);
}
template <typename F, typename T>
auto call_simplify_([[maybe_unused]] F flags, RewriteContext &ctx, T const &x) -> decltype(simplify(ctx, x)) {
    return simplify(ctx, x);
}

template <class F, class T> auto simplify_str_(ParseHelper &ph, std::optional<T> value, F flags) -> std::string {
    if (value) {
        auto &ctx = ph.ctx();
        auto guard = std::is_same_v<T, Stm> ? nullptr : ctx.push();
        auto ures = unpool(ctx, value.value());
        bool changed = ures.has_value();
        if (!changed) {
            ures = Util::make_vec<T>(value.value());
        }
        auto [state, res] = call_simplify_(flags, ctx, ures->at(0));
        if (changed && !res) {
            res = ures->at(0);
        }
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
        return std::move(oss).str();
    }
    return "<failed>";
}

auto simplify_term(std::string const &str, SimplifyTermFlags flags = SimplifyTermFlags::none) -> std::string {
    ParseHelper ph;
    return simplify_str_(ph, ph.term(str), flags);
}

auto simplify_literal(std::string const &str, SimplifyLiteralFlags flags = SimplifyLiteralFlags::none) -> std::string {
    ParseHelper ph;
    return simplify_str_(ph, ph.literal(str), flags);
}

auto simplify_statement(std::string const &str) -> std::string {
    ParseHelper ph;
    return simplify_str_(ph, ph.statement(str), 0);
}

} // namespace

TEST_CASE("simplify_unary") {
    // numeric
    REQUIRE(simplify_term("-1") == "-1, U");
    REQUIRE(simplify_term("-X+1") == "-1*X+1, U");
    REQUIRE(simplify_term("|-1|") == "1, U");
    // any
    REQUIRE(simplify_term("-X") == "<unchanged>, U");
    REQUIRE(simplify_term("--X") == "<unchanged>, U");
    REQUIRE(simplify_term("---X") == "-X, U");
    REQUIRE(simplify_term("|-X|") == "<unchanged>, U");
    REQUIRE(simplify_term("|1*X+0|") == "|X|, U");
    // symbolic
    REQUIRE(simplify_term("--f") == "f, U");
    REQUIRE(simplify_term("---f") == "-f, U");
    REQUIRE(simplify_term("-f(-|X|)") == "<unchanged>, U");
    // fail
    REQUIRE(simplify_term("~a") == "<unchanged>, F");
    REQUIRE(simplify_term("-(1,2)") == "<unchanged>, F");
    REQUIRE(simplify_term("-(1,X)") == "<unchanged>, F");
    REQUIRE(simplify_term("|()|") == "<unchanged>, F");
    REQUIRE(simplify_term("|(X,)|") == "<unchanged>, F");
    REQUIRE(simplify_term("|f|") == "<unchanged>, F");
    REQUIRE(simplify_term("|f(X)|") == "<unchanged>, F");
}

TEST_CASE("simplify_binary") {
    // evaluate constant
    REQUIRE(simplify_term("1+2") == "3, U");
    // keep variables
    REQUIRE(simplify_term("X") == "<unchanged>, U");
    // variable to linear
    REQUIRE(simplify_term("X+0") == "1*X+0, U");
    // linear + constant
    REQUIRE(simplify_term("(2*X+3)+2") == "2*X+5, U");
    REQUIRE(simplify_term("(2*X+3)-2") == "2*X+1, U");
    REQUIRE(simplify_term("(2*X+3)*2") == "4*X+6, U");
    REQUIRE(simplify_term("(2*X+3)/2") == "(2*X+3)/2, U");
    REQUIRE(simplify_term("(1*X+0)/2") == "X/2, U");
    REQUIRE(simplify_term("(1*X+0)*0") == "X*0, U");
    // constant + linear
    REQUIRE(simplify_term("2*(2*X+3)") == "4*X+6, U");
    REQUIRE(simplify_term("2+(2*X+3)") == "2*X+5, U");
    REQUIRE(simplify_term("2-(2*X+3)") == "-2*X+(-1), U");
    REQUIRE(simplify_term("2/(2*X+3)") == "2/(2*X+3), U");
    REQUIRE(simplify_term("2/(1*X+0)") == "2/X, U");
    REQUIRE(simplify_term("0*(1*X+0)") == "0*X, U");
    // linear + linear
    REQUIRE(simplify_term("(2*X+3)+(3*X+5)") == "5*X+8, U");
    REQUIRE(simplify_term("(2*X+3)-(3*X+5)") == "-1*X+(-2), U");
    REQUIRE(simplify_term("(2*X+3)-(2*X+5)") == "0*X+(-2), U");
    REQUIRE(simplify_term("(2*X+3)+(3*Y+5)") == "2*X+(3*Y+8), U");
    REQUIRE(simplify_term("(2*X+3)-(3*Y+5)") == "2*X-(3*Y+2), U");
    // unchanged + unchanged
    REQUIRE(simplify_term("X/2-Y/2") == "<unchanged>, U");
    // changed + changed
    REQUIRE(simplify_term("(X/(2+0))-(Y/(2+0))") == "X/2-Y/2, U");
    // fail
    REQUIRE(simplify_term("1+a") == "<unchanged>, F");
}

TEST_CASE("simplify_symbolic") {
    REQUIRE(simplify_term("-f(-|1-2|)") == "-f(-1), U");
    REQUIRE(simplify_term("-f(1+2+X,-X)") == "-f(1*X+3,-X), U");
    REQUIRE(simplify_term("(1+2+X,-X)") == "(1*X+3,-X), U");
    REQUIRE(simplify_term("f(1+a)") == "<unchanged>, F");
    REQUIRE(simplify_term("f(X+a)") == "<unchanged>, F");
}

TEST_CASE("simplify_aux") {
    REQUIRE(simplify_term("1..2") == "__A_0, __A_0=1..2, U");
    REQUIRE(simplify_term("f(1..2)") == "f(__A_0), __A_0=1..2, U");
    REQUIRE(simplify_term("@f") == "__A_0, __A_0=@f, U");
    REQUIRE(simplify_term("@f(@g(1+2))") == "__A_1, __A_0=@g(3), __A_1=@f(__A_0), U");
}

TEST_CASE("simplify_matchable") {
    auto flags = SimplifyTermFlags::matchable | SimplifyTermFlags::unfailable;
    REQUIRE(simplify_term("f(1,X+Y)", flags) == "f(1,__A_0), __A_0=X+Y, U");
    REQUIRE(simplify_term("f(1,2*X)", flags) == "f(1,__A_0), __A_0=2*X+0, U");
    REQUIRE(simplify_term("p(X+5,@f(g(X*X)))", flags) == "p(__A_1,__A_0), __A_0=@f(g(X*X)), __A_1=1*X+5, U");

    flags &= ~SimplifyTermFlags::unfailable;
    REQUIRE(simplify_term("f(1,X+Y)", flags) == "f(1,1*__A_0+0), __A_0=X+Y, U");
    REQUIRE(simplify_term("f(1,2*X)", flags) == "f(1,2*X+0), U");
    REQUIRE(simplify_term("p(X+5,@f(g(X*X)))", flags) == "p(1*X+5,__A_0), __A_0=@f(g(X*X)), U");
}

TEST_CASE("simplify_literal") {
    auto flags = SimplifyLiteralFlags::matchable;
    REQUIRE(simplify_literal("#sup<=1", flags) == "#false, B");
    REQUIRE(simplify_literal("1>=2", flags) == "#false, B");
    REQUIRE(simplify_literal("1<2", flags) == "#true, T");
    REQUIRE(simplify_literal("2<1", flags) == "#false, B");
    REQUIRE(simplify_literal("X=Y+Z", flags) == "<unchanged>, U");
    REQUIRE(simplify_literal("X=Y+Z=Z", flags) == "<unchanged>, U");
    REQUIRE(simplify_literal("not not X=Y+Z=Z", flags) == "X=Y+Z=Z, U");
    REQUIRE(simplify_literal("not X=Y+Z=Z", flags) == "not X=__A_0=Z, __A_0=Y+Z, U");
    REQUIRE(simplify_literal("X=f(Y+Z,Z+5)<f(Y+Z,Z+5)", flags) == "X=f(1*__A_0+0,1*Z+5)<f(Y+Z,1*Z+5), __A_0=Y+Z, U");

    REQUIRE(simplify_literal("X=2<1", flags) == "#false, B");
    REQUIRE(simplify_literal("X=2>1", flags) == "<unchanged>, U");
    REQUIRE(simplify_literal("not X!=2<1", flags) == "#true, T");
    REQUIRE(simplify_literal("not X!=2>1", flags) == "<unchanged>, U");
    REQUIRE(simplify_literal("1<2<3<4", flags) == "#true, T");
    REQUIRE(simplify_literal("not 1<2<3<4", flags) == "#false, B");

    REQUIRE(simplify_literal("p(X,*)", flags) == "<unchanged>, U");
    REQUIRE(simplify_literal("p(X+Y,Y+1)", flags) == "p(1*__A_0+0,1*Y+1), __A_0=X+Y, U");
    REQUIRE(simplify_literal("not p(X+Y,Y+1)", flags) == "not p(X+Y,1*Y+1), U");
    REQUIRE(simplify_literal("not not p(X+Y,Y+1)", flags) == "not not p(X+Y,1*Y+1), U");
    REQUIRE(simplify_literal("p(X,1*Y+1)", flags) == "<unchanged>, U");
    REQUIRE(simplify_literal("not p(X+Y,1*Y+1)", flags) == "<unchanged>, U");
    REQUIRE(simplify_literal("not not p(X+Y,1*Y+1)", flags) == "<unchanged>, U");
}

TEST_CASE("simplify_head_cond_lit") {
    REQUIRE(simplify_statement(":-.") == "<unchanged>, B");
    REQUIRE(simplify_statement("X=Y+Z=Z: cond.") == "<unchanged>, U");
    REQUIRE(simplify_statement("X=Y+Z=Z: cond(Y).") == "<unchanged>, U");
    REQUIRE(simplify_statement("not not X=Y+Z=Z: cond.") == "X=Y+Z=Z: cond., U");
    REQUIRE(simplify_statement("not X=Y+Z=Z: cond.") == "not X=__A_0=Z: cond, __A_0=Y+Z., U");
}

TEST_CASE("simplify_body_cond_lit") {
    REQUIRE(simplify_statement("x :- X=Y+Z=Z: cond.") == "x :- X=__A_0=Z: cond, __A_0=Y+Z., U");
    REQUIRE(simplify_statement("x :- not not X=Y+Z=Z: cond.") == "x :- X=__A_0=Z: cond, __A_0=Y+Z., U");
    REQUIRE(simplify_statement("x :- not X=Y+Z=Z: cond.") == "<unchanged>, U");
    REQUIRE(simplify_statement("x(X) :- not X=Y+Z=Z: cond(X).") == "<unchanged>, U");
}

TEST_CASE("simplify_head_set_aggregate") {
    REQUIRE(simplify_statement("{ not 2<1<X: p(X) }.") == "#count { 3,X: #true: p(X) }., U");
    REQUIRE(simplify_statement("X+a <= {a} :- x.") == "#true., T");
    REQUIRE(simplify_statement("X+Y <= {a}.") == "X+Y <= #count { 0,a: a }., U");
    REQUIRE(simplify_statement("@f(X) <= {a}.") == "__A_0 <= #count { 0,a: a } :- __A_0=@f(X)., U");
    REQUIRE(simplify_statement("{a; not a; not not a; X+Y<Z: p(U)} <= 1.") ==
            "#count { 0,a: a; 1,a: not a; 2,a: not not a; 6,X,Y,Z: #true: p(U), X+Y<Z } <= 1., U");
    REQUIRE(simplify_statement("1..2 <= {a(3..4,X+Y): b(A+B)} <= 5..6.") ==
            "__A_2 <= #count { "
            "0,a(__A_0,__A_1): a(__A_0,__A_1): "
            "b(1*__A_4+0), __A_0=3..4, __A_1=X+Y, __A_4=A+B "
            "} <= __A_3 :- __A_2=1..2; __A_3=5..6., U");

    REQUIRE(simplify_statement("X <= {1!=2;2!=2;#true;#false} <= Y.") == "X<=2<=Y., U");
    REQUIRE(simplify_statement("1 <= {1!=2;2!=2;#true;#false} <= 2.") == "#true., T");
    REQUIRE(simplify_statement("1 <= {1!=2;2!=2;#true;#false} <= 1.") == " :- ., B");
}

TEST_CASE("simplify_body_set_aggregate") {
    REQUIRE(simplify_statement("x :- X+a <= {a}.") == "#true., T");
    REQUIRE(simplify_statement("x :- X+Y <= {a}.") == "x :- X+Y <= #count { 0,a: a }., U");
    REQUIRE(simplify_statement("x :- @f(X) <= {a}.") == "x :- __A_0 <= #count { 0,a: a }; __A_0=@f(X)., U");
    REQUIRE(simplify_statement("x :- {a; not a; not not a; X+Y<Z: p(U)} <= 1.") ==
            "x :- #count { 0,a: a; 1,a: not a; 2,a: not not a; 6,X,Y,Z: p(U), X+Y<Z } <= 1., U");
    REQUIRE(simplify_statement("x :- 1..2 <= {a(3..4,X+Y): b(A+B)} <= 5..6.") ==
            "x :- __A_2 <= #count { "
            "0,a(1*__A_0+0,1*__A_1+0): "
            "b(1*__A_4+0), __A_0=3..4, __A_1=X+Y, a(1*__A_0+0,1*__A_1+0), __A_4=A+B "
            "} <= __A_3; __A_2=1..2; __A_3=5..6., U");
    REQUIRE(simplify_statement("x :- X <= {1!=2;2!=2;#true;#false} <= Y.") == "x :- X<=2<=Y., U");
    REQUIRE(simplify_statement("x :- 1 <= {1!=2;2!=2;#true;#false} <= 2.") == "x., U");
    REQUIRE(simplify_statement("x :- 1 <= {1!=2;2!=2;#true;#false} <= 1.") == "#true., T");
}

TEST_CASE("simplify_head_aggregate") {
    REQUIRE(simplify_statement("#count { X: #true } >= 1 :- p(X).") == "<unchanged>, U");
    REQUIRE(simplify_statement("#count { 1: #true } >= 1 :- p(X).") == "#true., T");
    REQUIRE(simplify_statement("#count { : #true } >= 1 :- p(X).") == "#true., T");
    REQUIRE(simplify_statement("#count { 1: #true } >= 2 :- p(X).") == " :- p(X)., U");
    REQUIRE(simplify_statement("#min { : #true } <= 1 :- p(X).") == " :- p(X)., U");
    REQUIRE(simplify_statement("#max { : #true } >= 1 :- p(X).") == " :- p(X)., U");
    REQUIRE(simplify_statement("#min { : #true } >= 1 :- p(X).") == "#true., T");
    REQUIRE(simplify_statement("#max { : #true } <= 1 :- p(X).") == "#true., T");
    REQUIRE(simplify_statement("#min { 1 : #true } >= 1 :- p(X).") == "#true., T");
    REQUIRE(simplify_statement("#max { 1 : #true } >= 1 :- p(X).") == "#true., T");
    REQUIRE(simplify_statement("#sum { 1 : #true; -1: #true; -1: #true } >= 1 :- p(X).") == " :- p(X)., U");
    REQUIRE(simplify_statement("#sum { 1 : #true; -1: #true; -1: #true } >= 0 :- p(X).") == "#true., T");
    REQUIRE(simplify_statement("#sum+ { 1 : #true; -1: #true; -1: #true } >= 2 :- p(X).") == " :- p(X)., U");
    REQUIRE(simplify_statement("#sum+ { 1 : #true; -1: #true; -1: #true } >= 1 :- p(X).") == "#true., T");
    REQUIRE(simplify_statement("#sum+ { f(X) : #true; X: #true } >= 1 :- p(X).") ==
            "#sum+ { X: #true } >= 1 :- p(X)., U");
    REQUIRE(simplify_statement("#sum { 1 : X < Y+1 < Z } >= 1 :- p(X).") ==
            "#sum { 1: #true: X<1*Y+1<Z } >= 1 :- p(X)., U");
    REQUIRE(simplify_statement("#sum { 1 : not X < Y+1 < Z } >= 1 :- p(X).") ==
            "#sum { 1: #true: __A_0=1*Y+1, not X<__A_0<Z } >= 1 :- p(X)., U");
}

TEST_CASE("simplify_body_aggregate") {
    REQUIRE(simplify_statement("p(X) :- X+Y = #count { X } >= X+Y.") ==
            "p(X) :- 1*__A_0+0 = #count { X } >= X+Y; __A_0=X+Y., U");
    REQUIRE(simplify_statement("p(X) :- #count { X } >= 1.") == "<unchanged>, U");
    REQUIRE(simplify_statement("p(X) :- #count { 1 } >= 1.") == "p(X)., U");
    REQUIRE(simplify_statement("p(X) :- #count { : #true } >= 1.") == "p(X)., U");
    REQUIRE(simplify_statement("p(X) :- #count { 1 } >= 2.") == "#true., T");
    REQUIRE(simplify_statement("p(X) :- #min { : #true } <= 1.") == "#true., T");
    REQUIRE(simplify_statement("p(X) :- #max { : #true } >= 1.") == "#true., T");
    REQUIRE(simplify_statement("p(X) :- #min { : #true } >= 1.") == "p(X)., U");
    REQUIRE(simplify_statement("p(X) :- #max { : #true } <= 1.") == "p(X)., U");
    REQUIRE(simplify_statement("p(X) :- #min { 1 : #true } >= 1.") == "p(X)., U");
    REQUIRE(simplify_statement("p(X) :- #max { 1 : #true } >= 1.") == "p(X)., U");
    REQUIRE(simplify_statement("p(X) :- #sum { 1 : #true; -1: #true; -1: #true } >= 1.") == "#true., T");
    REQUIRE(simplify_statement("p(X) :- #sum { 1 : #true; -1: #true; -1: #true } >= 0.") == "p(X)., U");
    REQUIRE(simplify_statement("p(X) :- #sum+ { 1 : #true; -1: #true; -1: #true } >= 2.") == "#true., T");
    REQUIRE(simplify_statement("p(X) :- #sum+ { 1 : #true; -1: #true; -1: #true } >= 1.") == "p(X)., U");
    REQUIRE(simplify_statement("p(X) :- #sum+ { f(X) : #true; X: #true } >= 1.") == "p(X) :- #sum+ { X } >= 1., U");
    REQUIRE(simplify_statement("p(X) :- #sum { 1 : X < Y+1 < Z } >= 1.") == "p(X) :- #sum { 1: X<1*Y+1<Z } >= 1., U");
    REQUIRE(simplify_statement("p(X) :- #sum { 1 : not X < Y+1 < Z } >= 1.") ==
            "p(X) :- #sum { 1: not X<__A_0<Z, __A_0=1*Y+1 } >= 1., U");
}

TEST_CASE("simplify_head_theory") {
    REQUIRE(simplify_statement("&t(X+1..Y) { 1..X: X+1..Y>Z } >= f(1..X).") ==
            "&t(__A_0) { (1 .. X): __A_1>Z, __A_1=X+1..Y } >= f((1 .. X)) :- __A_0=X+1..Y., U");
    REQUIRE(simplify_statement("&t { 1: #true; 2: #false }.") == "&t { 1 }., U");
}

TEST_CASE("simplify_body_theory") {
    REQUIRE(simplify_statement("p(X) :- &t(X+1..Y) { 1..X: X+1..Y>Z } >= f(1..X).") ==
            "p(X) :- &t(__A_0) { (1 .. X): __A_1>Z, __A_1=X+1..Y } >= f((1 .. X)); __A_0=X+1..Y., U");
    REQUIRE(simplify_statement("p(X) :- &t { 1: #true; 2: #false }.") == "p(X) :- &t { 1 }., U");
}

TEST_CASE("simplify_rule") {
    REQUIRE(simplify_statement("p(X+1) :- q(2*X,Y+Z).") == "p(1*X+1) :- q(2*X+0,1*__A_0+0); __A_0=Y+Z., U");
    REQUIRE(simplify_statement("p(X+1) | p(X+Y).") == "p(1*X+1); p(X+Y)., U");
    REQUIRE(simplify_statement("p(X) | p(X+Y).") == "<unchanged>, U");
    REQUIRE(simplify_statement("p(X+1) | p(X+Y): q(X+Y).") ==
            "p(1*X+1); p(1*__A_0+0): q(1*__A_1+0), __A_0=X+Y, __A_1=X+Y., U");
    REQUIRE(simplify_statement("#false | p(X+Y): q(X+Y).") == "p(1*__A_0+0): q(1*__A_1+0), __A_0=X+Y, __A_1=X+Y., U");
    REQUIRE(simplify_statement("#false | #true: q(X+Y).") == "#true: q(1*__A_0+0), __A_0=X+Y., U");
    REQUIRE(simplify_statement("#false | #false: q(X+Y).") == " :- ., B");
    REQUIRE(simplify_statement("#true | p(X+Y): q(X+Y).") == "#true., T");
    REQUIRE(simplify_statement("x :- p(X+Y): q(X+Y).") == "x :- p(X+Y): q(1*__A_0+0), __A_0=X+Y., U");
    REQUIRE(simplify_statement("x :- not p(X+Y): q(X+Y).") == "x :- not p(X+Y): q(1*__A_0+0), __A_0=X+Y., U");
    REQUIRE(simplify_statement("x :- #false: q(X+Y).") == "x :- #false: q(1*__A_0+0), __A_0=X+Y., U");
    REQUIRE(simplify_statement("x :- #true: q(X+Y).") == "x., U");
    REQUIRE(simplify_statement("x :- p(X+a): q(X+Y).") == "x., U");
    REQUIRE(simplify_statement("x :- p(X+Y): q(X+a).") == "x., U");
    REQUIRE(simplify_statement("#false.") == "<unchanged>, B");
    REQUIRE(simplify_statement("#false :- #true.") == " :- ., B");
    REQUIRE(simplify_statement("p(X) :- q(*).") == "<unchanged>, U");
    REQUIRE(simplify_statement("h(Y) :- X=1..2, Y=@x(X).") == "<unchanged>, U");
}

TEST_CASE("simplify_show") {
    REQUIRE(simplify_statement("#show p(X) : #false.") == "#true., T");
    REQUIRE(simplify_statement("#show p(X+a).") == "#true., T");
    REQUIRE(simplify_statement("#show p(1..2) : p(3..4).") ==
            "#show p(__A_0): p(1*__A_1+0); __A_0=1..2; __A_1=3..4., U");
}

TEST_CASE("simplify_project") {
    REQUIRE(simplify_statement("#project p(X) : #false.") == "#true., T");
    REQUIRE(simplify_statement("#project p(X+a).") == "#true., T");
    REQUIRE(simplify_statement("#project p(1..2) : p(3..4).") ==
            "#project p(1*__A_0+0): p(1*__A_1+0); __A_0=1..2; __A_1=3..4., U");
}

TEST_CASE("simplify_external") {
    REQUIRE(simplify_statement("#external p(X) : #false.") == "#true., T");
    REQUIRE(simplify_statement("#external p(X+a).") == "#true., T");
    REQUIRE(simplify_statement("#external p(1..2) : p(3..4). [@x]") ==
            "#external p(1*__A_0+0): p(1*__A_2+0); __A_0=1..2; __A_1=@x; __A_2=3..4. [__A_1], U");
}

TEST_CASE("simplify_edge") {
    REQUIRE(simplify_statement("#edge (X,Y) : #false.") == "#true., T");
    REQUIRE(simplify_statement("#edge (X+a,Y).") == "#true., T");
    REQUIRE(simplify_statement("#edge (1..2,3..4) : p(5..6).") ==
            "#edge (__A_0,__A_1): p(1*__A_2+0); __A_0=1..2; __A_1=3..4; __A_2=5..6., U");
}

TEST_CASE("simplify_heuristic") {
    REQUIRE(simplify_statement("#heuristic p(X+a). [X@Y,Z]") == "#true., T");
    REQUIRE(simplify_statement("#heuristic p(X). [X+a@Y,Z]") == "#true., T");
    REQUIRE(simplify_statement("#heuristic p(X). [X@Y+a,Z]") == "#true., T");
    REQUIRE(simplify_statement("#heuristic p(X). [X@Y,Z+a]") == "#true., T");
    REQUIRE(simplify_statement("#heuristic p(1..2) : p(3..4). [5..6@7..8,0..9]") ==
            "#heuristic p(1*__A_0+0)"
            ": p(1*__A_4+0); __A_0=1..2; __A_1=0..9; __A_2=5..6; __A_3=7..8; __A_4=3..4"
            ". [__A_2@__A_3,__A_1], U");
}

TEST_CASE("simplify_weak") {
    REQUIRE(simplify_statement(":~ p(X+a). [X@Y,Z]") == "#true., T");
    REQUIRE(simplify_statement(":~ p(X). [X+a@Y,Z]") == "#true., T");
    REQUIRE(simplify_statement(":~ p(X). [X@Y+a,Z]") == "#true., T");
    REQUIRE(simplify_statement(":~ p(X). [X@Y,Z+a]") == "#true., T");
    REQUIRE(simplify_statement(":~ p(1..2). [3..4@5..6,7..8]") ==
            " :~ p(1*__A_3+0); __A_0=3..4; __A_1=5..6; __A_2=7..8; __A_3=1..2"
            ". [__A_0@__A_1,__A_2], U");
}

TEST_CASE("simplify_fstring") {
    REQUIRE(simplify_statement(":- p(X,f\"{X}\").") == " :- p(X,__A_0); __A_0=f\"{X}\"., U");
    REQUIRE(simplify_statement(":- p(X,Y); Y=f\"{X}\".") == "<unchanged>, U");
    REQUIRE(simplify_term("f\"abc\"") == "\"abc\", U");
    REQUIRE(simplify_term(R"(f"{1+3}")") == R"(f"{4}", U)");
}

} // namespace CppClingo::Input::Test
