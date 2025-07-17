# Build Environment

The clang project that ships with conda does not come with an up-to-date
libc++. Hence, the commands below setup a development environment with a
manually complied clang.

Bring some cups of coffee to wait out the build process and note that this
won't work on MacOS!

```bash
# create conda environment
conda create -n clang -c conda-forge cxx-compiler cmake ninja 'python>=3.12' pynvim libxml2 swig git doxygen
conda activate clang
# clone, configure, build, install llvm project
git clone -b release/18.x https://github.com/llvm/llvm-project.git
cmake -G Ninja -S llvm -B build \
  -DLLVM_ENABLE_PROJECTS='clang;clang-tools-extra;lld;lldb' \
  -DLLVM_ENABLE_RUNTIMES='compiler-rt;libcxx;libcxxabi;libunwind' \
  -DCMAKE_BUILD_TYPE='Release' \
  -DCMAKE_INSTALL_PREFIX="$CONDA_PREFIX" \
  -DCMAKE_FIND_ROOT_PATH="$CONDA_PREFIX" \
  -DCLANG_DEFAULT_CXX_STDLIB='libc++' \
  -DCLANG_DEFAULT_LINKER='lld' \
  -DCLANG_DEFAULT_RTLIB='compiler-rt' \
  -DLLVM_ENABLE_LIBCXX=ON \
  -DLLVM_ENABLE_LIBCXXABI=ON \
  -DCLANG_DEFAULT_CXX_STDLIB=libc++ \
  -DCLANG_DEFAULT_RTLIB=compiler-rt
cmake --build build
cmake --build build --target install
# configure clang to find libraries in the right places
cat > "${CONDA_PREFIX}/bin/clang.cfg" <<EOF
-Wl,-rpath="${CONDA_PREFIX}/lib/x86_64-unknown-linux-gnu"
-Wl,-rpath="${CONDA_PREFIX}/lib"
-L"${CONDA_PREFIX}/lib"
EOF
cp -fs "${CONDA_PREFIX}/bin/clang.cfg" "${CONDA_PREFIX}/bin/clang++.cfg"
```

## Installing Neovide Using Libraries From Conda Environments

```bash
export LIBRARY_PATH="${CONDA_PREFIX}/lib"
export RUSTFLAGS="-C link-args=-Wl,-rpath,${LIBRARY_PATH}"
cargo install --git https://github.com/neovide/neovide.git
```

## Building Python Extensions

The commands below show how to build wheels for the current platform and for
pyodide.

```bash
# build using the current python environment
pipx run build .
# build using the latest stable cibw
pipx run cibuildwheel --platform pyodide
# build using the latest master of cibw
pipx run --spec git+https://github.com/pypa/cibuildwheel -- cibuildwheel --only cp313-pyodide_wasm32 .
```
