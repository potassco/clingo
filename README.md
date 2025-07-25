# ⚡ Overview

Clingo is a cross-platform solver for Answer Set Programming (ASP), developed
as part of the [Potassco][potassco] project. It runs on Linux, macOS, Windows,
and can also be compiled to JavaScript for use in browsers and JavaScript
environments.

ASP offers a simple and powerful modeling language for describing combinatorial
problems as logic programs. Clingo takes such a logic program and computes
answer sets representing solutions to the given problem.

To get started, check out our [Getting Started][quickstart] page or try the
[Online Demo][demo].

## 📝 Resources

- Online Demo: <https://potassco.org/clingo-preview/>
- Python API Documentation: <https://potassco.org/clingo-preview/python-api/>
- C/C++ API Documentation: <https://potassco.org/clingo-preview/c-api/>

## 📦 Binary Packages

Binary packages are available from [Anaconda][anaconda]:

```sh
conda install -c potassco/label/dev-20 -c conda-forge clingo
```

You need to have [conda] installed (see Miniforge installation instructions).

## 🛠️ Building the Application

### 📋 Requirements

- a C++20 compiler
- re2c
- cmake
- ninja (recommended, optional)
- python (recommended, optional)

### 🏗️ Building

The instructions below work for both single-config and multi-config generators.
For faster builds, consider using the Ninja generator by adding `-G Ninja` to the
cmake command.

```sh
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### 📥 Installation

The `install` target copies the built files to standard system locations, as
determined by [GNUInstallDirs][install].

```sh
cmake --build build --config Release --target install
```

## 🐍 Building the Python Module

### 📋 Requirements

- a C++20 compiler
- python
- pipx (recommended)

### 🏗️ Building

Assuming pipx is installed, build the Python module with:

```sh
pipx run build .
```

### 📥 Installation

It is strongly recommended to install the package into a virtual environment to
avoid potential conflicts with other packages or system-wide Python
installations. To create and activate a virtual environment, you can use the
following commands:

```sh
python -m venv .venv
source .venv/bin/activate # Unix-based systems
.venv/Scripts/activate    # Windows
```

After building, install the generated wheel file from the `dist` folder:

```sh
pip install dist/clingo*.whl
```

[demo]: https://potassco.org/clingo-preview/
[install]: https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html
[anaconda]: https://anaconda.org/potassco/clingo
[conda]: https://github.com/conda-forge/miniforge#install
[potassco]: https://potassco.org/
[quickstart]: https://potassco.org/doc/start
