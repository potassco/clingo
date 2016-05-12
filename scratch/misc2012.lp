%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% options (values: 0 for off, 1 for on) %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% group clique elements sharing relations
#const cliqueRelations = 1.

% include redundant cardinality constraints for cliques
#const cliqueShortcuts = 1.

% group clique elements by package names for optimization
#const cliqueBreak = 1.

% limit of values per key for counter implementation (numeric)
#const alignCounter = 30.

% output solution in terms of in/2
#const outPutIn = 1.

% trace derivations of objectives' elements (for debugging only)
% #const trace = 0.

%%%%%%%%%%%%%%%%%%%%%%%%%%%
% reify package relations %
%%%%%%%%%%%%%%%%%%%%%%%%%%%

relate(P,V,con,D) :- unit(P,V,in), conflict(P,V,D).
relate(P,V,dep,D) :- unit(P,V,in), depends(P,V,D).
relate(P,V,sat,D) :- unit(P,V,in), satisfies(P,V,D).
relate(P,V,rec,D) :- recommends(P,V,D,W), W != 0.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% analyze conflict cliques %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%

clique(I) :- clique(I,_,_).

realClique(I)     :- clique(I), 2 { unit(P,V,in) : clique(I,P,V) }.
realClique(I,P,V) :- realClique(I), clique(I,P,V), unit(P,V,in).
realClique(I,P)   :- realClique(I,P,_).

relaClique(I,P,V,R,D) :- realClique(I,P,V), relate(P,V,R,D), cliqueRelations == 1.
relaClique(I,R,D)     :- relaClique(I,_,_,R,D).

joinClique(I,R,D)   :- relaClique(I,R,D),   2 { relaClique(I,X,Y,R,D) }.
joinClique(I,R,D,N) :- joinClique(I,R,D), N = { relaClique(I,X,Y,R,D) }.

dropClique(P,V,R,D) :- relaClique(I,P,V,R,D), joinClique(I,R,D).

equiClique(I,R1,D1,R2,D2) :- joinClique(I,R1,D1,N), joinClique(I,R2,D2,N), (R1,D1) < (R2,D2),
                             relaClique(I,P,V,R1,D1) : relaClique(I,P,V,R2,D2).

mapsClique(I,R1,D1,R1,D1) :- joinClique(I,R1,D1),
                             not equiClique(I,R,D,R1,D1) : equiClique(I,R,D,R1,D1).
mapsClique(I,R1,D1,R2,D2) :- equiClique(I,R1,D1,R2,D2), mapsClique(I,R1,D1,R1,D1).

%%%%%%%%%%%%%%%%%%%%%
% generate solution %
%%%%%%%%%%%%%%%%%%%%%

{ hold(uni(P,V)) } :- unit(P,V,in).
 :- realClique(I), 2 { hold(uni(P,V)) : realClique(I,P,V) }, cliqueShortcuts == 1.

hold(rel(I,R1,D1)) :- hold(uni(P,V)), relaClique(I,P,V,R1,D1), mapsClique(I,R1,D1,R1,D1).

active(R,D) :- hold(rel(I,R1,D1)), mapsClique(I,R1,D1,R,D), R != rec.
active(R,D) :- hold(uni(P,V)), relate(P,V,R,D), R != rec, not dropClique(P,V,R,D).
 :- active(con,D),     active(sat,D).
 :- active(dep,D), not active(sat,D).
 :-    request(D), not active(sat,D).

#show in(P,V) : hold(uni(P,V)).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% precalculate target sets and elements of objectives %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

installed(P) :- installed(P,_).

solution(P) :- unit(P,_,in).

criterion(T,C) :- criterion(_,T,C,_).

target(T) :- criterion(T,_).

  up(P,M) :- target(up),   installed(P), M = #max{ V : installed(P,V) }.
down(P,M) :- target(down), installed(P), M = #min{ V : installed(P,V) }.

target(sol,solution,P,V) :- unit(P,V,in), target(solution).
target(sol,changed ,P,V) :- unit(P,V,in), target(changed), not installed(P,V).
target(sol,new     ,P,V) :- unit(P,V,in), target(new),     not installed(P).
target(sol,up      ,P,V) :- unit(P,V,in),   up(P,M), M < V.
target(sol,down    ,P,V) :- unit(P,V,in), down(P,M), V < M.

