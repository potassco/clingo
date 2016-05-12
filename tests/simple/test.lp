p(1).
p(2).
p(3).
p(4).
p(5).
p(6).
p(7).
p(8).
p(9).
p(10).
a(1,1,1,1).
a(A,B,C,D) :- b(A,B,C,D), p(A), p(B), p(C), p(D).
b(A,B,C,D) :- a(A,B,C,D), p(A), p(B), p(C), p(D).
