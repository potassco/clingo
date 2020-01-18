letter(s;e;n;d;m;o;r;y).

%&dom {0..9} = X :- letter(X).
&sum { X } <= 10 :- letter(X).
&sum { X } >= 0 :- letter(X).

&sum {             1000*s + 100*e + 10*n + 1*d
     ;             1000*m + 100*o + 10*r + 1*e
     } = 10000*m + 1000*o + 100*n + 10*e + 1*y.
&sum { m } != 0.

%&distinct {X : letter(X)}.
&sum { X } != Y :- letter(X), letter(Y), X < Y.

%&show {X : letter(X)}.
