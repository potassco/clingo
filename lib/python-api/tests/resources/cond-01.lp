{ a(a;b) }.
b(X) :- a(X).
c(X) :- a(X).
:- #count {A,B: a(A),  b(A),  c(A), a(B), b(B), c(B) } >= 2.
#show a/1.
%% { "options": ["0"]
%% , "solutions": [[], ["a(a)"], ["a(b)"]] }
