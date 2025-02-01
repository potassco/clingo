node(a;b;c;d).
edge(a,b;b,(c;d);c,(a;d);d,a).
1 { path(X,Y) : edge(X,Y) } 1 :- node(X).
1 { path(X,Y) : edge(X,Y) } 1 :- node(Y).
start(X) :- X = #min { Y : node(Y) }.
#edge (X,Y) : path(X,Y), not start(Y).
#show path/2.
%% { "options": ["0"]
%% , "solutions": [["path(a,b)", "path(b,c)", "path(c,d)", "path(d,a)"]] }
