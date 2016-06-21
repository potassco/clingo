Gringo is a grounder that, given an input program with first-order variables,
computes an equivalent ground (variable-free) program. Its output can be
processed further with answer set solvers like clasp, cmodels, or smodels.

Clingo combines both gringo and clasp into a monolithic system. This way it
offers more control over the grounding and solving process than gringo and
clasp can offer individually: multi-shot solving.

The Python and Lua clingo modules offer the functionality of clingo in the
respective scripting language.

Reify is a small utility that reifies logic programs given in smodels format.
It produces a set of facts, which can be processed further with gringo.

Lpconvert is a converter between intermediate and smodels format.

Please consult the following resources for further information:

  - CHANGES:  changes between different releases
  - INSTALL:  installation instructions and software requirements
  - examples: a folder with examples each having a focus on certain features
              (many examples require clingo)

For more information please visit the project website: 
  
  http://potassco.sourceforge.net/

[![Build Status master](https://badges.herokuapp.com/travis/potassco/clingo?branch=master&label=master)](https://travis-ci.org/potassco/clingo?branch=master)
[![Build Status wip](https://badges.herokuapp.com/travis/potassco/clingo?branch=wip&label=wip)](https://travis-ci.org/potassco/clingo?branch=wip)
