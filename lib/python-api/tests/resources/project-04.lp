d(1).
p(X) :- not q(*), d(X).
q(X) :- not p(*), d(X).
#show p/1.
#show q/1.
%% { "options": ["0"]
%% , "solutions": [["p(1)"], ["q(1)"]] }
