% run
% cat test_parity.lp | bingo | clasp --cautious
% cat test_parity.lp | bingo -t | bingo | clasp --cautious
% every answer set has to include the atom "all(par)"

p(X) :- not r(X), X=1..2.
r(X) :- not p(X), X=1..2.
c.

fpar(1) :- #odd{}.
fpar(2) :- #even{c}.
fpar(3) :- #even{c, c}.
fpar(4) :- #odd[c, c].
all(fpar) :- {fpar(X) : X=1..4} 0.

tpar(1) :- not #odd[].
tpar(2) :- not #even[c].
tpar(3) :- not #even{c, c}.
tpar(4) :- not #odd[c, c].
tpar(5, X) :- #even[c=X, c=X], X=1..2.
tpar(5) :- tpar(5, X) : X=1..2.
all(tpar) :- tpar(X) : X=1..5.

mpar(1) :- #even{p(1)}.
mfpar(1) :- p(1), mpar(1).
mfpar(1) :- not p(1), not mpar(1).
mfpar(2) :- #even{p(1), r(1)}.
mfpar(3) :- not #even{p(1), not r(1)}.
mfpar(4) :- #odd {p(1), r(1), c, c}.
mpar(5, X) :- #even { p(X), c }, X=1..2.
mfpar(5) :- p(X), not mpar(5, X), X=1..2.
mfpar(5) :- not p(X), mpar(5, X), X=1..2.
mfpar(6) :- #even { p(X) : X=1..2, not r(X) : X=1..2, c }.
all(mpar) :- {mfpar(X) : X=1..6} 0.

#even [].
#odd [] :- opar(1).
hpar(1) :- not opar(1). { opar(1) }.
#odd{c}.
#even{c} :- opar(2).
hpar(2) :- not opar(2). { opar(2) }.
#odd {opar(3), opar(4), c} :- .
hpar(3) :- not 1 {opar(3), opar(4)} 1.
#odd [opar(5)=1, opar(6)=2].
hpar(4) :- opar(5).
all(hpar) :- hpar(X) : X=1..4.

all(par) :- all(tpar), all(fpar), all(mpar), all(hpar).

#hide.
#show all/1.