target(sub,changed,P,V) :- installed(P,V), target(changed).
target(sub,removed,P,V) :- installed(P,V), target(removed).
target(sub,removed,P)   :- target(sub,removed,P,_), solution(P).

remClique(P)   :- target(sub,removed,P), realClique(I,P), realClique(I,P,V) : unit(P,V,in).
remClique(I,P) :- realClique(I,P), remClique(P).

delete(changed,P,V) :- target(sub,changed,P,V), not unit(P,V,in).
delete(removed,P,V) :- target(sub,removed,P,V), not solution(P).

attribute(S,T,P,V,A,X,B,Y)   :- target(S,T,P,V), attribute(P,V,A,X), attribute(P,V,B,Y),
                                criterion(T,aligned(A,B)).
attribute(I,S,T,P,V,A,X,B,Y) :- attribute(S,T,P,V,A,X,B,Y), realClique(I,P,V).
attribute(I,T,A,X,B,Y)       :- attribute(I,sol,T,_,_,A,X,B,Y).

oneval(T,A,X,B,Y) :- attribute(sub,T,_,_,A,X,B,Y).
oneval(T,A,X,B,Y) :- attribute(sol,T,P,V,A,X,B,Y),
                     { attribute(I,T,A,X,B,_) : realClique(I,P,V) } 1.
oneval(T,A,X,B,Y) :- attribute(_,T,A,X,B,Y), oneval(T,A,X,B).
oneval(T,A,X,B)   :- oneval(T,A,X,B,_).

twoval(T,A,X,B,I) :- attribute(I,T,A,X,B,_), not oneval(T,A,X,B).
twoval(T,A,X,B)   :- twoval(T,A,X,B,_).

keyval(T,A,X,B) :- oneval(T,A,X,B), 2 { oneval(T,A,X,B,Y) }.
keyval(T,A,X,B) :- twoval(T,A,X,B), 2 { twoval(T,A,X,B,Y) }.

remval(I,changed,P,V,A,X,B,Y) :- attribute(I,sub,changed,P,V,A,X,B,Y), keyval(changed,A,X,B).
remval(I,removed,P,0,A,X,B,Y) :- attribute(I,sub,removed,P,_,A,X,B,Y), keyval(removed,A,X,B),
                                 remClique(I,P).
remval(I,T,A,X,B,Y)           :- remval(I,T,_,_,A,X,B,Y).

static(T,aligned(A,X,B,Y)) :- remval(I,T,A,X,B,Y), 2 { remval(I,T,J,K,A,X,B,Y) }.
static(T,aligned(A,X,B,Y)) :- attribute(sub,T,P,V,A,X,B,Y), keyval(T,A,X,B), delete(T,P,V).

delrec(T,D) :- target(sub,T,P,V), delete(T,P,V), criterion(T,unsat_recommends),
               relate(P,V,rec,D).

static(T,unsat_recommends(D),M) :- delrec(T,D),
                                   M = #sum { W,P,V,D : recommends(P,V,D,W), delete(T,P,V) }, M != 0.

remrec(P,D) :- target(sub,removed,P), criterion(removed,unsat_recommends),
               installed(P,V), relate(P,V,rec,D).

remove(count              ,P,M) :- target(sub,removed,P), criterion(removed,count),
                                   M = { installed(P,X) }.
remove(notuptodate        ,P,M) :- target(sub,removed,P), criterion(removed,notuptodate),
                                   M = { not maxversion(P,V) : installed(P,V) }, 0 < M.
remove(sum(A)             ,P,M) :- target(sub,removed,P), criterion(removed,sum(A)),
                                   M = #sum { X,P,V,A : attribute(P,V,A,X), installed(P,V) }, M != 0.
remove(unsat_recommends(D),P,M) :- rerec(P,D),
                                   M = #sum { W,P,V,D : recommends(P,V,D,W), installed(P,V) }, M != 0.

remove(C,P)                :- remove(C,P,_).
remove(aligned(A,X,B,Y),P) :- attribute(sub,removed,P,_,A,X,B,Y),
                              keyval(removed,A,X,B), not static(removed,aligned(A,X,B,Y)).

object(count              ,S,T,P,V,1) :- target(S,T,P,V), T != removed, not delete(T,P,V),
                                         criterion(T,count).
