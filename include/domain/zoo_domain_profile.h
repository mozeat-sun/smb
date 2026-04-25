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

ZOO_DOMAIN_PROFILE_ENUM zoo_domain_profile_get_active(void);
const char* zoo_domain_profile_to_string(ZOO_DOMAIN_PROFILE_ENUM profile);
bool zoo_domain_profile_uses_assurance_mesh(ZOO_DOMAIN_PROFILE_ENUM profile);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_DOMAIN_PROFILE_H */
