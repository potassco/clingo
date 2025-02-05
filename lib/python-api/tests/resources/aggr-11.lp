{c; d}.
:- not c, not d.
:- c, d.
a :- #sum{ 1: b; -1: a } >= 0, c.
b :- a.
a :- #sum{ 1: b; -1: x } >= 0, d.
x :- a.
b :- a.

%% { "options": ["0"]
%% , "solutions": [["a", "b", "c", "x"]] }
