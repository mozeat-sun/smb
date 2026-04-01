# CMake generated Testfile for 
# Source directory: /home/sky/zoo/socket/tests
# Build directory: /home/sky/zoo/socket/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(zoo_socket_unit_tests "/home/sky/zoo/socket/build/tests/zoo_socket_tests")
set_tests_properties(zoo_socket_unit_tests PROPERTIES  TIMEOUT "60" WORKING_DIRECTORY "/home/sky/zoo/socket/build/tests" _BACKTRACE_TRIPLES "/home/sky/zoo/socket/tests/CMakeLists.txt;124;add_test;/home/sky/zoo/socket/tests/CMakeLists.txt;0;")
subdirs("../_deps/unity-build")
