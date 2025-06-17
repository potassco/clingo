#!/bin/bash

set -e

cpu_count=$(nproc --ignore=1)
build_path="build/pgo"
options=("-S" "." "-B" "${build_path}" "-DCMAKE_BUILD_TYPE=Release" "-DCLINGO_BUILD_TESTS=Off" "-DCLINGO_BUILD_EXAMPLES=Off")
flags=("-flto=auto -fuse-linker-plugin")

rm -rf "${build_path:?}" || true
cmake "${options[@]}" -DCMAKE_CXX_FLAGS="-fprofile-generate ${flags[*]}" -DCMAKE_C_FLAGS="-fprofile-generate ${flags[*]}"
cmake --build "${build_path}" -j "${cpu_count}"

sleep 5
"${build_path}/bin/clingo" ascent-full-problem.lp --project-anonymous --opt-str=usc -q || true

cmake "${options[@]}" -DCMAKE_CXX_FLAGS="-fprofile-use ${flags[*]}" -DCMAKE_C_FLAGS="-fprofile-use ${flags[*]}"
cmake --build "${build_path}" -j "${cpu_count}"
