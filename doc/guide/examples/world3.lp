% P3
%
block(b0).
block(b1).
block(b2).
block(b3).
block(b4).
block(b5).
block(b6).
block(b7).
%
%initial state:
%
%  2   7
%  3 4 6
%  0 1 5
% -------
%
init(on(b0,table)).
init(on(b1,table)).
init(on(b5,table)).
init(on(b3,b0)).
init(on(b4,b1)).
init(on(b2,b3)).
init(on(b6,b5)).
init(on(b7,b6)).
%
% goal state:
%
%  7 5
%  3 0 2
%  4 1 6
% -------
%
goal(on(b4,table)).
goal(on(b6,table)).
goal(on(b1,table)).
goal(on(b3,b4)).
goal(on(b7,b3)).
goal(on(b2,b6)).
goal(on(b0,b1)).
goal(on(b5,b0)).
