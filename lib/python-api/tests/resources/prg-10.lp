#const n = 18.

% initial prime
prime(1,2).

% mark prime and all its multiples as sieved
sieve(I,X) :- prime(I,X).
sieve(I,Z) :- prime(I,X), sieve(I,Y), Z=X+Y<=n.

% mark numbers not sieved by prime
% (ignoring numbers smaller than the prime)
nsieve(I,Z) :- prime(I,X), sieve(I,Y), Z=Y+(1..X-1)<=n.

% mark all primes sieved so far
% (ignoring numbers smaller than the prime)
rec_sieve(I,X) :- sieve(I,X).
rec_sieve(I,Y) :- prime(I,X), rec_sieve(I-1,Y), Y>=X.

% mark all primes not sieved so far
rec_nsieve(1,X) :- nsieve(1,X).
rec_nsieve(I,X) :- nsieve(I,X), rec_nsieve(I-1,X).

% mark the consecutive prefix that has been sieved
sieved_prefix(I,X) :- prime(I,X).
sieved_prefix(I,X) :- sieved_prefix(I,X-1), rec_sieve(I,X).

% mark the first non sieved number as prime
prime(I+1,X) :- sieved_prefix(I,X-1), rec_nsieve(I,X).

#show.
#show prime(X) : prime(_,X).

%% { "options": ["0"]
%% , "solutions": [["prime(2)", "prime(3)", "prime(5)", "prime(7)", "prime(11)", "prime(13)", "prime(17)"]] }
