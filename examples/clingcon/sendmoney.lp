#include "csp.lp".

digit(1,3,s).   digit(2,3,m).   digit(sum,4,m).
digit(1,2,e).	digit(2,2,o).	digit(sum,3,o).
digit(1,1,n).	digit(2,1,r).	digit(sum,2,n).
digit(1,0,d).	digit(2,0,e).	digit(sum,1,e).
				digit(sum,0,y).
base(10).
exp(E) :- digit(_,E,_).

power(1,0).
power(B*P,E) :- base(B), power(P,E-1), exp(E), E>0.

number(N) :- digit(N,_,_), N!= sum.
high(D) :- digit(N,E,D), not digit(N,E+1,_).

&dom {0..9} = X :- digit(_,_,X).

&sum { M*D : digit(N,E,D),   power(M,E), number(N);
      -M*D : digit(sum,E,D), power(M,E)           } = 0.

%&sum { D : high(D) } > 0.
&sum { D } > 0 :- high(D).

%added distinct for all digits
&distinct {D : digit(_,_,D) }.

&show {D : digit(_,_,D)}.
