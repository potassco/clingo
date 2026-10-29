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

- Online Demo: <https://potassco.org/clingo/run/>
- Python API Documentation: <https://docs.potassco.org/clingo/python-api/6.0/>
- C/C++ API Documentation: <https://docs.potassco.org/clingo/c-api/6.0/>

## 📦 Binary Packages

### 💚 Conda

Binary packages are available from [Anaconda]:

```sh
# stable
conda install -c potassco -c conda-forge clingo
# development
conda install -c potassco/label/dev-20 -c conda-forge clingo
```

Packages have to be installed on top of the conda-forge channel. The Miniforge
distribution ships a conda version configured to use this channel by default
(see Miniforge [installation instructions][conda]).

### 🐍 PyPI

Python packages are available from [PyPI]:

```sh
# stable
pip install clingo
# development
pip install --extra-index-url https://test.pypi.org/simple clingo
```

Development packages are available on the separate package index at
[Test PyPI].

### ☁️ Cloudsmith

Packages for Debian and Ubuntu are available on [Cloudsmith]:

```sh
# stable
curl -1sLf 'https://dl.cloudsmith.io/public/potassco/stable/setup.deb.sh' | sudo -E bash
sudo apt update
sudo apt install clingo python3-clingo
# development
curl -1sLf 'https://dl.cloudsmith.io/public/potassco/wip-20/setup.deb.sh' | sudo -E bash
sudo apt update
sudo apt install clingo python3-clingo
```

Note that only Ubuntu 24.04 and 26.04, and Debian 13 are supported at the
moment. For these distributions, only development (wip-20) packages are
available at the moment. Newer distributions or additional architectures might
be added later if there is demand.

### 🐧 Ubuntu PPA

For Ubuntu users, a [PPA] is available for easy installation and updates:

```sh
# stable
sudo add-apt-repository ppa:potassco/stable
sudo apt update
sudo apt install clingo python3-clingo
# development
sudo add-apt-repository ppa:potassco/wip-20
sudo apt update
sudo apt install clingo python3-clingo
```

Note that only Ubuntu 24.04 and 26.04 development versions are supported for
now. The packages are more or less the same as the ones for Cloudsmith but
built and shipped via Ubuntu's Launchpad.

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

Avoid running the `install` target as root or administrator; instead, set a
local prefix using the `CMAKE_INSTALL_PREFIX` variable when running cmake. If
Python support is important, installing into a virtual environment is a good
idea.

## 🐍 Building the Python Module

It is also possible to just build a Python module for Clingo, which allows you
to use Clingo from Python without needing to build the entire application.

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

[demo]: https://potassco.org/clingo/run/
[install]: https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html
[Anaconda]: https://anaconda.org/potassco/clingo/
[conda]: https://github.com/conda-forge/miniforge#install
[potassco]: https://potassco.org/
[quickstart]: https://potassco.org/guide/quickstart/
[PyPI]: https://pypi.org/project/clingo/
[Test PyPI]: https://test.pypi.org/project/clingo/
[Cloudsmith]: https://cloudsmith.io/~potassco/
[PPA]: https://launchpad.net/~potassco/
