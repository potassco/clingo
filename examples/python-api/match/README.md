## Symbol Pattern Matching Example

This example demonstrates symbolic pattern matching for logic programming using
Python. It provides:

- A parser for matcher expressions (e.g., `f(X,10)`)
- Matchers for exact values, variables, and function structures
- Support for variable binding and tuple/function recognition

### Usage

1. Define a matcher expression as a string.
2. Parse a symbolic term using Clingo's `parse_term`.
3. Use `match(expression, symbol)` to check for matches and extract variable
   assignments.

### Running

```bash
python example.py
```

The script runs built-in tests for demonstration.
