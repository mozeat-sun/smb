/*******************************************************************************
 * Copyright (C) 2026, ZOO Ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Domain
 * Component id: ZOO_DOMAIN_POLICY
 * File name: zoo_domain_policy.h
 * Description: Domain policy contracts for assurance and runtime composition
 ******************************************************************************/

#ifndef ZOO_DOMAIN_POLICY_H
#define ZOO_DOMAIN_POLICY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "domain/zoo_domain_profile.h"

typedef enum
{
    ZOO_DOMAIN_STARTUP_DECISION_ALLOW = 0,
    ZOO_DOMAIN_STARTUP_DECISION_REQUIRE_PROTOCOL_COMPATIBILITY
} ZOO_DOMAIN_STARTUP_DECISION_ENUM;

typedef enum
{
    ZOO_DOMAIN_ASSURANCE_CLASS_BEST_EFFORT = 0,
    ZOO_DOMAIN_ASSURANCE_CLASS_CONTROL,
    ZOO_DOMAIN_ASSURANCE_CLASS_SAFETY_CRITICAL
} ZOO_DOMAIN_ASSURANCE_CLASS_ENUM;

ZOO_DOMAIN_STARTUP_DECISION_ENUM zoo_domain_policy_get_startup_decision(
    ZOO_DOMAIN_PROFILE_ENUM profile);

ZOO_DOMAIN_ASSURANCE_CLASS_ENUM zoo_domain_policy_classify_startup(
    ZOO_DOMAIN_PROFILE_ENUM profile);

ZOO_DOMAIN_ASSURANCE_CLASS_ENUM zoo_domain_policy_get_min_assurance_class(
    ZOO_DOMAIN_PROFILE_ENUM profile);

uint32_t zoo_domain_policy_get_partition_id(
    ZOO_DOMAIN_PROFILE_ENUM profile);

bool zoo_domain_policy_is_partition_allowed(
    ZOO_DOMAIN_PROFILE_ENUM profile,
    uint32_t partition_id);

bool zoo_domain_policy_requires_identity(
    ZOO_DOMAIN_PROFILE_ENUM profile);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_DOMAIN_POLICY_H */
