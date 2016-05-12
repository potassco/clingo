.. _language_reference:

*****************
Language Refernce
*****************

.. _display_statements:

Display Statements
==================

There are two different kinds of display statements, one two show and one two hide predicates.
Both come in three flavours, they can be used to 
hide or show a whole predicate irrespective of any arguments (*local display statements*), 
adjust the initial visibility of predicates (*global display statements*), or
change the visibility of individual ground predicates (*individual display statements*).

Local display statements have the form: ``(#hide | #show) predicate/arity.`` 
to show or hide predicates, respectively.
Here, *predicate* refers to the name of a predicate and *arity* to the 
arity of the predicate. Global display statements are evaluated in the order 
they appear in input. A hide statement hides a predicate that was previously shown
and show statements cancel preceding hide statements. Initially, all predicates 
are shown. The following example shows the predicate ``q/1`` but hide 
the predicate ``p/1``::

  #hide p/1.
  p(a). p(b). p(c).
  q(X) :- p(X).

Second, there are global display statements to hide or show all predicates: 
``#show.`` and ``#hide.``. Those statements change the initial visibilty 
of predicates. Like in the previous form, both statements cancel each other 
in the order they appear in the program. Note that global statements do not
affect the visibility given by local display statements.
A common scenario is to hide all atoms and then individually 
show them on a per-predicate basis::

  #hide.
  #show q/1.
  p(a). p(b). p(c).
  q(X) :- p(X).

Finally, there are individual display statements: 
``(#hide | #show)  predicate (: conditional)*.`` to display individual ground atoms.
Here, *predicate* is a non-ground predicate and *conditional* some conditional.
Such statements partially undo previous local or global display specifications
but do not influence each other::

  #hide.
  #show p/2.
  #hide p(X,X).
  #show q(X,_) : r(X).
  p(a,a). p(a,b).
  r(a).
  q(a,a). q(a,b). q(b,a).

External Statements
===================

The external statement is used to mark certain atoms as external. This means 
that those atoms are not subject of simplification and consequently will not be removed 
from the logic program. There are two kinds of external directives, 
global and local external statements.

Global external statements have the form ``#external predicate/arity.`` where 
*predicate* refers to the name of a predicate and *arity* to the arity of the predicate.
They mark complete predicates irrespective of any arguments as external.
This means that nothing is known about the predicate and hence it cannot be used 
for instantiation::

  #external q/1.
  p(1). p(2).
  r(X) :- q(X), p(X).


Local external statements have the form ``#external predicate (: conditional)*.`` where
*predicate* is some non-ground predicate and *conditional* some conditional.
In contrast to global external directies, local external statements precisely specify 
which atoms are external and hence can be used for instantiation::

  #external q(X) : p(X).
  p(1). p(2).
  r(X) :- q(X).

Furthermore, the lparse output has been modified to include an additional table 
that stores a list  of all external atoms. For compatibility, this table is only inserted if the 
program actually contains external directives. It contains the respective atom indexes terminated
by a zero and is inserted directly after lparse' compute statement. Note that a warning is issued 
if an external atom is hidden::

  B+
  ...
  0
  B-
  ...
  0
  E
  2
  3
  4
  ...
  0
  42

