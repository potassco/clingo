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

a :- p.
b :- p.
c :- p.

p :- a.
p :- b.
p :- c.

%% { "options": ["0"]
%% , "solutions": [["a","aux","b","c","p","s_b1","s_b2","s_c2"]] }