object(notuptodate        ,S,T,P,V,1) :- target(S,T,P,V), T != removed, not delete(T,P,V),
                                         criterion(T,notuptodate), not maxversion(P,V).
object(sum(A)             ,S,T,P,V,X) :- target(S,T,P,V), T != removed, not delete(T,P,V),
                                         criterion(T,sum(A)), attribute(P,V,A,X), X != 0.
object(unsat_recommends(D),S,T,P,V,W) :- target(S,T,P,V), T != removed, not delete(T,P,V),
                                         criterion(T,unsat_recommends),
                                         recommends(P,V,D,W), W != 0.

object(C               ,S,T,P,V) :- object(C,S,T,P,V,_).
object(aligned(A,X,B,Y),S,T,P,V) :- attribute(S,T,P,V,A,X,B,Y), T != removed,
                                    keyval(T,A,X,B), not static(T,aligned(A,X,B,Y)).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% calculate constituents of objectives %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

special(unsat_recommends(D),sub,removed) :- remove(unsat_recommends(D),_).
special(unsat_recommends(D),sub,changed) :- object(unsat_recommends(D),sub,changed,_,_).

special(unsat_recommends(D)) :- special(unsat_recommends(D),_,_).
special(unsat_recommends(D)) :- object(unsat_recommends(D),sol,_,_,_).
special(unsat_recommends(D)) :- static(_,unsat_recommends(D),_).

conClique(I,C,P) :- remClique(I,P), remove(C,P).
conClique(I,C)   :- conClique(I,C,_).

subClique(I,C,sub,removed,P,V) :- unit(P,V,in), conClique(I,C,P).
subClique(I,C,S,T        ,P,V) :- object(C,S,T,P,V), realClique(I,P,V).
subClique(I,C,S,T)             :- subClique(I,C,S,T,_,_).

resClique(I,C,S,T)     :- subClique(I,C,S,T), 2 { subClique(I,C,S,T,X,Y) }.
resClique(I,C,S,T,P,V) :- subClique(I,C,S,T,P,V), resClique(I,C,S,T).
resClique(C,S,T,P,V)   :- resClique(_,C,S,T,P,V).

pkgRemove(C,P) :- remove(C,P), not remClique(P).
pkgRemove(C,P) :- conClique(I,C,P), not resClique(I,C,sub,removed).
pkgRemove(P)   :- pkgRemove(_,P).

subRemove(I,P)   :- pkgRemove(P), realClique(I,P), cliqueBreak == 1.
subRemove(I,P,N) :- subRemove(I,P), N = { realClique(I,P,X) }.

recClique(I,C,sub,removed,Z) :- resClique(I,C,sub,removed), special(C,sub,removed),
                                Z = #sum { M,C,P : remove(C,P,M), conClique(I,C,P) }.
recClique(I,C,sub,changed,Z) :- resClique(I,C,sub,changed), special(C,sub,changed),
                                Z = #sum { M,C,P,V : object(C,sub,changed,P,V,M),
                                  resClique(I,C,sub,changed,P,V) }.

valClique(I,C,sub,removed,P,0,M)   :- resClique(I,C,sub,removed), conClique(I,C,P),
                                      remove(C,P,M), not special(C,sub,removed).
valClique(I,C,sub,removed,P,0,Z-M) :- resClique(I,C,sub,removed), conClique(I,C,P),
                                      remove(C,P,M), recClique(I,C,sub,removed,Z), Z != M.
valClique(I,C,S,T        ,P,V,M)   :- resClique(I,C,S,T,P,V), object(C,S,T,P,V,M),
                                      not special(C,S,T).
valClique(I,C,S,T        ,P,V,Z-M) :- resClique(I,C,S,T,P,V), object(C,S,T,P,V,M),
                                      recClique(I,C,S,T,Z), Z != M.
valClique(I,C,S,T,P,V)             :- valClique(I,C,S,T,P,V,_).
valClique(I,C,S,T,M)               :- valClique(I,C,S,T,_,_,M).
valClique(I,C,S,T,Z)               :- recClique(I,C,S,T,Z), Z != 0.
valClique(I,C,S,T)                 :- valClique(I,C,S,T,_).

uniClique(I,C,S,T) :- valClique(I,C,S,T), { valClique(I,C,S,T,X) } 1.

