{ y; d } 1.
c :- y.
c :- d.
b :- d.
x:y | a:b :- c.
b :- a.
%% { "options": ["0"]
%% , "solutions": [[],["a","b","c","d"],["c","x","y"]] }
