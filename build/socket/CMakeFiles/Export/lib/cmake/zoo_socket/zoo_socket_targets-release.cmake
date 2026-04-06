#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "zoo::zoo_socket_static" for configuration "Release"
set_property(TARGET zoo::zoo_socket_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(zoo::zoo_socket_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libzoo_socket.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS zoo::zoo_socket_static )
list(APPEND _IMPORT_CHECK_FILES_FOR_zoo::zoo_socket_static "${_IMPORT_PREFIX}/lib/libzoo_socket.a" )

# Import target "zoo::zoo_socket_shared" for configuration "Release"
set_property(TARGET zoo::zoo_socket_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(zoo::zoo_socket_shared PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libzoo_socket.so.1.0.0"
  IMPORTED_SONAME_RELEASE "libzoo_socket.so.1"
  )

list(APPEND _IMPORT_CHECK_TARGETS zoo::zoo_socket_shared )
list(APPEND _IMPORT_CHECK_FILES_FOR_zoo::zoo_socket_shared "${_IMPORT_PREFIX}/lib/libzoo_socket.so.1.0.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
