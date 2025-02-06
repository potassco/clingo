b.
a(a).

a :- a(X).
c(a) :- a.
a(b) :- c(*), b.
x :- a(b).
%% { "options": ["0"]
%% , "solutions": [["a", "a(a)", "b", "a(b)", "c(a)", "x"]] }
