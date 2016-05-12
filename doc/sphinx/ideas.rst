**********
Some Ideas
**********

Extending  LUA
==============

Arbitrary Values
----------------

It would be nice to have LUA functions that can take arbitrary values as arguments.
This would require wrapping bingo's value class to lua class that provide methods
to recognize types or at least to work with the wrapped values, like extracting 
arguments of function symbols. Alternatively, values could be mapped to intrinsic 
LUA terms that are transfomed before and after. One has to take a look at LUA I am
no LUA expert:::
  #begin_lua
    function lua(x, y) 
      if y.is_func() then
        return y[1]
      else
        return x + y
      end
    end
  #end_lua
  a(X) :- X = @lua("abc",f(x,y)).

Array values
------------

The bingo syntax could be extended to allow for passing arrays of values to LUA.
This would allow for very interesting aggregations (over stratified parts of the 
program.) For example, we often pre-calculate certain values hidden in the input.
The next example shows how counting could be realized using this extension:::
  #begin_lua
    function lua(a) 
      s = 0
      for x in a do
        s = s + x
      end
      return x
    end
  #end_lua
  p(1..10).
  a(X) :- X = @lua[ X : p(X) ].
