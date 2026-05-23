/*******************************************************************************
 * Copyright (C) 2026, ZOO Ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Domain
 * Component id: ZOO_DOMAIN_PROFILE
 * File name: zoo_domain_profile.c
 * Description: Domain profile selection and capability helpers
 ******************************************************************************/

#include "domain/zoo_domain_profile.h"

/**
 * @brief Validate whether a profile enum belongs to the supported domain set.
 * @param profile Domain profile value to validate.
 * @return bool true when the profile value is supported, false otherwise.
 */
bool zoo_domain_profile_is_valid(ZOO_DOMAIN_PROFILE_ENUM profile)
{
    switch (profile)
    {
        case ZOO_DOMAIN_PROFILE_GENERIC:
        case ZOO_DOMAIN_PROFILE_INDUSTRIAL:
        case ZOO_DOMAIN_PROFILE_AUTOMOTIVE:
        case ZOO_DOMAIN_PROFILE_MILITARY:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Resolve active compile-time domain profile.
 * @return ZOO_DOMAIN_PROFILE_ENUM Active domain profile configured for build.
 */
ZOO_DOMAIN_PROFILE_ENUM zoo_domain_profile_get_active(void)
{
#if defined(ZOO_DOMAIN_PROFILE_military)
    return ZOO_DOMAIN_PROFILE_MILITARY;
#elif defined(ZOO_DOMAIN_PROFILE_automotive)
    return ZOO_DOMAIN_PROFILE_AUTOMOTIVE;
#elif defined(ZOO_DOMAIN_PROFILE_industrial)
    return ZOO_DOMAIN_PROFILE_INDUSTRIAL;
#else
    return ZOO_DOMAIN_PROFILE_GENERIC;
#endif
}

/**
 * @brief Convert domain profile enum to canonical lowercase string.
 * @param profile Domain profile value to stringify.
 * @return const char* Stable profile string, or "unknown" for unsupported values.
 */
const char* zoo_domain_profile_to_string(ZOO_DOMAIN_PROFILE_ENUM profile)
{
    switch (profile)
    {
        case ZOO_DOMAIN_PROFILE_GENERIC:
            return "generic";
        case ZOO_DOMAIN_PROFILE_INDUSTRIAL:
            return "industrial";
        case ZOO_DOMAIN_PROFILE_AUTOMOTIVE:
            return "automotive";
        case ZOO_DOMAIN_PROFILE_MILITARY:
            return "military";
        default:
            return "unknown";
    }
}

/**
 * @brief Check whether assurance mesh policies apply to the profile.
 * @param profile Domain profile value to evaluate.
 * @return bool true when mesh-based assurance is enabled for the profile.
 */
bool zoo_domain_profile_uses_assurance_mesh(ZOO_DOMAIN_PROFILE_ENUM profile)
{
    return zoo_domain_profile_is_valid(profile) &&
           profile != ZOO_DOMAIN_PROFILE_GENERIC;
}
