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

lits(B) :- normal(A,B).
lits(B) :- choice(A,B).
%lits(B) :- disjunction(A,B).

body(B) :- lits(B),
        hold(L) : lits(B, L), L > 0;
    not hold(L) : lits(B,-L), L > 0.

hold(A) :- normal(A,B), body(B).
hold(A) :- weight(A,G,B),
    #sum  { W,L :     hold(L), wlits(B, L,W), L > 0;
            W,L : not hold(L), wlits(B,-L,W), L > 0 } >= G.
% hold(A) : atoms(H,A)   :- disjunction(H,B), body(B).
{ hold(A) : atoms(H,A) } :- choice(H,B), body(B).

 :- compute( L), L > 0, not hold(L).
 :- compute(-L), L > 0,     hold(L).

hide(A,_criteria(J,W,Q)) :- show(A,_criteria(J,W,Q)).
hide(A,_query)           :- show(A,_query).
hide(A,_optimize(J,W,Q)) :- show(A,_optimize(J,W,Q)).

optimize(J,W,Q) :- hold(A), show(A,_optimize(J,W,Q)).

 :- not hold(A), show(A,_query).

#show.
#show T : show(A,T), hold(A), not hide(A,T).
