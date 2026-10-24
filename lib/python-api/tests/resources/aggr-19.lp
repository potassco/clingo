%% { "options": ["0"]
%% , "solutions": [["p(1)"],
%%                 ["q"]] }
{ p(1) }.
q :- #max { X : p(X) } < 0.