ordClique(I,C,S,T,M2,M1) :- valClique(I,C,S,T,M1;M2), |M1| < |M2|, 0 < M1*M2,
                            |M| <= |M1| : valClique(I,C,S,T,M), |M| < |M2|, 0 < M*M2.
ordClique(I,C,S,T,M2,0)  :- valClique(I,C,S,T,M2), not uniClique(I,C,S,T),
                            M1*M2 <= 0 : valClique(I,C,S,T,M1), |M1| < |M2|.

outClique(I,C,S,T) :- valClique(I,C,S,T), not uniClique(I,C,S,T), not special(C,S,T).
outClique(I,C,S,T) :- recClique(I,C,S,T,0).

brkClique(i(I,m,f(C,S,T,M))        ,P,V) :- valClique(I,C,S,T,P,V,M), not uniClique(I,C,S,T),
                                            T != removed.
brkClique(i(I,m,f(C,sub,removed,M)),P,V) :- valClique(I,C,sub,removed,P,0,M), unit(P,V,in),
                                            not uniClique(I,C,sub,removed).
brkClique(i(I,n,f(C,S,T))          ,P,V) :- resClique(I,C,S,T,P,V), not outClique(I,C,S,T).

brkClique(J)   :- brkClique(J,_,_).
brkClique(J,N) :- brkClique(J), N = { brkClique(J,X,Y) }.

donClique(i(I,m,F),uni(P,V))   :- brkClique(i(I,m,F),P,V), brkClique(i(I,m,F),1).
donClique(i(I,L,F),rel(I,R,D)) :- brkClique(i(I,L,F),N),
                                  joinClique(I,R,D,N), mapsClique(I,R,D,R,D),
                                  brkClique(i(I,L,F),P,V) : relaClique(I,P,V,R,D).
donClique(J)                   :- donClique(J,_).

comClique(J,N) :- brkClique(J,N), not donClique(J).

simClique(i(I,L1,F1),i(I,L2,F2)) :- comClique(i(I,L1,F1);i(I,L2,F2),N), (L1,F1) < (L2,F2),
                                    brkClique(i(I,L1,F1),P,V) : brkClique(i(I,L2,F2),P,V).

mapClique(i(I,L1,F1),i(I,L1,F1)) :- comClique(i(I,L1,F1),_),
                       not simClique(i(I,L,F),i(I,L1,F1)) : simClique(i(I,L,F),i(I,L1,F1)).
mapClique(i(I,L1,F1),i(I,L2,F2)) :- simClique(i(I,L1,F1),i(I,L2,F2)),
                                    mapClique(i(I,L1,F1),i(I,L1,F1)).

pkgClique(i(I,m,F))        :- mapClique(i(I,m,F),i(I,m,F)), cliqueBreak == 1.
pkgClique(i(I,n,f(C,S,T))) :- mapClique(i(I,n,f(C,S,T)),i(I,n,f(C,S,T))),
                              uniClique(I,C,S,T) : valClique(I,C,S,T), cliqueBreak == 1.
pkgClique(J,P)             :- pkgClique(J), brkClique(J,P,_).

preClique(J,P)   :- pkgClique(J,P),   2 { brkClique(J,P,X) }.
preClique(J,P,N) :- preClique(J,P), N = { brkClique(J,P,X) }.

pkgRelate(I,P)     :- subRemove(I,P).
pkgRelate(I,P)     :- preClique(i(I,_,_),P).
pkgRelate(I)       :- pkgRelate(I,_).
pkgRelate(I,R,D,P) :- pkgRelate(I), relaClique(I,P,_,R,D), mapsClique(I,R,D,R,D).

pkgUnique(I,R,D)     :- pkgRelate(I), mapsClique(I,R,D,R,D), { pkgRelate(I,R,D,_) } 1.
pkgUnique(I,R,D,P,N) :- pkgRelate(I,P), pkgRelate(I,R,D,P), joinClique(I,R,D,N),
                        pkgUnique(I,R,D).

ponClique(J,P,uni(P,V))          :- brkClique(J,P,V), pkgClique(J,P), not preClique(J,P).
ponClique(i(I,L,F),P,rel(I,R,D)) :- preClique(i(I,L,F),P,N), pkgUnique(I,R,D,P,N),
                                    brkClique(i(I,L,F),P,V) : relaClique(I,P,V,R,D).
