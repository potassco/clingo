%% { "options": ["0"]
%% , "solutions": [["a"],
%%                 ["a", "b", "c", "g"]] }
{ g }.
a :- 3 != #min { 2: a; 2: b; 3: c }.
c :- b.
b :- c.
b :- g, a.
