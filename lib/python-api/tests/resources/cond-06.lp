{ true(e,L,C) } :- var(e,L,C).
true(a,L,C) :- unequal, var(a,L,C).
unequal :- int(V), #sum{ C,Q,L : true(Q,L,C) } != V.
:- not unequal.
var(e,x1,2).
var(e,x2,4).
var(a,y1,-2).
var(a,y2,4).
int(4).
#show.
#show true(X,Y) : true(e,X,Y).
%% { "options": ["0"]
%% , "solutions": [] }
