# CMake generated Testfile for 
# Source directory: /home/runner/work/clingo/clingo/lib/cxx-api
# Build directory: /home/runner/work/clingo/clingo/_codeql_build_dir/libclingo/cxx-api
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/home/runner/work/clingo/clingo/_codeql_build_dir/libclingo/cxx-api/test_clingo-cxx-api-b12d07c_include.cmake")
include("/home/runner/work/clingo/clingo/_codeql_build_dir/libclingo/cxx-api/test_clingo-cxx-app-b12d07c_include.cmake")
add_test([=[test_clingo-cxx-app-error-main]=] "/home/runner/work/clingo/clingo/_codeql_build_dir/bin/tests/test_clingo-cxx-app-error" "m")
set_tests_properties([=[test_clingo-cxx-app-error-main]=] PROPERTIES  PASS_REGULAR_EXPRESSION "\\*\\*\\* ERROR: \\(clingo\\): main" _BACKTRACE_TRIPLES "/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;105;add_test;/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;112;add_clingo_error_test;/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;0;")
add_test([=[test_clingo-cxx-app-error-print]=] "/home/runner/work/clingo/clingo/_codeql_build_dir/bin/tests/test_clingo-cxx-app-error" "p")
set_tests_properties([=[test_clingo-cxx-app-error-print]=] PROPERTIES  PASS_REGULAR_EXPRESSION "\\*\\*\\* ERROR: \\(clingo\\): print" _BACKTRACE_TRIPLES "/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;105;add_test;/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;112;add_clingo_error_test;/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;0;")
add_test([=[test_clingo-cxx-app-error-register]=] "/home/runner/work/clingo/clingo/_codeql_build_dir/bin/tests/test_clingo-cxx-app-error" "r")
set_tests_properties([=[test_clingo-cxx-app-error-register]=] PROPERTIES  PASS_REGULAR_EXPRESSION "\\*\\*\\* ERROR: \\(clingo\\): register" _BACKTRACE_TRIPLES "/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;105;add_test;/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;112;add_clingo_error_test;/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;0;")
add_test([=[test_clingo-cxx-app-error-validate]=] "/home/runner/work/clingo/clingo/_codeql_build_dir/bin/tests/test_clingo-cxx-app-error" "v")
set_tests_properties([=[test_clingo-cxx-app-error-validate]=] PROPERTIES  PASS_REGULAR_EXPRESSION "\\*\\*\\* ERROR: \\(clingo\\): validate" _BACKTRACE_TRIPLES "/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;105;add_test;/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;112;add_clingo_error_test;/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;0;")
add_test([=[test_clingo-cxx-app-error-option]=] "/home/runner/work/clingo/clingo/_codeql_build_dir/bin/tests/test_clingo-cxx-app-error" "o")
set_tests_properties([=[test_clingo-cxx-app-error-option]=] PROPERTIES  PASS_REGULAR_EXPRESSION "\\*\\*\\* ERROR: \\(clingo\\): option" _BACKTRACE_TRIPLES "/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;105;add_test;/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;112;add_clingo_error_test;/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;0;")
add_test([=[test_clingo-cxx-app-error-invalid]=] "/home/runner/work/clingo/clingo/_codeql_build_dir/bin/tests/test_clingo-cxx-app-error" "i")
set_tests_properties([=[test_clingo-cxx-app-error-invalid]=] PROPERTIES  PASS_REGULAR_EXPRESSION "\\*\\*\\* ERROR: \\(clingo\\): invalid" _BACKTRACE_TRIPLES "/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;105;add_test;/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;112;add_clingo_error_test;/home/runner/work/clingo/clingo/lib/cxx-api/CMakeLists.txt;0;")
