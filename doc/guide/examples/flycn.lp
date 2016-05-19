 flies(X) :- bird(X), not -flies(X).
-flies(X) :- bird(X), not  flies(X).
-flies(X) :- penguin(X).
