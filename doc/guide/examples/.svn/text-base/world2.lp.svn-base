% P2
%
block(b0).
block(b1).
block(b2).
block(b3).
block(b4).
%
% initial state:
%
%    0
%  1 3
%  4 2
% -------
%
init(on(b4,table)).
init(on(b2,table)).
init(on(b3,b2)).
init(on(b0,b3)).
init(on(b1,b4)).
%
% goal state:
%
%  4
%  3
%  2
%  1
%  0
% -------
%
goal(on(b0,table)).
goal(on(b1,b0)).
goal(on(b2,b1)).
goal(on(b3,b2)).
goal(on(b4,b3)).
