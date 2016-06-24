#include "csp.lp".
#const n = 10.

pos(1..n).

&dom {1..n} = q(X) :- pos(X).

&distinct {q(X) : pos(X)}.
%diagonals
&distinct {q(X)+X-1 : pos(X)}.
&distinct {q(X)-X+1 : pos(X)}.

&show {q(X) : pos(X)}.
