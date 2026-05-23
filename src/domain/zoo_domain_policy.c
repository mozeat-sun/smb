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

/**
 * @brief Get startup admission decision for a domain profile.
 * @param profile Active or requested domain profile.
 * @return ZOO_DOMAIN_STARTUP_DECISION_ENUM Startup decision result.
 */
ZOO_DOMAIN_STARTUP_DECISION_ENUM zoo_domain_policy_get_startup_decision(
    ZOO_DOMAIN_PROFILE_ENUM profile)
{
    if (!zoo_domain_profile_is_valid(profile))
    {
        return ZOO_DOMAIN_STARTUP_DECISION_REQUIRE_PROTOCOL_COMPATIBILITY;
    }

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

/**
 * @brief Classify startup assurance class for a domain profile.
 * @param profile Active or requested domain profile.
 * @return ZOO_DOMAIN_ASSURANCE_CLASS_ENUM Assurance class for startup checks.
 */
ZOO_DOMAIN_ASSURANCE_CLASS_ENUM zoo_domain_policy_classify_startup(
    ZOO_DOMAIN_PROFILE_ENUM profile)
{
    if (!zoo_domain_profile_is_valid(profile))
    {
        return ZOO_DOMAIN_ASSURANCE_CLASS_SAFETY_CRITICAL;
    }

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

/**
 * @brief Get minimum assurance class required during startup.
 * @param profile Active or requested domain profile.
 * @return ZOO_DOMAIN_ASSURANCE_CLASS_ENUM Minimum required assurance class.
 */
ZOO_DOMAIN_ASSURANCE_CLASS_ENUM zoo_domain_policy_get_min_assurance_class(
    ZOO_DOMAIN_PROFILE_ENUM profile)
{
    return zoo_domain_policy_classify_startup(profile);
}

/**
 * @brief Get default partition identifier for a domain profile.
 * @param profile Active or requested domain profile.
 * @return uint32_t Partition identifier, or UINT32_MAX for invalid profile.
 */
uint32_t zoo_domain_policy_get_partition_id(
    ZOO_DOMAIN_PROFILE_ENUM profile)
{
    if (!zoo_domain_profile_is_valid(profile))
    {
        return UINT32_MAX;
    }

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

/**
 * @brief Check whether peer identity is mandatory for a domain profile.
 * @param profile Active or requested domain profile.
 * @return bool true when identity is required, false otherwise.
 */
bool zoo_domain_policy_requires_identity(
    ZOO_DOMAIN_PROFILE_ENUM profile)
{
    if (!zoo_domain_profile_is_valid(profile))
    {
        return true;
    }

    return zoo_domain_profile_uses_assurance_mesh(profile);
}

/**
 * @brief Validate whether a partition id is allowed under a domain profile.
 * @param profile Active or requested domain profile.
 * @param partition_id Partition identifier to validate.
 * @return bool true when allowed by policy, false otherwise.
 */
bool zoo_domain_policy_is_partition_allowed(
    ZOO_DOMAIN_PROFILE_ENUM profile,
    uint32_t partition_id)
{
    if (!zoo_domain_profile_is_valid(profile))
    {
        return false;
    }

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
