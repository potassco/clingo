%% { "options": ["0"]
%% , "solutions":[ ["0", "2", "4", "5", "9"]
%%               , ["0", "2", "5", "9"]
%%               , ["0", "4", "9"]
%%               , ["0", "9"]] }
holds(atom(A)) :- rule(lit(pos,atom(A)),B); holds(B).
{ holds(atom(A)) : head_aggregate_element_set(I, head_aggregate_element(*,lit(pos,atom(A)),C))
                 , holds(C) }
:- rule(head_aggregate(left(#inf,less_equal),count,head_aggregate_element_set(I),right(less_equal,#sup)), C);
   holds(C).

holds(conjunction(I)) :- conjunction(I);
                                 holds(A) : conjunction(I,lit(pos,A));
                         not     holds(A) : conjunction(I,lit(neg,A));
                         not not holds(A) : conjunction(I,lit(neg_neg,A)).

body_aggregate(L,F,S,U) :- conjunction(*,lit(*,body_aggregate(L,F,S,U))).

holds(body_aggregate(left(#inf,less_equal),sump,body_aggregate_element_set(S),right(greater_equal,U)))

:- body_aggregate(left(#inf,less_equal),sump,body_aggregate_element_set(S),right(greater_equal,U));
   #sum+ { W,T : body_aggregate_element_set(S,body_aggregate_element((W,T),conjunction(C))),
                 holds(C) } >= U.

#show.
#show I : holds(conjunction(I)), conjunction(I).

rule(head_aggregate(left(#inf,less_equal),count,head_aggregate_element_set(0),right(less_equal,#sup)),
conjunction(0)).
conjunction(0).
head_aggregate_element_set(0).
head_aggregate_element_set(0,head_aggregate_element((),lit(pos,atom(p(1))),conjunction(0))).
head_aggregate_element_set(0,head_aggregate_element((),lit(pos,atom(p(2))),conjunction(0))).
rule(lit(pos,atom(__aux(1))),conjunction(1)).
conjunction(1).
conjunction(1,lit(pos,atom(p(2)))).
conjunction(1,lit(neg,atom(p(2)))).
rule(lit(pos,atom(__aux(2))),conjunction(2)).
conjunction(2).
conjunction(2,lit(pos,atom(p(2)))).
rule(lit(pos,atom(__aux(2))),conjunction(3)).
conjunction(3).
conjunction(3,lit(pos,atom(__aux(1)))).
rule(lit(pos,atom(__aux(4))),conjunction(6)).
conjunction(4).
conjunction(4,lit(pos,atom(p(1)))).
conjunction(5).
conjunction(5,lit(pos,atom(__aux(2)))).
body_aggregate_element_set(0).
body_aggregate_element_set(0,body_aggregate_element((1,(0,())),conjunction(4))).
body_aggregate_element_set(0,body_aggregate_element((2,(1,())),conjunction(5))).
conjunction(6).
conjunction(6,lit(pos,body_aggregate(left(#inf,less_equal),sump,body_aggregate_element_set(0),right(greater_equal,1)))).
rule(lit(pos,atom(__aux(5))),conjunction(7)).
conjunction(7).
conjunction(7,lit(pos,body_aggregate(left(#inf,less_equal),sump,body_aggregate_element_set(0),right(greater_equal,3)))).
rule(lit(pos,atom(__aux(3))),conjunction(8)).
conjunction(8).
conjunction(8,lit(pos,atom(__aux(4)))).
conjunction(8,lit(neg,atom(__aux(5)))).
rule(lit(pos,false),conjunction(9)).
conjunction(9).
conjunction(9,lit(neg,atom(__aux(3)))).
show_atom(p(1)).
show_atom(p(2)).
