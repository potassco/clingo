#const h=3.

time(0..h).
volume(1..10).

0 $<= $b(T) $<= 20 :- time(T).
0 $<= $f(T) $<= 20 :- time(T).

1 $<= $m $<= 16.

$b(0) $= 10.
$f(0) $= 10.

$b(T) $= 2 $* $b(T-1) $- $m      :- time(T), T>0.
$f(T) $=      $f(T-1) $- $b(T-1) :- time(T), T>0, not exogenous(T).

$f(T) $=      $f(T-1) $- $b(T-1) $+ V :- add(V,T), volume(V), time(T), T>0.

 { add(V,T) : volume(V) } 1 :- time(T), T>0.

exogenous(T) :- add(V,T).

#minimize{ 1@2,T : exogenous(T) }.
#minimize{ V@1,T : add(V,T) }.
#minimize{ 1@3,X : $m $>= X, X=1..16 }.

query(T) :- $b(T) $>10, $f(T) $<5, time(T), T>0.
% query(T) :- $b(T) $> 10, time(T), T>0.

 :- not query(h).

#show.
#show $b/1.
#show $f/1.
#show add/2.
#show $m/0.

