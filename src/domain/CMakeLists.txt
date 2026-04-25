set(CMAKE_C_STANDARD 11)

add_library(zoo_domain_static STATIC
    zoo_domain_profile.c
    zoo_domain_policy.c
)
set_target_properties(zoo_domain_static PROPERTIES OUTPUT_NAME zoo_domain)
target_compile_definitions(zoo_domain_static PRIVATE
    ZOO_DOMAIN_PROFILE_${ZOO_DOMAIN_PROFILE}=1
)
target_include_directories(zoo_domain_static PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/domain
)

target_link_libraries(zoo_domain_static PUBLIC
    zoo_util_static
)

add_library(zoo_domain_shared SHARED
    zoo_domain_profile.c
    zoo_domain_policy.c
)
set_target_properties(zoo_domain_shared PROPERTIES OUTPUT_NAME zoo_domain)
target_compile_definitions(zoo_domain_shared PRIVATE
    ZOO_DOMAIN_PROFILE_${ZOO_DOMAIN_PROFILE}=1
)
target_include_directories(zoo_domain_shared PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/domain
)

target_link_libraries(zoo_domain_shared PUBLIC
    zoo_util_shared
)

install(TARGETS zoo_domain_static zoo_domain_shared
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
