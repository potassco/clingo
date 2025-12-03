# Solving Temporal Programs

This example demonstrates how to transform temporal logic programs for
incremental solving using clingo's Python API. The transformation rewrites
temporal programs by adding a time parameter to symbolic atoms and program
directives, enabling temporal reasoning and incremental solving.

## Features

- Adds a time parameter to all symbolic atoms and program directives.
- Converts temporal program sections (`final`, `initial`, `dynamic`, `static`)
  to their incremental counterparts.
- Injects temporal control atoms as needed.
- Provides a command-line interface for solving temporal logic programs.

## Usage

Transform a temporal ASP program using the command-line interface:

```bash
python tmode.py <input-files>
```

## Example

Given an input file `example.lp` containing temporal logic rules, running the
script computes its temporal stable models.

```bash
python tmode.py example.lp
```