ponClique(J,P)                   :- ponClique(J,P,_).

comClique(J,P,N) :- preClique(J,P,N), not ponClique(J,P).

simClique(i(I,L1,F1),i(I,L2,F2),P) :- comClique(i(I,L1,F1);i(I,L2,F2),P,N), (L1,F1) < (L2,F2),
                                      brkClique(i(I,L1,F1),P,V) : brkClique(i(I,L2,F2),P,V).

mapClique(i(I,L1,F1),i(I,L1,F1),P) :- comClique(i(I,L1,F1),P,_),
                         not simClique(i(I,L,F),i(I,L1,F1),P) : simClique(i(I,L,F),i(I,L1,F1),P).
mapClique(i(I,L1,F1),i(I,L2,F2),P) :- simClique(i(I,L1,F1),i(I,L2,F2),P),
                                      mapClique(i(I,L1,F1),i(I,L1,F1),P).

hasClique(J              ,E)                 :- donClique(J,E).
hasClique(J              ,E)                 :- mapClique(E,J), E != J.
hasClique(J              ,uni(P,V))          :- mapClique(J,J), brkClique(J,P,V),
                                                cliqueBreak != 1.
hasClique(J              ,E)                 :- ponClique(J,_,E).
hasClique(J              ,pkg(E,P))          :- mapClique(E,J,P).
hasClique(i(I,n,f(C,S,T)),i(I,m,f(C,S,T,M))) :- valClique(I,C,S,T,M), not recClique(I,C,S,T,M),
                                                not uniClique(I,C,S,T), not outClique(I,C,S,T),
                                                not donClique(i(I,n,f(C,S,T))).
 :- mapClique(J,J), 2 { hold(E) : hasClique(J,E) }, cliqueShortcuts == 1.
 :- valClique(I,C,S,T), not uniClique(I,C,S,T), not outClique(I,C,S,T),
    2 { hold(E) : hasClique(i(I,n,f(C,S,T)),E) }, cliqueShortcuts == 1.

donRemove(I,P,uni(P,V))   :- subRemove(I,P,1), realClique(I,P,V).
donRemove(I,P,rel(I,R,D)) :- subRemove(I,P,N), pkgUnique(I,R,D,P,N).
donRemove(I,P,pkg(J,P))   :- subRemove(I,P,N), preClique(J,P,N), mapClique(J,J,P).
donRemove(I,P)            :- donRemove(I,P,_).

hasRemove(rem(P),E)        :- subRemove(I,P), donRemove(I,P,E).
hasRemove(rem(P),sub(I,P)) :- subRemove(I,P), not donRemove(I,P).
hasRemove(rem(P),uni(P,V)) :- pkgRemove(P), unit(P,V,in),
                              not realClique(I,P,V) : realClique(I,P,V), cliqueBreak == 1.

abbrev(pkg(J,P)) :- mapClique(J,J,P).
abbrev(sub(I,P)) :- hasRemove(_,sub(I,P)).

%%%%%%%%%%%%%%%%%%%%%%%%%%%
% derive objective values %
%%%%%%%%%%%%%%%%%%%%%%%%%%%

hold(sub(I,P)) :- hold(uni(P,V)), abbrev(sub(I,P)), realClique(I,P,V).
hold(pkg(J,P)) :- hold(uni(P,V)), abbrev(pkg(J,P)), brkClique(J,P,V).

hold(J) :- hasClique(J,E), hold(E).
hold(J) :- hasRemove(J,E), hold(E).

penalty(rem(P),C,sub,removed,M) :- remove(C,P,M), pkgRemove(C,P), hold(rem(P)),
                                   not special(C,sub,removed).
special(rem(P),C,sub,removed,M) :- remove(C,P,M), pkgRemove(C,P), not hold(rem(P)),
                                   special(C,sub,removed).

penalty(uni(P,V),C,S,T,M) :- object(C,S,T,P,V,M), not resClique(C,S,T,P,V), hold(uni(P,V)),
                             not special(C).
special(uni(P,V),C,S,T,M) :- object(C,S,T,P,V,M), special(C), not resClique(C,S,T,P,V),
                             hold(uni(P,V)) : not special(C,S,T);
                             not hold(uni(P,V)) : special(C,S,T).

