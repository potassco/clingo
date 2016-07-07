#script (python)

import clingo

def solve(s):
    prg = clingo.Control(["0", "-Wno-operation-undefined"])
    with prg.builder() as b:
        clingo.parse_program(s, lambda stm: b.add(stm))
    prg.ground([("base", [])])
    ret = []
    prg.solve(on_model=lambda m: ret.append(sorted(m.atoms(shown=True))))
    return str(sorted(ret))

def parse(s):
    prg = clingo.Control([])
    with prg.builder() as b:
        clingo.parse_program(s, lambda stm: b.add(stm))
    prg.ground([("base", [])])
    return str([x for x in prg.theory_atoms])

def check(l, t = "", s=solve):
    ret = []
    for n, (p, r) in zip(range(len(l)), l):
        ret.append((n, clingo.Function("ok" if s(p+t) == r else "fail")))
    return ret

def stm():
    return check([
        ("a.", "[[a]]"),
        ("a. c. b :- a, c.", "[[a, b, c]]"),
        ("#const a=10. p(a).", "[[p(10)]]"),
        ("a. b. #show a/0.", "[[a]]"),
        ("$a$=1. $b$=2. #show $a/0.", "[[$(a,1)]]"),
        ("a. #show b : a.", "[[a, b]]"),
        ("$a$=1. $b$=2. #show. #show $a.", "[[$(a,1)]]"),
        ("#minimize{ 1:b; 2:a }. {a;b}. :- not a, not b.", "[[b]]"),
        # NOTE: atm the script code is not evaluated...
        ("#script (python) def x() return 32; end #"+ "end. p(@x()).", "[[]]"),
        ("#edge (u,v) : a. #edge (v,u) : b. {a;b}.", "[[], [a], [b]]"),
        ("#theory x {}.", "[[]]"),
        ("#external a.", "[[]]"),
        ("#heuristic a : b, c. [1@2,level] b. c.", "[[b, c]]"),
        ("#project a.", "[[]]"),
        ("#project a/0.", "[[]]"),
    ])

def bdl():
    return check([
        ("{a}. :-a.", "[[]]"),
        ("{a}. :-not a.", "[[a]]"),
        ("{a}. :-not not a.", "[[]]"),
        (":-a:b. 0{a; b}0.", "[]"),
        (":-0{a:b;c}1. {a;b;c}.", "[[a, b, c]]"),
        (":-0#min{1,2:a,b;2:c}2. {a;b;c}.", "[[], [a], [b]]"),
        ("1 $<= $x $<= 2. 1 $<= $y $<= 2. :- #disjoint {1:$x; 2:$y}.", "[[$(x,1), $(y,1)], [$(x,2), $(y,2)]]"),
    ])

def hdl():
    return check([
        ("a.", "[[a]]"),
        ("not a. a :- #false.", "[[]]"),
        ("not not a. a :- #false.", "[]"),
        ("a:b;c.{b}.", "[[a, b], [b, c], [c]]"),
        ("1{a:b;b}2.", "[[a, b], [b]]"),
        ("#min{1,2:a;2:c}1.", "[[a], [a, c]]"),
    ])

def lit():
    return check([
        ("a.", "[[a]]"),
        ("1=1.", "[[]]"),
        ("1!=1.", "[]"),
        ("#true.", "[[]]"),
        ("#false.", "[]"),
        ("1 $< 2 $< 3.", "[[]]"),
        ("2 $< 3 $< 1.", "[]"),
    ])

def trm():
    return check([
        ("p(a).", "[[p(a)]]"),
        ("p(X) :- X=a.", "[[p(a)]]"),
        ("p(-1).", "[[p(-1)]]"),
        ("p(|1|).", "[[p(1)]]"),
        ("p((3+2)).", "[[p(5)]]"),
        ("p((3-2)).", "[[p(1)]]"),
        ("p((3*2)).", "[[p(6)]]"),
        ("p((7/2)).", "[[p(3)]]"),
        ("p((7\\2)).", "[[p(1)]]"),
        ("p((7?2)).", "[[p(7)]]"),
        ("p((7^2)).", "[[p(5)]]"),
        ("p(3..3).", "[[p(3)]]"),
        ("p(a;b).", "[[p(a), p(b)]]"),
        ("p((),(1,),f(),f(1,2)).", "[[p((),(1,),f,f(1,2))]]"),
        ("p((a,;b)).", "[[p(b), p((a,))]]"),
        ("12 $< 1 $+ 3 $* $x $+ 7 $< 17. 0 $<= x $<= 4.", "[[$(x,2)]]"),
    ])

def tht():
    theory = '''
        #theory x {
            t {
                * : 1, binary, left;
                ^ : 2, binary, right;
                - : 3, unary
            };
            &a/0 : t, directive;
            &b/0 : t, {=}, t, any
        }.
        '''
    return check([
        ("&a{}.", "[&a{}]"),
        ("&a{1,2,3:a,b}. {a;b}.", "[&a{1,2,3: a,b}]"),
        ("&b{} = a.", "[&b{}=a]"),
        ("&b{} = X:-X=1.", "[&b{}=1]"),
        ("&b{} = [].", "[&b{}=[]]"),
        ("&b{} = [1].", "[&b{}=[1]]"),
        ("&b{} = [1,2].", "[&b{}=[1,2]]"),
        ("&b{} = ().", "[&b{}=()]"),
        ("&b{} = (a).", "[&b{}=a]"),
        ("&b{} = (a,).", "[&b{}=(a,)]"),
        ("&b{} = {}.", "[&b{}={}]"),
        ("&b{} = f().", "[&b{}=f()]"),
        ("&b{} = f(a,1).", "[&b{}=f(a,1)]"),
        ("&b{} = a*x.", "[&b{}=(a*x)]"),
        ("&b{} = -a*x*y^z^u.", "[&b{}=(((-a)*x)*(y^(z^u)))]"),
        ("&b{} = -(a*x*y^z^u).", "[&b{}=(-((a*x)*(y^(z^u))))]"),
    ], theory, parse)

def main(prg):
    prg.ground([("base", [])])
    prg.solve()

#end.

stm(N,R) :- (N,R) = @stm().
bdl(N,R) :- (N,R) = @bdl().
hdl(N,R) :- (N,R) = @hdl().
lit(N,R) :- (N,R) = @lit().
trm(N,R) :- (N,R) = @trm().
tht(N,R) :- (N,R) = @tht().

