% P4
%
block(b0).
block(b1).
block(b2).
block(b3).
block(b4).
block(b5).
block(b6).
block(b7).
block(b8).
block(b9).
block(b10).
%
%initial state:
%
%   10 8
%  2 9 7
%  1 4 6
%  0 3 5
% -------
%
init(on(b0,table)).
init(on(b3,table)).
init(on(b5,table)).
init(on(b1,b0)).
init(on(b2,b1)).
init(on(b4,b3)).
init(on(b9,b4)).
init(on(b10,b9)).
init(on(b6,b5)).
init(on(b7,b6)).
init(on(b8,b7)).
%
% goal state:
%
%      1
%      2
%  7 0 10
%  8 4 6
%  3 9 5
% -------
%
goal(on(b9,table)).
goal(on(b3,table)).
goal(on(b5,table)).
goal(on(b4,b9)).
goal(on(b0,b4)).
goal(on(b8,b3)).
goal(on(b7,b8)).
goal(on(b6,b5)).
goal(on(b10,b6)).
goal(on(b2,b10)).
goal(on(b1,b2)).
