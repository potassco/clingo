#const n=6.

vec(c(1..n,1..n)).

res(agW,N1,N2,NM) :-
    vec(N1), N1 = c(X1,Y1),
    vec(N2), N2 = c(X2,Y2),
    XM=#min{X1;X2}, YM=#min{Y1;Y2},
    NM=#max{XM*XM;YM*YM*YM}.

num(N) :- N = { res(_,N1,N2,NM) }.

#show num/1.

%% { "options": ["0"]
%% , "solutions": [["num(1296)"]] }
