#hide.
#show holds(atom(_)).

% extract body elements
body(B) :- rule(_,pos(B)).
body(B) :- body(conjunction(S)), set(S,pos(B)).
body(B) :- body(conjunction(S)), set(S,neg(B)).

% define bodies
holds(conjunction(S)) :-     holds(A) : set(S,pos(A)),
                         not holds(A) : set(S,neg(A)), body(conjunction(S)).
holds(sum(L,WL,U))  :- L #sum  [     holds(A) = W : wlist(WL,_,pos(A),W),
                                 not holds(A) = W : wlist(WL,_,neg(A),W) ] U, body(sum(L,WL,U)).
holds(avg(L,WL,U))  :- L #avg  [     holds(A) = W : wlist(WL,_,pos(A),W),
                                 not holds(A) = W : wlist(WL,_,neg(A),W) ] U, body(avg(L,WL,U)).
holds(min(L,WL,U))  :- L #min  [     holds(A) = W : wlist(WL,_,pos(A),W),
                                 not holds(A) = W : wlist(WL,_,neg(A),W) ] U, body(min(L,WL,U)).
holds(max(L,WL,U))  :- L #max  [     holds(A) = W : wlist(WL,_,pos(A),W),
                                 not holds(A) = W : wlist(WL,_,neg(A),W) ] U, body(max(L,WL,U)).
holds(even(L,WL,U)) :-   #even [     holds(A) = W : wlist(WL,_,pos(A),W),
                                 not holds(A) = W : wlist(WL,_,neg(A),W) ]  , body(even(L,WL,U)).
holds(odd(L,WL,U))  :-   #odd  [     holds(A) = W : wlist(WL,_,pos(A),W),
                                 not holds(A) = W : wlist(WL,_,neg(A),W) ]  , body(odd(L,WL,U)).

% derive heads
head(A) :- rule(pos(A),pos(B)), holds(B).
holds(false)   :- head(false).
holds(atom(A)) :- head(atom(A)).
holds(A) : set(S,pos(A)) :- head(disjunction(S)).
L #sum  [     holds(A) = W : wlist(WL,_,pos(A),W),
          not holds(A) = W : wlist(WL,_,neg(A),W) ] U :- head(sum(L,WL,U)).
L #avg  [     holds(A) = W : wlist(WL,_,pos(A),W),
          not holds(A) = W : wlist(WL,_,neg(A),W) ] U :- head(avg(L,WL,U)).
L #min  [     holds(A) = W : wlist(WL,_,pos(A),W),
          not holds(A) = W : wlist(WL,_,neg(A),W) ] U :- head(min(L,WL,U)).
L #max  [     holds(A) = W : wlist(WL,_,pos(A),W),
          not holds(A) = W : wlist(WL,_,neg(A),W) ] U :- head(max(L,WL,U)).
  #even [     holds(A) = W : wlist(WL,_,pos(A),W),
          not holds(A) = W : wlist(WL,_,neg(A),W) ]   :- head(even(L,WL,U)).
  #odd  [     holds(A) = W : wlist(WL,_,pos(A),W),
          not holds(A) = W : wlist(WL,_,neg(A),W) ]   :- head(odd(L,WL,U)).

#minimize [     holds(A) = W @ P : wlist(WL,_,pos(A),W) : minimize(P,WL),
            not holds(A) = W @ P : wlist(WL,_,neg(A),W) : minimize(P,WL) ].

 :- holds(false).

