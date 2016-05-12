#script(lua)
Fun = gringo.Fun
Sup = gringo.Sup
Inf = gringo.Inf
function lua_gcd(a, b) if a == 0 then return b else return lua_gcd(b % a, a) end end
function lua_test()    return {1, 2, Fun("a"), "a", Fun("f", {1,2,3}), Sup(), Inf()} end
function lua_match()   return {Fun("",{1,2}),Fun("",{1,3}),Fun("",{2,1})} end
py_cmp = gringo.cmp
#end.

#script (python)
from gringo import Fun, Sup, Inf
def py_gcd(a, b): return b if a == 0 else py_gcd(b % a, a)
def py_test():    return [1, 2, Fun("a"), "a", Fun("f", [1,2,3]), Sup(), Inf()]
def py_match():   return [(1,2),(1,3),(2,1)]
lua_cmp = gringo.cmp
#end.

py_p(@py_gcd(2*3*7*13,3*11*13)).
py_q(@py_test()).
py_r(X) :- (1,X)=@py_match().
py_s(X) :- X=@py_match().
py_t(@py_cmp(1,a),1,a).
py_t(@py_cmp(b,1),b,1).
py_t(@py_cmp(1,1),1,1).

lua_p(@lua_gcd(2*3*7*13,3*11*13)).
lua_q(@lua_test()).
lua_r(X) :- (1,X)=@lua_match().
lua_s(X) :- X=@lua_match().
lua_t(@lua_cmp(1,a),1,a).
lua_t(@lua_cmp(b,1),b,1).
lua_t(@lua_cmp(1,1),1,1).

{a}.
:~ not a. [1]
