left       (7).
right            (2).
plus       (L  +  R)  :- left(L), right(R).
minus      (L  -  R)  :- left(L), right(R).
times      (L  *  R)  :- left(L), right(R).
divide     (L  /  R)  :- left(L), right(R).
divide     (R div L)  :- left(L), right(R).
modulo     (L mod R)  :- left(L), right(R).
absolute   (abs(- R)) :-          right(R).
complement     (~ R)  :-          right(R).
and        (L  &  R)  :- left(L), right(R).
or         (L  ?  R)  :- left(L), right(R).
xor        (L  ^  R)  :- left(L), right(R).
