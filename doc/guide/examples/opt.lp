1 { hotel(1..5) } 1.
star(1,5).   star(2,4).   star(3,3).  star(4,3).  star(5,2).
cost(1,170). cost(2,140). cost(3,90). cost(4,75). cost(5,60).
main_street(4).
noisy :- hotel(X), main_street(X).
maximize [ hotel(X) : star(X,Y) = Y ].
minimize [ hotel(X) : cost(X,Y) : star(X,Z) = Y/Z ].
minimize { noisy }.
