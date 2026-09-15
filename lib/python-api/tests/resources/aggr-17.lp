%% { "options": ["0"]
%% , "solutions": [["chain(a,1,2)", "chain(b,1,3)", "d(a,1)", "d(a,2)", "d(b,1)", "d(b,3)", "key(a)", "key(b)"]] }
key(a;b).
d(a,1).
d(a,2).
d(b,1).
d(b,3).
chain(K,X,Y) :- key(K), (X,Y) = #sort { Z : d(K,Z) }.
