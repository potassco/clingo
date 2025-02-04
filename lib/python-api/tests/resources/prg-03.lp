r(1).
p(X) :- not not p(X), r(X).
q(X) :- p(X).
%% { "options": ["0"]
%% , "solutions": [["p(1)", "q(1)", "r(1)"], ["r(1)"]] }
