% the head cycle passes through rules in a later component
{ c }.
a : c | b.
a :- b.
b :- a.

%% { "options": ["0"]
%% , "solutions": [["a", "b"], ["a", "b", "c"]] }
