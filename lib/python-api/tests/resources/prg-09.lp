#const b=18.

sqrt_b(M) :- M = 1..b, M*M <= b, (M+1)*(M+1) > b.
composite(I*J) :- sqrt_b(M), I = 2..M, J = I..(b/I).
prime(I) :- I = 2..b, not composite(I).

#show prime/1.

%% { "options": ["0"]
%% , "solutions": [["prime(2)", "prime(3)", "prime(5)", "prime(7)", "prime(11)", "prime(13)", "prime(17)"]] }
