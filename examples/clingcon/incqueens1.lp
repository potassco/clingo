% the idea of this encoding is
% 1. restrict the domain of all queens variables to 1..max globally
% 2. add csp variables for each step
% 3. add a fresh distinct for all variables every step, do not reuse old constraints
% 3.5. this works, as old constraints are not "invalid", e.g. distinct(q(1),q(2)) is still valid if we add distinct(q(1),q(2),q(3)),
% it is just unecessary

#script(python)
import clingo
from clingo import *

def main(prg):
  for i in range(1, 30):
     # old assumptions are free to vary
     prg.assign_external(Function("active",[i-1]),None)
     prg.ground([("step",[i])])
     # new assumption is true for this step
     prg.assign_external(Function("active",[i]),True)
     prg.solve()
#end.

#include "csp.lp".
#const max=300.
#program step(n).
#external active(n).

pos(n).

% add a maximum value for this variable,
%this restricts the encoding to increment only up to this value
&dom {1..max} = q(n).

% all further rules are only true in the current active step
&dom {1..n} = q(X) :- pos(X), active(n).

&distinct {q(X) : pos(X)} :- active(n).
%diagonals
&distinct {q(X)+X-1 : pos(X)} :- active(n).
&distinct {q(X)-X+1 : pos(X)} :- active(n).

% add the last queen to the show statement
&show {q(n)}.
