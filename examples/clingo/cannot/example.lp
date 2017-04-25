&cannot { "pick outgoing edges from % both to % and %", X, Y, Z: Y > X }
:- in(X,Y), in(Y,Z), Y!=Z.

7 { in(1..3, 1..3) }.
