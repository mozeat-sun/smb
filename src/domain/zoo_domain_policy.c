/*******************************************************************************
 * Copyright (C) 2026, ZOO Ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Domain
 * Component id: ZOO_DOMAIN_POLICY
 * File name: zoo_domain_policy.c
 * Description: Domain policy contracts for assurance and runtime composition
 ******************************************************************************/

#include "domain/zoo_domain_policy.h"

ZOO_DOMAIN_STARTUP_DECISION_ENUM zoo_domain_policy_get_startup_decision(
    ZOO_DOMAIN_PROFILE_ENUM profile)
{
    switch (profile)
    {
        case ZOO_DOMAIN_PROFILE_INDUSTRIAL:
        case ZOO_DOMAIN_PROFILE_AUTOMOTIVE:
        case ZOO_DOMAIN_PROFILE_MILITARY:
            return ZOO_DOMAIN_STARTUP_DECISION_REQUIRE_PROTOCOL_COMPATIBILITY;
        case ZOO_DOMAIN_PROFILE_GENERIC:
        default:
            return ZOO_DOMAIN_STARTUP_DECISION_ALLOW;
    }
}

ZOO_DOMAIN_ASSURANCE_CLASS_ENUM zoo_domain_policy_classify_startup(
    ZOO_DOMAIN_PROFILE_ENUM profile)
{
    switch (profile)
    {
        case ZOO_DOMAIN_PROFILE_MILITARY:
            return ZOO_DOMAIN_ASSURANCE_CLASS_SAFETY_CRITICAL;
        case ZOO_DOMAIN_PROFILE_AUTOMOTIVE:
            return ZOO_DOMAIN_ASSURANCE_CLASS_CONTROL;
        case ZOO_DOMAIN_PROFILE_INDUSTRIAL:
            return ZOO_DOMAIN_ASSURANCE_CLASS_CONTROL;
        case ZOO_DOMAIN_PROFILE_GENERIC:
        default:
            return ZOO_DOMAIN_ASSURANCE_CLASS_BEST_EFFORT;
    }
}

ZOO_DOMAIN_ASSURANCE_CLASS_ENUM zoo_domain_policy_get_min_assurance_class(
    ZOO_DOMAIN_PROFILE_ENUM profile)
{
    return zoo_domain_policy_classify_startup(profile);
}

uint32_t zoo_domain_policy_get_partition_id(
    ZOO_DOMAIN_PROFILE_ENUM profile)
{
    switch (profile)
    {
        case ZOO_DOMAIN_PROFILE_MILITARY:
            return 30U;
        case ZOO_DOMAIN_PROFILE_AUTOMOTIVE:
            return 20U;
        case ZOO_DOMAIN_PROFILE_INDUSTRIAL:
            return 10U;
        case ZOO_DOMAIN_PROFILE_GENERIC:
        default:
            return 0U;
    }
}

bool zoo_domain_policy_requires_identity(
    ZOO_DOMAIN_PROFILE_ENUM profile)
{
    return profile == ZOO_DOMAIN_PROFILE_INDUSTRIAL ||
           profile == ZOO_DOMAIN_PROFILE_AUTOMOTIVE ||
           profile == ZOO_DOMAIN_PROFILE_MILITARY;
}

bool zoo_domain_policy_is_partition_allowed(
    ZOO_DOMAIN_PROFILE_ENUM profile,
    uint32_t partition_id)
{
    switch (profile)
    {
        case ZOO_DOMAIN_PROFILE_INDUSTRIAL:
            return partition_id >= 10U && partition_id < 20U;
        case ZOO_DOMAIN_PROFILE_AUTOMOTIVE:
            return partition_id >= 20U && partition_id < 30U;
        case ZOO_DOMAIN_PROFILE_MILITARY:
            return partition_id >= 30U && partition_id < 40U;
        case ZOO_DOMAIN_PROFILE_GENERIC:
        default:
            return partition_id == 0U;
    }
}
