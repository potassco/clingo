% run
% cat test_avg.lp | bingo | clasp --cautious
% cat test_avg.lp | bingo -t | bingo | clasp --cautious
% every answer set has to include the atom "all(avg)"

p(X) :- not r(X), X=1..3.
r(X) :- not p(X), X=1..3.
c.

favg(1) :- not 1 #avg [].
favg(2) :- not #avg [] -1.
favg(3) :- 2 #avg [c].
favg(4) :- #avg [c] -1.
favg(5) :- not 1 #avg [c].
favg(6) :- not #avg [c] 1.
favg(7) :- not 0 #avg [p(1)=2, r(1)].
favg(8) :- not #avg [p(1)=-2, r(1)=0] 1.
favg(9) :- not 0 #avg [p(1)=4, r(1)=2] 4.
favg(10) :- not 0 #avg [p(1)=2, r(1)].
favg(11) :- not #avg [p(1)=-4, r(1)=-2] 0.
favg(12) :- 2 #avg [p(1), r(1), c].
favg(13) :- #avg [p(1), r(1), c] 0.
favg(14) :- not 1 #avg [p(1), r(1), c].
favg(15) :- not #avg [p(1), r(1), c] 1.
all(favg) :- {favg(X) : X=1..Y} 0, Y=15.

tavg(1) :- 0 #avg [].
tavg(2) :- #avg [] 0.
tavg(3) :- not 2 #avg [c].
tavg(4) :- not #avg [c] -1.
tavg(5) :- 1 #avg [c].
tavg(6) :- #avg [c] 1.
tavg(7) :- 0 #avg [p(1)=2, r(1)=1].
tavg(8) :- #avg [p(1)=-2, r(1)=0] 1.
tavg(9) :- 0 #avg [p(1)=4, r(1)=2] 4.
tavg(10) :- 0 #avg [p(1)=2, r(1)=1].
tavg(11) :- #avg [p(1)=-4, r(1)=-2] 0.
tavg(12) :- not 2 #avg [p(1), r(1), c].
tavg(13) :- not #avg [p(1), r(1), c] 0.
tavg(14) :- 1 #avg [p(1), r(1), c].
tavg(15) :- #avg [p(1), r(1), c] 1.
all(tavg) :- Y {tavg(X) : X=1..Y }, Y=15.

mavg(1) :- 0 #avg [p(1)=-1, r(1)].
mfavg(1) :- #even{p(1), mavg(1)}.
mavg(2) :- #avg [p(1)=-1, r(1)] 0.
mfavg(2) :- #even{r(1), mavg(2)}.
mavg(3) :- not 4 #avg [p(1)=3, r(1)].
mfavg(3) :- not mavg(3).
mavg(4) :- not #avg [p(1)=2, r(1)] 0.
mfavg(4) :- not mavg(4).
mavg(5) :- 2 #avg [not p(1)=3, not r(1)] 4.
mfavg(5) :- #even{mavg(5), p(1)}.
mavg(6) :- 2 #avg [p(1)=2, not r(1)=3, c].
mfavg(6) :- #even{mavg(6), r(1)}.
mavg(7) :- not 2 #avg [not p(1)=3, not r(1)] 4.
mfavg(7) :- #even{mavg(7), r(1)}.
mavg(8) :- not 2 #avg [p(1)=2, not r(1)=3, c].
mfavg(8) :- #even{mavg(8), p(1)}.
mavg(9) :- 0 #avg [p(1)=1, r(1)=2, c=3] 2. 
mfavg(9) :- #even{mavg(9), r(1)}.
mavg(10) :- not 0 #avg [p(1)=1, r(1)=2, c=3] 2.
mfavg(10) :- #even{mavg(9), r(1)}.
mavg(11) :- 2 #avg [ not p(X) = X : X=1..3 ] 2.
mfavg(11) :- p(2), #even{p(1), p(3)}, not mavg(11).
mfavg(11) :- p(1), p(2), p(3), not mavg(11).
mavg(12, X) :- 2 #avg [ r(X), not p(X)=3, c=2 ] 2, X=1..2.
mfavg(12) :- { mavg(12, X) : X=1..2 } 1.
all(mavg) :- {mfavg(X) : X=1..12} 0.

0 #avg [].
#avg [] 0.
1 #avg [c].
#avg [c] 1.
2 #avg [c] :- oavg(1).
havg(1) :- not oavg(1). { oavg(1) }.
#avg [c] -1 :- oavg(2).
havg(2) :- not oavg(2). { oavg(2) }.
0 #avg [oavg(3)=2, oavg(4)=1].
#avg [oavg(5)=-2, oavg(6)=0] 1.
0 #avg [oavg(7)=4, oavg(8)=2] 4.
2 #avg [oavg(9), c] :- oavg(10).
havg(3) :- not oavg(10). { oavg(10) }.
#avg [oavg(11), c] 0 :- oavg(12).
havg(4) :- not oavg(12). { oavg(12) }.
2 #avg [oavg(14)=1, oavg(15)=3, c=2] 2.
havg(5) :- #even[oavg(14), oavg(15)].
all(havg) :- Y {havg(X) : X=1..Y }, Y=5.

all(avg) :- all(favg), all(tavg), all(mavg), all(havg).

#hide.
#show all/1.
