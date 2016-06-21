% use --distinct-to-card=false
% the idea of this encoding is
% 1. all variables are restricted to -2^30..2^30
% 2. add csp variables for each step
% 3. add a fresh distinct for all variables every step, do not reuse old constraints
% 3.5. this works, as old constraints are disabled, there fore we use nonstrict constraints
%%% Difference to incqueens2
% use of nonstrict constraints
% releasing the external and removing the constraint
% manual "completion" of the constraints using 2 rules
% no "old" constraints are reused
% no "old" constraints can be found in the solver

#script(python)
import clingo
from clingo import *

def main(prg):
  for i in range(1, 300):
     prg.release_external(function("active",[i-1]))
     prg.ground([("step",[i])])
     # new assumption is true for this step
     prg.assign_external(function("active",[i]),True)
     prg.solve()
#end.

#include "csp.lp".
#program step(n).
#external active(n).

pos(n).

% all further rules are only true in the current active step
:- not &dom(nonstrict) {1..n} = q(X), pos(X), active(n).
:- &dom(nonstrict) {1..n} = q(X), pos(X), not active(n).

:- not &distinct(nonstrict) {q(X) : pos(X)}, active(n).
:- &distinct(nonstrict) {q(X) : pos(X)}, not active(n).
%%%%diagonals
:- not &distinct(nonstrict) {q(X)+X-1 : pos(X)}, active(n).
:- &distinct(nonstrict) {q(X)+X-1 : pos(X)}, not active(n).
:- not &distinct(nonstrict) {q(X)-X+1 : pos(X)}, active(n).
:- &distinct(nonstrict) {q(X)-X+1 : pos(X)}, not active(n).

% add the last queen to the show statement
&show {q(n)}.
