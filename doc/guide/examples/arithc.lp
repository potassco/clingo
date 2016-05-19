num(1).  num(2).
eq (X,Y) :- X   ==  Y, num(X), num(Y).
neq(X,Y) :- X   !=  Y, num(X), num(Y).
lt (X,Y) :- X   <   Y, num(X), num(Y).
leq(X,Y) :- X   <=  Y, num(X), num(Y).
gt (X,Y) :- X   >   Y, num(X), num(Y).
geq(X,Y) :- X   >=  Y, num(X), num(Y).
all(X,Y) :- X-1 < X+Y, num(X), num(Y).
non(X,Y) :- X/X > Y*Y, num(X), num(Y).
