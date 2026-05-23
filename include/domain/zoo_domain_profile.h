/*******************************************************************************
 * Copyright (C) 2026, ZOO Ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Domain
 * Component id: ZOO_DOMAIN_PROFILE
 * File name: zoo_domain_profile.h
 * Description: Domain profile selection and capability helpers
 ******************************************************************************/

#ifndef ZOO_DOMAIN_PROFILE_H
#define ZOO_DOMAIN_PROFILE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

typedef enum
{
    ZOO_DOMAIN_PROFILE_GENERIC = 0,
    ZOO_DOMAIN_PROFILE_INDUSTRIAL,
    ZOO_DOMAIN_PROFILE_AUTOMOTIVE,
    ZOO_DOMAIN_PROFILE_MILITARY
} ZOO_DOMAIN_PROFILE_ENUM;

/**
 * @brief Validate whether a profile enum value is supported.
 * @param profile Domain profile value to validate.
 * @return bool true when profile is valid, false otherwise.
 */
bool zoo_domain_profile_is_valid(ZOO_DOMAIN_PROFILE_ENUM profile);

/**
 * @brief Resolve the active compile-time domain profile.
 * @return ZOO_DOMAIN_PROFILE_ENUM Active profile selected by build definition.
 */
ZOO_DOMAIN_PROFILE_ENUM zoo_domain_profile_get_active(void);

/**
 * @brief Convert domain profile enum to canonical string value.
 * @param profile Domain profile value.
 * @return const char* Lowercase profile name, or "unknown" for invalid value.
 */
const char* zoo_domain_profile_to_string(ZOO_DOMAIN_PROFILE_ENUM profile);

/**
 * @brief Check whether assurance mesh policy is used by profile.
 * @param profile Domain profile value.
 * @return bool true when profile requires assurance mesh policy.
 */
bool zoo_domain_profile_uses_assurance_mesh(ZOO_DOMAIN_PROFILE_ENUM profile);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_DOMAIN_PROFILE_H */
