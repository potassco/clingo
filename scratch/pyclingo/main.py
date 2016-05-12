import clingo
import sys

def inject_one(val):
    return clingo.create_num(val.num() + val.num())

def inject_two(val):
    return [val, clingo.create_num(val.num() + val.num())]

def on_model(m):
    print m

c = clingo.Control(sys.argv)
c.add("base", [], "#external c. a :- c. a :- not b. b :- not a. r(@inject_one(2)). r(@inject_two(3)). #program p(k). q(k).")
c.ground([("base", []), ("p", [clingo.create_num(17)])], sys.modules[__name__])
print c.solve(on_model=on_model)

vals = [clingo.create_id("p"), clingo.create_str("q"), clingo.create_num(42), clingo.Sup, clingo.Inf]
vals.append(clingo.create_fun("f", vals, True))
vals.append(clingo.create_fun("", vals))
print vals
print vals[2].num()
print vals[0].name()
print vals[1].str()
print vals[3].type() == clingo.Value.SUP
print vals[5].args()
print vals[0] == vals[0], vals[0] != vals[0], vals[0] < vals[0], vals[0] <= vals[0], vals[0] > vals[0], vals[0] >= vals[0]
print vals[5] == vals[6], vals[5] < vals[6], vals[5] > vals[6]

c.assign_external(clingo.create_id("c"), True)
with c.solve_iter() as it:
    for m in it:
        print m
    print it.get(), it.get() == clingo.SolveResult.SAT

program = c.parse("""
#program base(k,t).
p(|1;2|).
a :- b.
a(1,2) :- b(-3,4;5+6).
#true :- #false, 1 < 2. a.
a :- b:c,d.
a;b;c:d. p((1,2)).
1 <= #count { a : b : c, d; e : #true  } <= 2.
1 { a; b : c } != 2.
:- 1 <= #count { a : b, c; d } <= 2.
:- 1 <= { a : b, c; d } <= 2.
#const xxx = 17.
#minimize {1@2 : a; 3@4,5 : b, c }.
#show a/2.
#show $a/2.
#show a : b.
#show $a : b.
#edge (a, b) : c.
#edge (a, b; c, d) : e.
#external a : b.
#heuristic a : b, c. [x@y,z]
#heuristic a(1;2) : b, c. [x@y,z]
#project a : b, c.
#project a/3.
:- 1 $* $a $+ $b $+ 2 $< $c $- 1 $< 3*3.
#disjoint { a, b : $c $* 1 $+ 2 : c; 1 : $d : e }.
p(X) :- q(X).
#theory x {
  a {
    + : 1, unary;
    * : 2, binary, left;
    ^ : 3, binary, right
  };
  &b/0 : a, any;
  &c/0 : a, {+,-}, a, directive
}.
&b { f(a) : q } <= a * -b * c.
""")
for node in program:
    print node

c.add_ast(program)
