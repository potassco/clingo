#include "csp.lp".

%%% This example uses unrestricted variables, which will give them the domain (-2^30 + 2 .. 2^30 -2)
%%% which is -1073741822 .. 1073741822
%%% The intermediate results can be in the range of 64bit integers and do silently overflow.

&sum {1440*a; 6*b; -3*c} = 136164.  % works fine

%&sum {1000*a; -42100*b; 123456*c} = 1234560. % does not work, getting stuck in the propagation

vars(a;b;c).
&show {X : vars(X)}.