penalty(i(I),C,S,T,M) :- valClique(I,C,S,T,M), uniClique(I,C,S,T),
                         hold(i(I,n,f(C,S,T))), not special(C).
special(i(I),C,S,T,M) :- valClique(I,C,S,T,M), special(C), uniClique(I,C,S,T),
                         hold(i(I,n,f(C,S,T))) : not special(C,S,T);
                         not hold(i(I,n,f(C,S,T))) : special(C,S,T).

penalty(i(I,M1),C,S,T,M2-M1) :- ordClique(I,C,S,T,M2,M1), hold(i(I,m,f(C,S,T,M2))),
                                not special(C).
penalty(i(I,M1),C,S,T,M2-M1) :- ordClique(I,C,S,T,M2,M1), penalty(i(I,M2),C,S,T,_).

special(i(I,M1),C,S,T,M2-M1) :- ordClique(I,C,S,T,M2,M1), not hold(i(I,n,f(C,S,T))),
                                recClique(I,C,S,T,M2).
special(i(I,M1),C,S,T,M2-M1) :- ordClique(I,C,S,T,M2,M1), hold(i(I,m,f(C,S,T,M2))),
                                special(C).
special(i(I,M1),C,S,T,M2-M1) :- ordClique(I,C,S,T,M2,M1), special(i(I,M2),C,S,T,_).

penalty(s,unsat_recommends(D),sub,T,M) :- static(T,unsat_recommends(D),M),
                                          not active(sat,D).
penalty(E,unsat_recommends(D),S  ,T,M) :- special(E,unsat_recommends(D),S,T,M),
                                          not active(sat,D).

aligned(removed,A,X,B,Y) :- remove(aligned(A,X,B,Y),P), pkgRemove(aligned(A,X,B,Y),P),
                            not hold(rem(P)).
aligned(T      ,A,X,B,Y) :- object(aligned(A,X,B,Y),sol,T,P,V), hold(uni(P,V)),
                            not resClique(aligned(A,X,B,Y),sol,T,P,V).
aligned(T      ,A,X,B,Y) :- object(aligned(A,X,B,Y),sub,T,P,V), not hold(uni(P,V)),
                            not resClique(aligned(A,X,B,Y),sub,T,P,V).
aligned(T      ,A,X,B,Y) :- resClique(I,aligned(A,X,B,Y),sol,T),
                            hold(i(I,n,f(aligned(A,X,B,Y),sol,T))).
aligned(T      ,A,X,B,Y) :- resClique(I,aligned(A,X,B,Y),sub,T),
                            not hold(i(I,n,f(aligned(A,X,B,Y),sub,T))).

alvalue(removed,A,X,B,Y) :- remove(aligned(A,X,B,Y),_).
alvalue(T,      A,X,B,Y) :- object(aligned(A,X,B,Y),_,T,_,_).
alvalue(T,A,X,B)         :- alvalue(T,A,X,B,_).

statkey(T,A,X,B) :- alvalue(T,A,X,B), static(T,aligned(A,X,B,_)).

alcount(T,A,X,B) :- alvalue(T,A,X,B), { alvalue(T,A,X,B,_) } alignCounter.

aligned(T,A,X,B) :- aligned(T,A,X,B,_), not alcount(T,A,X,B), not statkey(T,A,X,B).

alchain(T,A,X,B,Y1,Y2) :- alcount(T,A,X,B), alvalue(T,A,X,B,Y1;Y2), Y1 < Y2,
                          Y <= Y1 : alvalue(T,A,X,B,Y), Y < Y2.
alchain(T,A,X,B,Y2)    :- alcount(T,A,X,B), alvalue(T,A,X,B,Y2),
                          not alchain(T,A,X,B,Y2,Y) : alchain(T,A,X,B,Y2,Y).

alcount(T,A,X,B,Y1,0)   :- alchain(T,A,X,B,Y1,_),  aligned(T,A,X,B,Y1), not statkey(T,A,X,B).
alcount(T,A,X,B,Y1,1)   :- alcount(T,A,X,B),       aligned(T,A,X,B,Y1), statkey(T,A,X,B).
alcount(T,A,X,B,Y2,Z)   :- alchain(T,A,X,B,Y1,Y2), alcount(T,A,X,B,Y1,Z),
                           Z != 0 : alchain(T,A,X,B,Y2).
