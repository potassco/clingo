Gringo is a grounder that, given an input program with first-order variables,
computes an equivalent ground (variable-free) program. Its output can be
processed further with answer set solver clasp. Starting with gringo series 5,
its output is no longer compatible with solvers like smodels or cmodels reading
smodels format.

Clingo combines both gringo and clasp into a monolithic system. This way it
offers more control over the grounding and solving process than gringo and
clasp can offer individually: multi-shot solving.

The Python and Lua clingo modules offer the functionality of clingo in the
respective scripting language.

Reify is a small utility that reifies logic programs given in aspif format.
It produces a set of facts, which can be processed further with gringo.

Lpconvert is a converter between gringo's aspif and smodels format.

Please consult the following resources for further information:

  - CHANGES:  changes between different releases
  - INSTALL:  installation instructions and software requirements
  - examples: a folder with examples each having a focus on certain features
              (many examples require clingo)

For more information please visit the project website:

  http://potassco.org
