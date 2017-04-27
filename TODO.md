# clingo 5
- maybe additional functions/functionality to work with theory atoms
  - functions to output aspif rules
  - reify theory atoms (but not the rest of the program)
- `--lparse-rewrite` should produce gringo 4 aligned output
- address `_` in negated literals
  - use `_` or maybe `*`
- add xor-constraints
- add sort-constraint
  - `order(B,A) :- (A, B) = #sort{ X : p(X) }.`
  - `order(A,B) :- ((_,A), (_,B)) = #sort{ K,X : p(X), key(X,K) }.`
- profiling
- sorting via conditional literals became less efficient with the latest implementation in some cases
- projection is disabled in non-monotone constructs for now
  it could be enabled again if equivalences are used for affected atoms
- csp-rewrite + output format
- shifting of disjunctions
- language/API extensions in view of preferences
  - turn terms into atoms
    - `x :- #assert(Atom, Sign), (Atom, Sign) = @luaCall().`
       or `x :- Atom, Atom = @luaCall().`
- simplify one elementary head aggregates as in gringo-3

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

