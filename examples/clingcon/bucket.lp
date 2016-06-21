% call with --min-lits-per-var=0

#script(python)
import clingo
from clingo import *

def main(prg):
  prg.ground([("base",[])])
  for i in range(1, 30):
     prg.ground([("step",[i])])
     prg.solve()
#end.

#include "csp.lp".
sides(l;r).
&show{add/2}.
&show{bucket/2}.

%left and right start with nothing
&sum{bucket(X,0)} = 0 :- sides(X).

#program step(n).

%add(l;r) at time n is a positive number
&sum{add(X,n)} >= 0 :- sides(X).

%add to both sides
&sum{bucket(X,P); add(X,n)} = bucket(X,n) :- sides(X), P=n-1.

up(r,n)  :- &sum{bucket(l,n)} < bucket(r,n).
low(r,n) :- &sum{bucket(l,n)} > bucket(r,n).
:- up(r,n), low(r,n). % this constraints helps the CSP solver
eq(n) :- not up(r,n), not low(r,n).

