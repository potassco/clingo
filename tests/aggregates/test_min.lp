% run
% cat test_min.lp | bingo | clasp --cautious
% cat test_min.lp | bingo -t | bingo | clasp --cautious
% every answer set has to include the atom "all(min)"

p(X) :- not r(X), X=1..2.
r(X) :- not p(X), X=1..2.
c.

fmin(1) :- 1 #max[].
fmin(2) :- not #max[] 1.
fmin(3) :- #min[] 1.
fmin(4) :- not 1 #min[].
fmin(5) :- not 1 #max [c] 2.
fmin(6) :- not 1 #min [c] 2.
fmin(7) :- 3 #max [p(1)=2, r(1)].
fmin(8) :- #min [p(1)=2, r(1)] 0.
fmin(9) :- not #max [p(1)=2, r(1)] 2.
fmin(10) :- not 0 #min [p(1)=2, r(1)].
fmin(11) :- 1 #max [p(1)=2, r(1), c=3] 1.
fmin(12) :- not 1 #max [p(1)=2, r(1), c=3].
fmin(13) :- 3 #min [p(1)=2, r(1), c=1] 3.
fmin(14) :- not #min [p(1)=2, b=3, c] 1.
all(fmin) :- { fmin(X) : X=1..14 } 0.

tmin(1) :- not 1 #max[].
tmin(2) :- #max[] 1.
tmin(3) :- not #min[] 1.
tmin(4) :- 1 #min[].
tmin(5) :- 1 #max [c] 2.
tmin(6) :- 1 #min [c] 2.
tmin(7) :- not 3 #max [p(1)=2, r(1)].
tmin(8) :- not #min [p(1)=2, r(1)] 0.
tmin(9) :- #max [p(1)=2, r(1)] 2.
tmin(10) :- 0 #min [p(1)=2, r(1)].
tmin(11) :- not 1 #max [p(1)=2, r(1), c=3] 1.
tmin(12) :- 1 #max [p(1)=2, r(1), c=3].
tmin(13) :- not 3 #min [p(1)=2, r(1), c=1] 3.
tmin(14) :- #min [p(1)=2, b=3, c] 1.
all(tmin) :- tmin(X) : X=1..14.

mmin(1) :- 2 #max [p(1)=2, r(1)].
mfmin(1) :- #even{r(1), mmin(1)}.
mmin(2) :- #max [p(1)=2, r(1)] 1.
mfmin(2) :- #even{p(1), mmin(2)}.
mmin(3) :- 1 #max [p(1)=2, r(1), c] 1.
mfmin(3) :- #even{p(1), mmin(3)}.
mmin(4) :- 1 #max [p(1)=2, r(1)] 2.
mfmin(4) :- not mmin(4).
mfmin(5) :- 0 #max [p(1)=2, r(1)] 0.
mmin(6) :- 2 #min [p(1)=2, r(1)].
mfmin(6) :- #even{r(1), mmin(6)}.
mmin(7) :- #min [p(1)=2, r(1)] 1.
mfmin(7) :- #even{p(1), mmin(7)}.
mmin(8) :- 1 #min [p(1)=0, r(1), c] 1.
mfmin(8) :- #even{p(1), mmin(8)}.
mmin(9) :- 1 #min [p(1)=2, r(1)] 2.
mfmin(9) :- not mmin(9).
mfmin(10) :- 3 #min [p(1)=2, r(1)] 3.
mmin(11) :- 1 #min [p(X)=X : X=1..2] 1.
mfmin(11) :- #even{mmin(11), not p(1)}.
mmin(12, X) :- X-1 #min[r(X)=X-1] X-1, X=1..2.
mfmin(12) :- #odd{r(X), mmin(12, X)}, X=1..2.
all(mmin) :- { mfmin(X) : X=1..12 } 0.

amin(1, X) :- X = #max [p(1)].
amin(1) :- amin(1, #infimum), not p(1), 1 [amin(1, X) ] 1.
amin(1) :- amin(1, 1), p(1), 1 [amin(1, X) ] 1.
amin(2, X) :- X = #max [p(1)=2, r(1)].
amin(2) :- amin(2, 1), r(1), 1 [amin(2, X) ] 1.
amin(2) :- amin(2, 2), p(1), 1 [amin(2, X) ] 1.
amin(3, X) :- X = #max [p(1)=2, r(1), c].
amin(3) :- amin(3, 1), r(1), 1 [amin(3, X) ] 1.
amin(3) :- amin(3, 2), p(1), 1 [amin(3, X) ] 1.
amin(4, X) :- X = #max [p(1)=2, r(1), c=2].
amin(4) :- amin(4, 2), 1 [amin(4, X) ] 1.
amin(5, X) :- X = #max [p(1)=2, r(1), c=3].
amin(5) :- amin(5, 3), 1 [amin(5, X) ] 1.
amin(6, X) :- X = #max [p(1)=f, r(1)=g(1), c=h].
amin(6) :- #odd[amin(6, h), r(1)], #even [amin(6, g(1)), r(1)].
amin(7, X) :- X = #min [p(1)=2].
amin(7) :- #even{p(1), amin(7, 2)}, #even{r(1), amin(7, #supremum)}.
amin(8, X) :- X = #min [p(1)=2, r(1)].
amin(8) :- #even{p(1), amin(8, 2)}, #even{r(1), amin(8, 1)}.
amin(9, X) :- X = #min [p(1)=2, r(1), c=3].
amin(9) :- #even{p(1), amin(9, 2)}, #even{r(1), amin(9, 1)}.
amin(10, X) :- X = #min [p(1)=2, r(1)=3, c=2].
amin(10) :- amin(10, 2).
amin(11, X) :- X = #min [p(1)=2, r(1)=3, c].
amin(11) :- amin(11, 1).
all(amin) :- amin(X) : X=1..11.



#max[omin(1)].
#max[c].
1 #max[omin(2)=0] :- omin(3).
hmin(1) :- not omin(3). { omin(3) }.
#max[omin(4)=0] 1.
#max[omin(6)=0] -1.
hmin(2) :- not omin(6).
2 #max[omin(7)=2] 2.
hmin(3) :- omin(7).
1 #max[c,omin(8)=2].
#min[c1b].
#min[c].
#min[omin(9)=2] 1 :- omin(10).
hmin(4) :- not omin(10). { omin(10) }.
1 #min[omin(11)=2].
1 #min[omin(12)=0].
hmin(5) :- not omin(12).
2 #min[omin(13)=2] 2.
hmin(6) :- omin(13).
#max[c,omin(14)=0] 1.
all(hmin) :- hmin(X) : X=1..6.

all(min) :- all(fmin), all(mmin), all(tmin), all(amin), all(hmin).

#hide.
#show all/1.
