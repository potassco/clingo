% the idea of this encoding is
% 1. all variables are restricted to -2^30..2^30
% 2. add csp variables for each step
% 3. add all x!=y constraints for the new queen
%%% Difference to incqueens3
% only domain of the variables i retracted(removed)
% i only add constraints, never remove them
% still quadratic

#script(python)
import clingo
from clingo import *

def main(prg):
  for i in range(1, 300):
     prg.release_external(Function("active",[i-1]))
     prg.ground([("step",[i])])
     # new assumption is true for this step
     prg.assign_external(Function("active",[i]),True)
     prg.solve()
#end.

#include "csp.lp".
#program step(n).
#external active(n).

pos(n).

% all further rules are only true in the current active step
:- not &dom(nonstrict) {1..n} = q(X), pos(X), active(n).
:- &dom(nonstrict) {1..n} = q(X), pos(X), not active(n).

&sum{q(X)} != q(n) :- X=1..n-1.

&sum{q(X)+X-1} != q(n)+n-1 :- X=1..n-1.
&sum{q(X)-X+1} != q(n)-n+1 :- X=1..n-1.

&show {q(n)}.
