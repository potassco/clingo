#include <incmode>.

#program base.
% Define
location(table).
location(X) :- block(X).
holds(F,0) :- init(F).

#program step(t).
% Generate
{ move(X,Y,t) : block(X), location(Y), X != Y } = 1.
% Test
:- move(X,Y,t), holds(on(A,X),t-1).
:- move(X,Y,t), holds(on(B,Y),t-1), B != X, Y != table.
% Define
moved(X,t) :- move(X,Y,t).
holds(on(X,Y),t) :- move(X,Y,t).
holds(on(X,Z),t) :- holds(on(X,Z),t-1), not moved(X,t).

#program check(t).
% Test
:- query(t), goal(F), not holds(F,t).

% Display
#show move/3.

#program base.
% Sussman Anomaly
%
block(b0).
block(b1).
block(b2).
%
% initial state:
%
%  2
%  0 1
% -------
%
init(on(b1,table)).
init(on(b2,b0)).
init(on(b0,table)).
%
% goal state:
%
%  2
%  1
%  0
% -------
%
goal(on(b1,b0)).
goal(on(b2,b1)).
goal(on(b0,table)).
