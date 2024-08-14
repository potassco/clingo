d(1,a).
d(1,b).
d(2,a).
d(2,b).
d(2,c).
d(3,a).
a(0).
a(X) :- a(X-1), #count { Y: d(X,Y) } >= 2.
