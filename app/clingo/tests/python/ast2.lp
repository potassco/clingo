#script (python)

import clingo

def parse(s):
    ret = []
    clingo.parse_program(s, lambda stm: ret.append(str(stm)))
    return ret[1:]

def check(l):
    ret = []
    for n, (p, r) in zip(range(len(l)), l):
        ret.append((n, clingo.Function("ok" if parse(p) == r else "fail")))
    return ret

def stm():
    return check([
        ("a.", ["a."]),
        ("a:-b.", ["a :- b."]),
        ("#const a=10.", ["#const a = 10. [default]"]),
        ("#show a/1.", ["#show a/1."]),
        ("#show $a/1.", ["#show $a/1."]),
        ("#show a : b.", ["#show a : b."]),
        ("#show $a : b.", ["#show $a : b."]),
        ("#minimize{ 1:b }.", [":~ b. [1@0]"]),
        ("#script (python) 42 #" + "end.", ["#script (python) 42 #" + "end."]),
        ("#program p(k).", ["#program p(k)."]),
        ("#external p(k).", ["#external p(k)."]),
        ("#external p(k) : a, b.", ["#external p(k) : a; b."]),
        ("#edge (u,v) : a, b.", ["#edge (u,v) : a; b."]),
        ("#heuristic a : b, c. [L@P,level]", ["#heuristic a : b; c. [L@P,level]"]),
        ("#project a : b.", ["#project a : b."]),
        ("#project a/2.", ["#project a/2."]),
        ("#theory x {}.", ["#theory x {\n}."]),
    ])

def thd():
    return check([
        ("#theory x { t { ++ : 1, unary } }.", ["#theory x {\n  t {\n    ++ : 1, unary\n  }\n}."]),
        ("#theory x { &a/0 : t, any }.", ["#theory x {\n  &a/0 : t, any\n}."]),
        ("#theory x { &a/0 : t, {+, -}, u, any }.", ["#theory x {\n  &a/0 : t, { +, - }, u, any\n}."]),
    ])

def bdl():
    return check([
        (":-a.", ["#false :- a."]),
        (":-not a.", ["#false :- not a."]),
        (":-not not a.", ["#false :- not not a."]),
        (":-a:b.", ["#false :- a : b."]),
        (":-a:b,c;d.", ["#false :- a : b, c; d."]),
        (":-1{a:b,c;e}2.", ["#false :- 1 <= { a : b, c; e :  } <= 2."]),
        (":-{a:b,c;e}2.", ["#false :- 2 >= { a : b, c; e :  }."]),
        (":-1#min{1,2:b,c;1:e}2.", ["#false :- 1 <= #min { 1,2 : b, c; 1 : e } <= 2."]),
        (":-&p { 1 : a,b; 2 : c }.", ["#false :- &p { 1 : a,b; 2 : c }."]),
        (":-#disjoint {1,2:$x:a,b}.", ["#false :- #disjoint { 1,2 : 1$*$x : a,b }."]),
    ])

def hdl():
    return check([
        ("a.", ["a."]),
        ("a:b.", ["a : b."]),
        ("a:b,c;d.", ["d : ; a : b, c."]),
        ("1{a:b,c;e}2.", ["1 <= { a : b, c; e :  } <= 2."]),
        ("{a:b,c;e}2.", ["2 >= { a : b, c; e :  }."]),
        ("1#min{1,2:h:b,c;1:e}2.", ["1 <= #min { 1,2 : h : b, c; 1 : e :  } <= 2."]),
        ("&p { 1 : a,b; 2 : c }.", ["&p { 1 : a,b; 2 : c }."]),
        ("&p { 1 : a,b; 2 : c } ** 33.", ["&p { 1 : a,b; 2 : c } ** 33."]),
    ])

def lit():
    return check([
        ("#true.", ["#true."]),
        ("#false.", ["#false."]),
        ("a.", ["a."]),
        ("not a.", ["not a."]),
        ("not not a.", ["not not a."]),
        ("1 != 3.", ["1!=3."]),
        ("1 $< 2 $< 3.", ["1$<2$<3."]),
        ("1 $< 2 $< 3.", ["1$<2$<3."]),
    ])

def trm():
    return check([
        ("p(a).", ["p(a)."]),
        ("p(X).", ["p(X)."]),
        ("p(-a).", ["p(-a)."]),
        ("p(~a).", ["p(~a)."]),
        ("p(|a|).", ["p(|a|)."]),
        ("p((a+b)).", ["p((a+b))."]),
        ("p((a-b)).", ["p((a-b))."]),
        ("p((a*b)).", ["p((a*b))."]),
        ("p((a/b)).", ["p((a/b))."]),
        ("p((a\\b)).", ["p((a\\b))."]),
        ("p((a?b)).", ["p((a?b))."]),
        ("p((a^b)).", ["p((a^b))."]),
        ("p(a..b).", ["p((a..b))."]),
        ("p((),(1,),f(),f(1,2)).", ["p((),(1,),f,f(1,2))."]),
        ("p(a;b).", ["(p(a);p(b))."]),
        ("p((a,;b)).", ["p(((a,);b))."]),
        ("1 $+ 3 $* $x $+ 7 $< 2 $< 3.", ["1$+3$*$x$+7$<2$<3."]),
    ])

def tht():
    return check([
        ("&p{ } !! a.", ["&p {  } !! a."]),
        ("&p{ } !! X.", ["&p {  } !! X."]),
        ("&p{ } !! [].", ["&p {  } !! []."]),
        ("&p{ } !! [1].", ["&p {  } !! [1]."]),
        ("&p{ } !! [1,2].", ["&p {  } !! [1,2]."]),
        ("&p{ } !! ().", ["&p {  } !! ()."]),
        ("&p{ } !! (a).", ["&p {  } !! a."]),
        ("&p{ } !! (a,).", ["&p {  } !! (a,)."]),
        ("&p{ } !! {}.", ["&p {  } !! {}."]),
        ("&p{ } !! f().", ["&p {  } !! f."]),
        ("&p{ } !! f(a,1).", ["&p {  } !! f(a,1)."]),
        ("&p{ } !! 1 + (x + y * z).", ["&p {  } !! (1 + (x + y * z))."]),
    ])

def main(prg):
    prg.ground([("base", [])])
    prg.solve_refactored()

#end.

stm(N,R) :- (N,R) = @stm().
thd(N,R) :- (N,R) = @thd().
bdl(N,R) :- (N,R) = @bdl().
hdl(N,R) :- (N,R) = @hdl().
lit(N,R) :- (N,R) = @lit().
trm(N,R) :- (N,R) = @trm().
tht(N,R) :- (N,R) = @tht().

