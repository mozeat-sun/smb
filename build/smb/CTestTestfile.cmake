# CMake generated Testfile for 
# Source directory: /home/mozeat/zoo/smb
# Build directory: /home/mozeat/zoo/build/smb
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(smb_comment_compliance "bash" "/home/mozeat/zoo/smb/tools/check_function_comments.sh")
set_tests_properties(smb_comment_compliance PROPERTIES  _BACKTRACE_TRIPLES "/home/mozeat/zoo/smb/CMakeLists.txt;62;add_test;/home/mozeat/zoo/smb/CMakeLists.txt;0;")
subdirs("src")
subdirs("tests")
