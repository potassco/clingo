## Predicate Dependency Graph Example

This example demonstrates how to compute and visualize the predicate dependency
graph of a logic program using Clingo's AST interface.

### Features

- Parses logic programs using Clingo's AST.
- Builds a dependency graph between predicates.
- Analyzes strongly connected components.
- Outputs the graph in Graphviz dot format for visualization.

### Usage

```bash
python example.py <input.lp>
```

Replace `<input.lp>` with your logic program file.

### Output

The script prints the dependency graph in dot format. You can visualize it
using Graphviz:

```bash
python example.py example.lp > example.dot
dot -Tpng example.dot -o example.png
```
