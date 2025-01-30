a(1) :- #count { }.
a(2) :- not #count { }.
a(3) :- not not #count { }.
a(4) :- { }.
a(5) :- not { }.
a(6) :- not not { }.
%% { "options": ["0"]
%% , "solutions": [["a(1)", "a(2)", "a(3)", "a(4)", "a(5)", "a(6)"]] }
