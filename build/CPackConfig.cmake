# This file will be configured to contain variables for CPack. These variables
# should be set in the CMake list file of the project before CPack module is
# included. The list of available CPACK_xxx variables and their associated
# documentation may be obtained using
#  cpack --help-variable-list
#
# Some variables are common to all generators (e.g. CPACK_PACKAGE_NAME)
# and some are specific to a generator
# (e.g. CPACK_NSIS_EXTRA_INSTALL_COMMANDS). The generator specific variables
# usually begin with CPACK_<GENNAME>_xxxx.


set(CPACK_BUILD_SOURCE_DIRS "/home/ubuntu/workspace/zoo;/home/ubuntu/workspace/zoo/build")
set(CPACK_CMAKE_GENERATOR "Unix Makefiles")
set(CPACK_COMPONENTS_ALL "Tests;Unspecified")
set(CPACK_COMPONENT_UNSPECIFIED_HIDDEN "TRUE")
set(CPACK_COMPONENT_UNSPECIFIED_REQUIRED "TRUE")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_FILE "/usr/share/cmake-3.28/Templates/CPack.GenericDescription.txt")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_SUMMARY "ZOO built using CMake")
set(CPACK_GENERATOR "TGZ;ZIP;STGZ;DEB;RPM")
set(CPACK_INNOSETUP_ARCHITECTURE "x64")
set(CPACK_INSTALL_CMAKE_PROJECTS "/home/ubuntu/workspace/zoo/build;ZOO;ALL;/")
set(CPACK_INSTALL_PREFIX "/home/ubuntu/workspace/zoo/stage")
set(CPACK_MODULE_PATH "/home/ubuntu/workspace/zoo/cmake")
set(CPACK_NSIS_DISPLAY_NAME "zoo 1.0.0")
set(CPACK_NSIS_INSTALLER_ICON_CODE "")
set(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
set(CPACK_NSIS_PACKAGE_NAME "zoo 1.0.0")
set(CPACK_NSIS_UNINSTALL_NAME "Uninstall")
set(CPACK_OBJCOPY_EXECUTABLE "/usr/bin/objcopy")
set(CPACK_OBJDUMP_EXECUTABLE "/usr/bin/objdump")
set(CPACK_OUTPUT_CONFIG_FILE "/home/ubuntu/workspace/zoo/build/CPackConfig.cmake")
set(CPACK_PACKAGE_CONTACT "support@zoo.com")
set(CPACK_PACKAGE_DEFAULT_LOCATION "/")
set(CPACK_PACKAGE_DESCRIPTION_FILE "/usr/share/cmake-3.28/Templates/CPack.GenericDescription.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ZOO Cross-Platform Library Collection")
set(CPACK_PACKAGE_DIRECTORY "/home/ubuntu/workspace/zoo/stage/packages")
set(CPACK_PACKAGE_FILE_NAME "zoo-1.0.0-Linux")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "zoo 1.0.0")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "zoo 1.0.0")
set(CPACK_PACKAGE_NAME "zoo")
set(CPACK_PACKAGE_RELOCATABLE "true")
set(CPACK_PACKAGE_VENDOR "ZOO Ltd")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_READELF_EXECUTABLE "/usr/bin/readelf")
set(CPACK_RESOURCE_FILE_LICENSE "/usr/share/cmake-3.28/Templates/CPack.GenericLicense.txt")
set(CPACK_RESOURCE_FILE_README "/usr/share/cmake-3.28/Templates/CPack.GenericDescription.txt")
set(CPACK_RESOURCE_FILE_WELCOME "/usr/share/cmake-3.28/Templates/CPack.GenericWelcome.txt")
set(CPACK_SET_DESTDIR "OFF")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_OUTPUT_CONFIG_FILE "/home/ubuntu/workspace/zoo/build/CPackSourceConfig.cmake")
set(CPACK_SYSTEM_NAME "Linux")
set(CPACK_THREADS "1")
set(CPACK_TOPLEVEL_TAG "Linux")
set(CPACK_WIX_SIZEOF_VOID_P "8")

if(NOT CPACK_PROPERTIES_FILE)
  set(CPACK_PROPERTIES_FILE "/home/ubuntu/workspace/zoo/build/CPackProperties.cmake")
endif()

if(EXISTS ${CPACK_PROPERTIES_FILE})
  include(${CPACK_PROPERTIES_FILE})
endif()
