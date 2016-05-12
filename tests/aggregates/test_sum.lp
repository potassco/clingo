% run
% cat test_sum.lp | bingo | clasp --cautious
% cat test_sum.lp | bingo -t | bingo | clasp --cautious
% every answer set has to include the atom "all(sum)"

p(X) :- not r(X), X=1..2.
r(X) :- not p(X), X=1..2.

asum(1, X) :- X = #sum [p(1)=2].
asum(1) :- #odd[asum(1, 0),asum(1,2)], not asum(1, 1).
all(asum) :- asum(X) : X=1..1.

all(sum) :- all(asum).

#hide.
#show all/1.
