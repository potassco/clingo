#hide.
#show queen/2.

num(1..10).

queen(X,Y) :- not nqueen(X,Y), num(X), num(Y).
nqueen(X,Y) :- not queen(X,Y), num(X), num(Y).

d1(X,X-Y) :- queen(X,Y).
 :- d1(X1,D), d1(X2,D), X1 < X2.

d2(X,X+Y) :- queen(X,Y).
 :- d2(X1,D), d2(X2,D), X1 < X2.

 :- queen(X,Y1), queen(X,Y2), Y1 < Y2.
 :- queen(X1,Y), queen(X2,Y), X1 < X2.

h1(X) :- queen(X,Y).
 :- num(X), not h1(X).

h2(Y) :- queen(X,Y).
 :- num(Y), not h2(Y).

