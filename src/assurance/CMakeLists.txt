set(CMAKE_C_STANDARD 11)

add_library(zoo_assurance_static STATIC
    zoo_assurance_mesh.c
)
set_target_properties(zoo_assurance_static PROPERTIES OUTPUT_NAME zoo_assurance)
target_compile_definitions(zoo_assurance_static PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
    ZOO_DOMAIN_PROFILE_${ZOO_DOMAIN_PROFILE}=1
)
target_include_directories(zoo_assurance_static PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/assurance
    ${CMAKE_SOURCE_DIR}/include/domain
    ${CMAKE_SOURCE_DIR}/include/kernal/utility
)
target_link_libraries(zoo_assurance_static PUBLIC
    zoo_domain_static
)

add_library(zoo_assurance_shared SHARED
    zoo_assurance_mesh.c
)
set_target_properties(zoo_assurance_shared PROPERTIES OUTPUT_NAME zoo_assurance)
target_compile_definitions(zoo_assurance_shared PRIVATE
    _GNU_SOURCE
    _POSIX_C_SOURCE=200809L
    ZOO_DOMAIN_PROFILE_${ZOO_DOMAIN_PROFILE}=1
)
target_include_directories(zoo_assurance_shared PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/assurance
    ${CMAKE_SOURCE_DIR}/include/domain
    ${CMAKE_SOURCE_DIR}/include/kernal/utility
)
target_link_libraries(zoo_assurance_shared PUBLIC
    zoo_domain_shared
)

install(TARGETS zoo_assurance_static zoo_assurance_shared
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
