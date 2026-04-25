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

bool zoo_domain_profile_uses_assurance_mesh(ZOO_DOMAIN_PROFILE_ENUM profile)
{
    return profile == ZOO_DOMAIN_PROFILE_INDUSTRIAL ||
           profile == ZOO_DOMAIN_PROFILE_AUTOMOTIVE ||
           profile == ZOO_DOMAIN_PROFILE_MILITARY;
}
