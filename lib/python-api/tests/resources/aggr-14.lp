%% { "options": ["0"]
%% , "solutions": [["sum(((1,(time,0)),),eq,10)", "sum(((1,(time,1)),),eq,10)"],
%%                 ["sum(((1,(time,0)),),eq,10)", "sum(((1,(time,1)),),gt,10)"],
%%                 ["sum(((1,(time,0)),),eq,10)", "sum(((1,(time,1)),),lt,10)"],
%%                 ["sum(((1,(time,0)),),gt,10)", "sum(((1,(time,1)),),eq,10)"],
%%                 ["sum(((1,(time,0)),),lt,10)", "sum(((1,(time,1)),),eq,10)"]] }
#const active=1.

step(0..1).

obs(happens(transfer(account1,account2,2)),10).

comparison(((1,(time,S)),),T) :- obs_at(T), step(S).

{ sum(LE,(lt;eq;gt),B) } = 1 :- comparison(LE,B).

obs_at(T) :- obs(X,T).
map_at(T) :- map(T,X).
map(T,S) :- obs_at(T), sum(((1,(time,S)),),eq,T).
         :- obs_at(T), not map_at(T).

true.
happens(E,S) :- obs(happens(E),T), map(T,S), not true, active=1.

comparison(((1, (balance(A1), T)),),TransM12) :- happens(transfer(A1,A2,TransM12),T).

#show sum/3.
