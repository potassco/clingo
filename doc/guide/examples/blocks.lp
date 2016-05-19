#base.
% Define
location(table).
location(X) :- block(X).
holds(F,0) :- init(F).
%
#cumulative t.
% Generate
1 { move(X,Y,t) : block(X) : location(Y) : X != Y } 1.
% Test
:- move(X,Y,t),
   1 { holds(on(A,X),t-1) : block(A) : A != X,
       holds(on(B,Y),t-1) : block(B) : B != X : B != Y : Y != table }.
% Define
holds(on(X,Y),t) :- move(X,Y,t).
holds(on(X,Z),t) :- holds(on(X,Z),t-1), block(X), location(Z), X != Z,
                    { move(X,Y,t) : location(Y) : Y != X : Y != Z } 0.
%
#volatile t.
% Test
:- goal(F), not holds(F,t).
%
#base.
% Display
#hide.
#show move/3.
