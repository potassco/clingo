#const n=5.

1 { pat(P,I) : seq(_,_,I) } 1 :- P = 0..(n-1).

occ(U,0,P) :- pat(0,I), seq(U,P,I).
occ(U,R+1,P) :- occ(U,R,P-1), seq(U,P,I), pat(R+1,I).
occ(U,R,P+1) :- occ(U,R,P), seq(U,P+1,_).

sup(U) :- occ(U,n-1,P), not seq(U,P+1,_).

#maximize { 1,U : sup(U) }.

#show pat/2.
