{ true(e,L,C) } :- var(e,L,C).
true(a,L,C) :- unequal, var(a,L,C).
unequal :- int(V), #sum{ C,Q,L : true(Q,L,C) } != V.
:- not unequal.
var(e,x1,6).
var(e,x2,3).
var(a,y1,-2).
var(a,y2,1).
int(4).
#show true/3.
%% { "options": ["0"]
%% , "solutions": [[],["true(e,x1,6)","true(e,x2,3)"]] }
