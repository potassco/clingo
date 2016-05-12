%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% This file is part of gringo.                                            %
% Copyright (C) 2015  Martin Gebser                                       %
% Copyright (C) 2015  Roland Kaminski                                     %
% Copyright (C) 2015  Torsten Schaub                                      %
%                                                                         %
% This program is free software: you can redistribute it and/or modify    %
% it under the terms of the GNU General Public License as published by    %
% the Free Software Foundation, either version 3 of the License, or       %
% (at your option) any later version.                                     %
%                                                                         %
% This program is distributed in the hope that it will be useful,         %
% but WITHOUT ANY WARRANTY; without even the implied warranty of          %
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           %
% GNU General Public License for more details.                            %
%                                                                         %
% You should have received a copy of the GNU General Public License       %
% along with this program.  If not, see <http://www.gnu.org/licenses/>.   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

sum(B,T) :- weight(_,_,B), T = #sum { W,L : wlits(B,L,W) }.

% extract supports of atoms and facts

supp(A,lits(B))    :- normal(A,B).
supp(A,wlits(G,B)) :- weight(A,G,B).
supp(A,lits(B))    :- choice(H,B), atoms(H,A).

supp(A) :- supp(A,_).

atom(A)   :- supp(A).
atom(|L|) :- lits(_,L).
atom(|L|) :- wlits(_,L,_).
atom(|L|) :- compute(L).

fact(A) :- normal(A,B), not lits(B,_). 

% generate interpretation

true(atom(A))                 :- fact(A).
true(atom(A)); fail(atom( A)) :- supp(A), not fact(A).
               fail(atom( A)) :- atom(A), not supp(A).
               fail(atom(-L)) :- compute(L), L < 0.

true(lits(B)) :- lits(B), 
    true(atom( L)) : lits(B,L), L > 0; 
    fail(atom(-L)) : lits(B,L), L < 0.
fail(lits(B)) :- lits(B), lits(B,L), fail(atom( L)), L > 0.
fail(lits(B)) :- lits(B), lits(B,L), true(atom(-L)), L < 0.

true(wlits(G,B)) :- weight(_,G,B),
    #sum { W,L : true(atom( L)), wlits(B,L,W), L > 0;
           W,L : fail(atom(-L)), wlits(B,L,W), L < 0 } >= G.
fail(wlits(G,B)) :- weight(_,G,B), sum(B,T),
    #sum { W,L : fail(atom( L)), wlits(B,L,W), L > 0;
           W,L : true(atom(-L)), wlits(B,L,W), L < 0 } >= T-G+1.

% verify supported model properties

bot :- normal(A,B),   true(lits(B)),    fail(atom(A)).
bot :- weight(A,G,B), true(wlits(G,B)), fail(atom(A)).
bot :- true(atom(A)), fail(B) : supp(A,B).
bot :- compute(L), L > 0, fail(atom( L)).

% verify acyclic derivability

internal(C,lits(B))    :- scc(C,A), supp(A,lits(B)),    scc(C,A'), lits(B,A').
internal(C,wlits(G,B)) :- scc(C,A), supp(A,wlits(G,B)), scc(C,A'), wlits(B,A',W).

external(C,lits(B))    :- scc(C,A), supp(A,lits(B)),    not internal(C,lits(B)).
external(C,wlits(G,B)) :- scc(C,A), supp(A,wlits(G,B)), not internal(C,wlits(G,B)).

steps(C,Z) :- scc(C,_), Z = { scc(C,A) : not fact(A) }.

wait(C,atom(A),0)    :- scc(C,A), fail(B) : external(C,B).
wait(C,lits(B),I)    :- internal(C,lits(B)), lits(B,A), wait(C,atom(A),I), steps(C,Z), I < Z.
wait(C,wlits(G,B),I) :- internal(C,wlits(G,B)), steps(C,Z), I = 0..Z-1, sum(B,T),
    #sum { W,L :     fail(atom(L)), wlits(B,L,W), L > 0, not scc(C,L);
           W,L : wait(C,atom(L),I), wlits(B,L,W), L > 0,     scc(C,L);
           W,L :    true(atom(-L)), wlits(B,L,W), L < 0 } >= T-G+1.
wait(C,atom(A),I)    :- wait(C,atom(A),0), steps(C,Z), I = 1..Z, wait(C,B,I-1) : supp(A,B), internal(C,B).

bot :- scc(C,A), true(atom(A)), wait(C,atom(A),Z), steps(C,Z).

% saturate interpretations that are not answer sets

true(atom(A)) :- supp(A), not fact(A), bot.
fail(atom(A)) :- supp(A), not fact(A), bot.

