%% { "options": ["0"]
%% , "solutions": [["d(1)", "d(2)", "d(3)", "p(#inf,1)", "p(1,2)", "p(2,3)", "p(3,#sup)"]] }
d(1..3).
p(Y,Z) :- (Y,Z) = #sort { #inf; X: d(X); #sup }.
