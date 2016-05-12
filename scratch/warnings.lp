% both aggregates below are equivalent
% in both cases a warning should be issued
a :- #sum { -1:a; 1:a } >= 0.
b :- #sum { 1:b; 2:b } != 1.
