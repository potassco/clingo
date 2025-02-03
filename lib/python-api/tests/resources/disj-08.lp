r(1).
q(1,2).
p(1..Y) : q(X,Y) | not p(1..Y) : q(X,Y) :- r(X).
#show p/1.
%% { "options": ["0"]
%% , "solutions": [[]] }
