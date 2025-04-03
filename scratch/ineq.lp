%%% translating
%p :- { a; b; c } != 2.

%% using a conditional literal and not existent gringo syntax :) it is equivalent to
% p :- ({ a; b; c } > 2 : { a; b; c } > 1).

%% the head of the conditional literal can be pulled out using an auxiliary atom
% aux :- { a; b; c } > 2.
% p :- (aux : { a; b; c } > 1).

%% we can get rid of the conditional literal using cabalar's/ferraris' rewriting
% aux1 :- { a; b; c } > 2.
% p :- aux.
% p :- not { a; b; c } > 1.
% p | { a; b; c } > 1 :- not not aux.

%% unfortunately this leads to a head aggregate that (unlike gringo's aggregates) has to be minimized
%% therefore we can use a (more or less) standard Sinz counter that uses equivalences instead of just implications
aux :- { a; b; c } > 2.

p :- aux.
p :- not s_c2.

s_b1 :- a.
s_b1 :- b.
a; b :- s_b1.

s_b2 :- a, b.
a :- s_b2.
b :- s_b2.

s_c2 :- c, s_b1.
s_c2 :- s_b2.
c; s_b2 :- s_c2.
s_b1; s_b2 :- s_c2.

p; s_c2 :- not not aux.

%%% just some equivalences to get an interesting answer set
a :- p.
b :- p.
c :- p.

p :- a.
p :- b.
p :- c.
