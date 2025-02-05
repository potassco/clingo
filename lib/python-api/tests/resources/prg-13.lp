#const n=6.
company(1..n).
owns(X,X+1,51) :- company(X), company(X+1).

controls(X,Y) :- #sum+ { S: owns(X,Y,S); S,Z: controls(X,Z), owns(Z,Y,S) } > 50, company(X), company(Y), X!=Y.

#show controls/2.

%% { "options": ["0"]
%% , "solutions": [ [ "controls(1,2)", "controls(1,3)", "controls(1,4)", "controls(1,5)", "controls(1,6)"
%%                  , "controls(2,3)", "controls(2,4)", "controls(2,5)", "controls(2,6)", "controls(3,4)"
%%                  , "controls(3,5)", "controls(3,6)", "controls(4,5)", "controls(4,6)", "controls(5,6)" ] ] }
