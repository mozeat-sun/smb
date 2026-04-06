# CMake generated Testfile for 
# Source directory: /home/mozeat/zoo/dispatcher/tests
# Build directory: /home/mozeat/zoo/build/dispatcher/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(zoo_dispatcher_unit_tests "/home/mozeat/zoo/build/dispatcher/tests/zoo_dispatcher_tests" "--unit")
set_tests_properties(zoo_dispatcher_unit_tests PROPERTIES  LABELS "unit" TIMEOUT "30" _BACKTRACE_TRIPLES "/home/mozeat/zoo/dispatcher/tests/CMakeLists.txt;72;add_test;/home/mozeat/zoo/dispatcher/tests/CMakeLists.txt;0;")
add_test(zoo_dispatcher_integration_tests "/home/mozeat/zoo/build/dispatcher/tests/zoo_dispatcher_tests" "--integration")
set_tests_properties(zoo_dispatcher_integration_tests PROPERTIES  LABELS "integration" TIMEOUT "60" _BACKTRACE_TRIPLES "/home/mozeat/zoo/dispatcher/tests/CMakeLists.txt;75;add_test;/home/mozeat/zoo/dispatcher/tests/CMakeLists.txt;0;")
add_test(zoo_dispatcher_performance_tests "/home/mozeat/zoo/build/dispatcher/tests/zoo_dispatcher_performance_tests_bin" "--basic")
set_tests_properties(zoo_dispatcher_performance_tests PROPERTIES  LABELS "performance" TIMEOUT "120" _BACKTRACE_TRIPLES "/home/mozeat/zoo/dispatcher/tests/CMakeLists.txt;78;add_test;/home/mozeat/zoo/dispatcher/tests/CMakeLists.txt;0;")
