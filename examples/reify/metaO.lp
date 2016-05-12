%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% This file is part of gringo.                                             %
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

cxopt(card).  cxopt(incl).  cxopt(pref).

criteria(J,W,Q,A) :- show(A,_criteria(J,W,Q)).
criteria(J,W,A) :- criteria(J,W,Q,A).

cxopt(J,W,O)    :- criteria(J,W,_,_), cxopt(O), optimize(J,W,O).
cxopt(J,W,card) :- criteria(J,W,_,_), #false : optimize(J,W,O).

%%%%%%%%%% verify dominance

equal(J) :- cxopt(J,_,_), equal(J,W,O) : cxopt(J,W,O).

chain(J1,J2) :- cxopt(J1,_,_), cxopt(J2,_,_), J2 < J1,
                #false : cxopt(J3,W,O), J2 < J3, J3 < J1.

check(J2) :- cxopt(J2,_,_), #false : chain(J1,J2).
check(J2) :- chain(J1,J2), check(J1), equal(J1).

bot :- #false : cxopt(J,W,O).
bot :- check(J1), worse(J1).
bot :- check(J1), equal(J1), #false : chain(J1,J2).

% require non-existence of dominating answer set

:- not bot.

%%%%%%%%%% check cardinality criteria

equal(J,W,card) :- cxopt(J,W,card), #sum { -1,Q : not not hold(A),       criteria(J,W,Q,A);
                                            1,Q :         true(atom(A)), criteria(J,W,Q,A) } >= 0.
worse(J)        :- cxopt(J,W,card), #sum { -1,Q : not not hold(A),       criteria(J,W,Q,A);
                                            1,Q :         true(atom(A)), criteria(J,W,Q,A) } >= 1.
%%%%%%%%%% check inclusion criteria

ndiff(A) :- cxopt(J,W,incl), criteria(J,W,A), true(atom(A)).
ndiff(A) :- cxopt(J,W,incl), criteria(J,W,A), not hold(A).

equal(J,W,incl) :- cxopt(J,W,incl), ndiff(A) : criteria(J,W,A).
worse(J)        :- cxopt(J,W,incl), criteria(J,W,A), true(atom(A)), not hold(A).

%%%%%%%%%% check preference criteria (relative to user predicate prefer/2)

cando(A) :- cxopt(J,W,pref), criteria(J,W,A), fail(A),     hold(A).
nocan(A) :- cxopt(J,W,pref), criteria(J,W,A), true(A).
nocan(A) :- cxopt(J,W,pref), criteria(J,W,A), not hold(A).

condo(A) :- cxopt(J,W,pref), criteria(J,W,A), true(A), not hold(A).
nocon(A) :- cxopt(J,W,pref), criteria(J,W,A), fail(A).
nocon(A) :- cxopt(J,W,pref), criteria(J,W,A), hold(A).

cando(J,W,A)    :- cxopt(J,W,pref), criteria(J,W,A), criteria(J,W,A'), prefer(A,A'), A != A', cando(A), condo(A').

nocon(J,W,A)    :- cxopt(J,W,pref), criteria(J,W,A), nocon(A).
nocon(J,W,A)    :- cxopt(J,W,pref), criteria(J,W,A), nocan(A') : criteria(J,W,A'), A != A', prefer(A,A').
nocon(J,W,A)    :- cxopt(J,W,pref), criteria(J,W,A), criteria(J,W,A'), prefer(A',A), not prefer(A,A'), cando(A').

equal(J,W,pref) :- cxopt(J,W,pref), criteria(J,W,A), cando(J,W,A), nocon(A') : criteria(J,W,A'), prefer(A',A), not prefer(A,A').

worse(J)        :- cxopt(J,W,pref), nocon(J,W,A) : criteria(J,W,A).

