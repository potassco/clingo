q(1).
p(X); not p(X); not r(X) :- q(X).
r(X); not r(X) :- q(X).
#show r/1.
#show p/1.
%% { "options": ["0"]
%% , "solutions": [[],["p(1)","r(1)"],["r(1)"]] }
