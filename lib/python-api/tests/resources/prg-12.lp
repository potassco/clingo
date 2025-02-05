company(c1;c2;c3;c4).
owns(c1,c2,60;c1,c3,20;c2,c3,35;c3,c4,51).

controls(X,Y) :- #sum+ { S: owns(X,Y,S); S,Z: controls(X,Z), owns(Z,Y,S) } > 50, company(X), company(Y), X!=Y.

#show controls/2.

%% { "options": ["0"]
%% , "solutions": [["controls(c1,c2)", "controls(c1,c3)", "controls(c1,c4)", "controls(c3,c4)"]] }
