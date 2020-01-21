letter(s;e;n;d;m;o;r;y).

&dom {0..9} = X :- letter(X).

&sum {             1000*s + 100*e + 10*n + 1*d
     ;             1000*m + 100*o + 10*r + 1*e
     } = 10000*m + 1000*o + 100*n + 10*e + 1*y.
&sum { m } != 0.

&distinct {X : letter(X)}.

%&show {X : letter(X)}.
