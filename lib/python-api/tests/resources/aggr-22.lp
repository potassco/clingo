%% { "options": ["0"]
%% , "solutions": [["a"]] }
a :- #sum+ { 1: b } != 2.
b :- #sum+ { 1: a; 2: a; 3: x; 3: y } != 3.
x :- b.
y :- b.
