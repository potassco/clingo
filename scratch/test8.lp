p(1..10).
c(X,Z) :- p(X), p(Z), X < Z, not X < Y < Z: p(Y).
