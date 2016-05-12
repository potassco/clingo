q(7;8).
q(X,X,X) :- X = 1..3.
#sum [ p(1;2;3) ].

#minimize [ p(1;2) = 3;4 @ 5;6 : q(7;8) ].
#minimize [ p(X) = W @ P : q(X,W,P)  ].
#minimize [ p(X) @ 42 ].

