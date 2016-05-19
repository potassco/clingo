 sym(a).  sym(b).
 num(1).  num(2).
 mix(A;B,M;N) :- sym(A;B), num(M;N), not -mix(M;N,A;B).
-mix(M;N,A;B) :- sym(A;B), num(M;N), not  mix(A;B,M;N).
