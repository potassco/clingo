#const l = 35.
#const o = 8.
&dom { 1..l } = p(P) :- P=1..o.

&sum { p(1) } = 1.
&sum { p(P) } < p(P+1) :- P=1..o-1.

&distinct{ p(Q) - p(P) : P < Q, P=1..o-1, Q=P+1..o }.
