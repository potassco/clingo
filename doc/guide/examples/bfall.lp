flies(tux)        :- bird(tux),    not neg_flies(tux).
flies(tweety)     :- bird(tweety), not neg_flies(tweety).
neg_flies(tux)    :- bird(tux),    not     flies(tux).
neg_flies(tweety) :- bird(tweety), not     flies(tweety).
neg_flies(tux)    :- penguin(tux).
neg_flies(tweety) :- penguin(tweety).
