b.
a(a).

a :- a(X).
c(a) :- a.
a(b) :- c(*), b.
x :- a(b).
%% { "options": ["0"]
%% , "solutions": [["a", "a(a)", "a(b)", "b", "c(a)", "x"]] }
