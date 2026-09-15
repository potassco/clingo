%% { "options": ["0"]
%% , "solutions": [[],
%%                 ["chain(1,2)", "d(1)", "d(2)"],
%%                 ["d(1)"],
%%                 ["d(2)"]] }
{ d(1..2) }.
chain(X,Y) :- (X,Y) = #sort { Z : d(Z) }.
