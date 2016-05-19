fruit(apple).  fruit(peach).  foul(peach).
natural(X) :- fruit(X).
healthy(X) :- natural(X), not foul(X).
tasty(X)   :- healthy(X), not bitter(X).
