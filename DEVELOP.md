# Build Environment

The clang project that ships with conda does not come with an up to date
libc++. Hence, the commands below simply setup a basic development environment
and installs the required clang manually.

Bring some cups of coffee to wait out the build process!

```
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
  -DCLANG_DEFAULT_RTLIB='compiler-rt'
cmake --build build
cmake --build build --target install
# configure clang to find libraries in the right places
cat > "${CONDA_PREFIX}/bin/clang.cfg" <<EOF
-Wl,-rpath="${CONDA_PREFIX}/lib/x86_64-unknown-linux-gnu"
-Wl,-rpath="${CONDA_PREFIX}/lib"
-L/mnt/scratch/kaminski/conda/envs/clang/lib
EOF
cp -fs "${CONDA_PREFIX}/bin/clang.cfg" "${CONDA_PREFIX}/bin/clang++.cfg"
```
