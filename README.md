# Quickstart

This readme is a stub subject to future improvement!

## Building the Application

### Requirements

- a C++20 compiler
- re2c
- cmake
- ninja (recommended, optional)
- python (recommended, optional)

### Building

The instructions below work for both single-config and multi-config generators.
For faster builds, consider using the Ninja generator by adding `-G Ninja` to the
cmake command.

```sh
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Installation

There is currently no install target configured. However, the binary
`build/bin/clingo` is ready for use after building.

## Building the Python Module

### Requirements

- a C++20 compiler
- python
- pipx (recommended)

### Building

Assuming pipx is installed, build the Python module with:

```sh
pipx run build .
```

### Installation

After building, install the generated wheel file from the `dist` folder:

```sh
pip install dist/clingo*.whl
```
