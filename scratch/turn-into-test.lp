#project p/2.
#project p(X) : q(X).
#project p(a).
#project p(b) : .

#heuristic p(X) : q(X). [1@2,sign]
#heuristic p(X) : q(X). [1@2,level]
#heuristic p(X) : q(X). [1@2,true]
#heuristic p(X) : q(X). [1@2,false]
#heuristic p(X) : q(X). [1@2,factor]
#heuristic p(X) : q(X). [1,init]
#heuristic p(X) : . [1,init]
#heuristic p(X). [1,init]

#edge (a, b).
#edge (x, y) : .
#edge (X, Y) : edge(X,Y).

p(init).
p(sign).
p(true).
p(false).
p(factor).
p(level).
p(levell).
