% Generate
1 { cycle(X,Y) : edge(X,Y) } 1 :- node(X).
1 { cycle(X,Y) : edge(X,Y) } 1 :- node(Y).
% Define
reached(Y) :- cycle(1,Y).
reached(Y) :- cycle(X,Y), reached(X).
% Test
:- node(Y), not reached(Y).
% Display
#hide.
#show cycle/2.
