% P1
%
block(b0).
block(b1).
block(b2).
block(b3).
%
% initial state:
%
%      3
%  0 1 2
% -------
%
init(on(b0,table)).
init(on(b1,table)).
init(on(b2,table)).
init(on(b3,b2)).
%
% goal state:
%
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