alcount(T,A,X,B,Y2,Z+1) :- alchain(T,A,X,B,Y1,Y2), alcount(T,A,X,B,Y1,Z),
                           aligned(T,A,X,B,Y2).

mult(minimize,1). mult(maximize,-1).
turn(sol     ,1). turn(sub     ,-1).

#minimize{ M*F*G@L,X,C,S,T     : penalty(X,C,S,T,M),   criterion(O,T,C,L),                mult(O,F), turn(S,G), not special(C);
           M*F  @L,X,C,Y,T     : penalty(X,C,Y,T,M),   criterion(O,T,unsat_recommends,L), mult(O,F),                special(C);
            -F  @L,T,A,X,B     : aligned(T,A,X,B),     criterion(O,T,aligned(A,B),L),     mult(O,F);
             F  @L,T,A,X,B,Y   : aligned(T,A,X,B,Y),   criterion(O,T,aligned(A,B),L),     mult(O,F), not alcount(T,A,X,B);
             F  @L,T,A,X,B,Y,Z : alcount(T,A,X,B,Y,Z), criterion(O,T,aligned(A,B),L),     mult(O,F),     alchain(T,A,X,B,Y);
             0  @L             :                       criterion(_,_,_,L) }.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% trace derivations of objectives' elements (for debugging only) %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%*
trace(uni(P,V))   :- hasRemove(_,uni(P,V)), trace == 1.
trace(rel(I,R,D)) :- donRemove(_,_,rel(I,R,D)), trace == 1.
trace(E)          :- donClique(_,E), trace == 1.
trace(E)          :- ponClique(_,_,E), trace == 1.

trace(uni(P,V)  ,P,V) :- trace(uni(P,V)).
trace(rel(I,R,D),P,V) :- trace(rel(I,R,D)), relaClique(I,P,V,R,D).
trace(sub(I,P)  ,P,V) :- realClique(I,P,V), abbrev(sub(I,P)), trace == 1.
trace(pkg(J,P)  ,P,V) :- brkClique(J,P,V), abbrev(pkg(J,P)), trace == 1.

trace(J,P,V) :- hasClique(J,E), trace(E,P,V).
trace(P,Q,V) :- hasRemove(P,E), trace(E,Q,V).

malClique(I,C,S,T      ,P,V)   :- resClique(I,C,S,T,P,V), not outClique(I,C,S,T),
                                  not trace(i(I,n,f(C,S,T)),P,V), trace == 1.
malClique(I,C,S,T      ,P,V,M) :- valClique(I,C,S,T,P,V,M), not uniClique(I,C,S,T), T != removed,
                                  not trace(i(I,m,f(C,S,T,M)),P,V), trace == 1.
malClique(I,C,S,removed,P,V,M) :- valClique(I,C,S,removed,P,0,M), unit(P,V,in),
                                  not uniClique(I,C,S,removed), trace == 1,
                                  not trace(i(I,m,f(C,S,removed,M)),P,V).

nalClique(I,C,S,T,P,V)   :- trace(i(I,n,f(C,S,T)),P,V), not resClique(I,C,S,T,P,V).
nalClique(I,C,S,T,P,V)   :- trace(i(I,n,f(C,S,T)),P,V), outClique(I,C,S,T).
nalClique(I,C,S,T,P,V,M) :- trace(i(I,m,f(C,S,T,M)),P,V),
                            not valClique(I,C,S,T,P,V,M) : T != removed,
                            not valClique(I,C,S,T,P,0,M) : T == removed.
nalClique(I,C,S,T,P,V,M) :- trace(i(I,m,f(C,S,T,M)),P,V), uniClique(I,C,S,T).

malRemove(P,P,V) :- pkgRemove(P), unit(P,V,in), not trace(rem(P),P,V), trace == 1.
nalRemove(P,Q,V) :- trace(rem(P),Q,V), P != Q.
nalRemove(P,P,V) :- trace(rem(P),P,V), not pkgRemove(P).
nalRemove(P,P,V) :- trace(rem(P),P,V), not unit(P,V,in).

#show malClique/6.
#show malClique/7.
#show nalClique/6.
#show nalClique/7.
#show malRemove/3.
#show nalRemove/3.
*%
