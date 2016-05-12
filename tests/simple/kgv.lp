#hide.
#show kgv/3.

%*
% gringo
num(1..10).
kgv(1..n*n).
gv(A,B,N)  :- num(A;B), A < B, N <= A * B, kgv(N), N #mod A == 0, N #mod B == 0.
lt(A,B,N)  :- gv(A,B,N;M), M < N.
kgv(A,B,N) :- gv(A,B,N), not lt(A,B,N).
kgv(B,A,N) :- gv(A,B,N), not lt(A,B,N).
kgv(A,A,A) :- num(A).
*%

% bingo
num(1).
num(Y) :- num(X), X < 10, Y = X + 1.

gcd(A,0,A) :- num(A).
gcd(A,A,A) :- num(A).
gcd(A,B,N) :- gcd(B,AA,N), B > 0, num(A), A > B, AA = A #mod B.
gcd(A,B,N) :- gcd(B,A,N), A > 0, B > A.

kgv(A,B,M) :- gcd(A,B,N), B > 0, AB = A * B, M = AB / N.

