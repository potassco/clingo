#!/bin/bash

set -e

cpu_count=$(nproc --ignore=1)
build_path="build/pgo_clang"
options=("-S" "." "-B" "${build_path}" "-DCMAKE_C_COMPILER=clang-18" "-DCMAKE_CXX_COMPILER=clang++-18" "-DCMAKE_BUILD_TYPE=Release" "-DCLINGO_BUILD_TESTS=Off" "-DCLINGO_BUILD_EXAMPLES=Off")
flags=("-flto -fuse-ld=lld-18 -Wunused-command-line-argument")
cxx_flags=("-stdlib=libc++" "${flags[@]}")

rm -rf "${build_path:?}" || true
cmake "${options[@]}" -DCMAKE_C_FLAGS="-fprofile-instr-generate ${flags[*]}" -DCMAKE_CXX_FLAGS="-fprofile-instr-generate ${cxx_flags[*]}"
cmake --build "${build_path}" -j "${cpu_count}"

rm -f merged.profdata profile-*.profraw || true
export LLVM_PROFILE_FILE="profile-%p.profraw"
sleep 5
"${build_path}/bin/clingo" ascent-full-problem.lp --project-anonymous --opt-str=usc -q || true
llvm-profdata-18 merge -output=merged.profdata profile-*.profraw

cmake "${options[@]}" -DCMAKE_C_FLAGS="-fprofile-instr-use=$(pwd)/merged.profdata ${flags[*]}" -DCMAKE_CXX_FLAGS="-fprofile-instr-use=$(pwd)/merged.profdata ${cxx_flags[*]}"
cmake --build "${build_path}" -j "${cpu_count}"
