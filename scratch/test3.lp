char_to_digit(X,X) :- X=0..9.

digit(Y-1,Y,D) :- char(C,Y), char_to_digit(C,D), Y > 0.

pos(A) :- digit(X,Y,A).

sign(X, 1) :- char(p,X).
sign(X,-1) :- char(m,X).

num_1(X,Y,A)      :- digit(X,Y,A), not pos(X).
num_1(X,Z,10*A+B) :- num_1(X,Y,A), digit(Y,Z,B).
num(X,Y,A)        :- num_1(X,Y,A), not pos(Y+1).

par_expr(Y-1,Z+1,A) :- char(o,Y), expr(Y,Z,A), char(c,Z+1), Y >= 0.

sign_expr(Y-1,Z,A*S) :- par_expr(Y,Z,A), sign(Y,S), Y > 0.
sign_expr(Y-1,Z,A*S) :- num(Y,Z,A),      sign(Y,S), Y > 0.

expr(0,Y,A)   :- num(0,Y,A).
expr(X,Y,A)   :- char(o,X), num(X,Y,A).
expr(X,Y,A)   :- par_expr(X,Y,A).
expr(X,Y,A)   :- sign_expr(X,Y,A).
expr(X,Z,A+B) :- expr(X,Y,A), sign_expr(Y,Z,B).

cmp(Y-1,A)   :- char(g,Y), num(Y,N,A), Y > 0.
cmp(Y-1,A*S) :- char(g,Y), sign(Y+1,S), num(Y+1,N,A), Y > 0.

eq(A,A) :- expr(0,Y,A), cmp(Y,A).
lt(A,B) :- expr(0,Y,A), cmp(Y,B), A < B.
gt(A,B) :- expr(0,Y,A), cmp(Y,B), A > B.

char(p,1).
char(0,2).
char(m,3).
char(o,4).
char(o,5).
char(6,6).
char(m,7).
char(7,8).
char(p,9).
char(3,10).
char(p,11).
char(o,12).
char(4,13).
char(c,14).
char(c,15).
char(m,16).
char(2,17).
char(p,18).
char(o,19).
char(p,20).
char(0,21).
char(c,22).
char(c,23).
char(p,24).
char(2,25).
char(m,26).
char(o,27).
char(p,28).
char(1,29).
char(c,30).
char(g,31).
char(m,32).
char(4,33).
