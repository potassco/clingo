{ p(a,X); p(b,X) } >= 1 :- X=1..1.
{ q(a,X); q(b,X) } >= 1 :- X=1..2.
:~ p(X,Y). [1,X,Y]
:~ q(X,Y). [2,X,Y]

#heuristic p(X,Y). [1,true]
