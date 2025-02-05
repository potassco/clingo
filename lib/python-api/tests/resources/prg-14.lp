node(a;b;c;d;e).
edge(a,c).
edge(b,c;b,e).
edge(c,d;c,e).

reach(V) :- node(V), not edge(*,V).
reach(V) :- node(V), #count { U: reach(U), edge(U,V) } >= 2.

#show reach/1.

%% { "options": ["0"]
%% , "solutions": [["reach(a)", "reach(b)", "reach(c)", "reach(e)"]] }
