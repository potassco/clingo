#const n=3000.
company(1..n).
owns(X,X+1,51) :- company(X), company(X+1).

controls(X,Y) :- #sum+ { S: owns(X,Y,S); S,Z: controls(X,Z), owns(Z,Y,S) } > 50, company(X), company(Y), X!=Y.
