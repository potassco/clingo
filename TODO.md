# clingo 5
- address `_` in negated literals
  - use `_` or maybe `*`
- add sort-constraint
  - `order(B,A) :- (A, B) = #sort{ X : p(X) }.`
  - `order(A,B) :- ((_,A), (_,B)) = #sort{ K,X : p(X), key(X,K) }.`
- profiling
- sorting via conditional literals became less efficient with the latest implementation in some cases
- projection is disabled in non-monotone constructs for now
  it could be enabled again if equivalences are used for affected atoms
- remove CSP support
- shifting of disjunctions

# constraints
- integrate constraint variables tighter into the gringo language
  - any term can contain csp variables
  - use [HT\_LC](http://www.cs.uni-potsdam.de/wv/pdfformat/cakaossc16a.pdf) as basis
- csp terms could be supported in aggregates too
- when the value of a csp term is required rewriting has to happen
  - `p(f($X+3))` becomes `p(f(Aux)), Aux = $X+3`
- supporting csp terms in functions symbols in build-ins is more tricky
  - `f($X+3) < g($Y+1)`
  - a translation as above is unsafe
  - the literal is a tautology

# memory management
- I think that the proper way to handle symbols is to use reference counting
  - the c api would have to put the burden of reference counting on the user
  - the APIs can provide reference counted symbols
- symbols should only be owned in key locations where the reference count is
  increased
  - for example a domain owns a symbol and can also take care of flyweighting
    it
- touching the counters should be avoided as much as possible during grounding
  - during the backtraking based instantiation a very limited set of
    intermediate symbols is created
  - these intermediate symbols can be freed upon backtracking (if they have not
    been added to a domain)
  - the temporary symbols may refer to owned symbols without increasing their
    reference count
  - this has the advantage that insertion into hash tables and touching of
    reference counts only happens when a symbol is commited into a domain
- implementing this should not be terribly difficult but affects *a lot* of
  code

# misc
- **enlarge test suites**
- incremental programs
  - atm indexes have to be cleared and recreated afterwards
    - it might be a good idea to optimize this and reuse indices later on
    - for now just clear them to not have them dangling around
- missing features in view of the ASP standard
  - queries
- assignment rewriting
  - enqueue: `expr(X,Z,Val):-expr(X,Y,Val_1)?,sing_term(Y,Z,Val_2)?,Val=(Val_1+Val_2),#X0=(Val_1+Val_2),#X0=Val.`
  - handle assignments in a more clever way...
- it would be nice to block grounding of rules if one index is empty
  (maybe even delaying the filling of indices if one index is empty)
- indices could be specialized to handle zero-ary predicates more efficiently
  - there could be one domain for all zero-ary predicates
- detect ground rules and implement more clever groundig
- domains
  - using a value as representation is wasteful
  - uses one unordered\_map to much
- predicate indices
  - using a valvec as key is wasteful
  - uses one unordered\_map too much
- on large instances both optimizations should safe a lot of memory
