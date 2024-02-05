#const t=2.

#program p(u).
b(t+1).
a :- b(u-1).

a; b: c :- d.
{ a; b: c } > 2 :- d.

p :- #sum+ { 1:aa } >= 2.
n :- #sum+ { 1:bb } <= 2.
