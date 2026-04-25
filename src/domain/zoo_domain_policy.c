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
