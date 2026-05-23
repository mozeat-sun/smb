/*******************************************************************************
 * Copyright (C) 2026, ZOO Ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Assurance
 * Component id: ZOO_ASSURANCE_MESH
 * File name: zoo_assurance_mesh.c
 * Description: Assurance mesh startup and protocol policy APIs
 ******************************************************************************/

#include "assurance/zoo_assurance_mesh.h"

/**
 * @brief Resolve policy snapshot fields for a given domain profile.
 * @param profile Domain profile used to derive startup policy.
 * @param snapshot Output policy snapshot populated on success.
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error code on failure.
 */
ZOO_ERROR_TYPE zoo_assurance_resolve_policy(
    ZOO_DOMAIN_PROFILE_ENUM profile,
    ZOO_ASSURANCE_POLICY_SNAPSHOT_STRUCT* snapshot)
{
    if (!snapshot)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    snapshot->startup_decision = zoo_domain_policy_get_startup_decision(profile);
    snapshot->assurance_class = zoo_domain_policy_classify_startup(profile);
    snapshot->partition_id = zoo_domain_policy_get_partition_id(profile);
    return ZOO_SMB_OK;
}

/**
 * @brief Validate a policy snapshot against profile-derived policy constraints.
 * @param profile Domain profile used for validation.
 * @param snapshot Policy snapshot to validate.
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK when valid, error code otherwise.
 */
ZOO_ERROR_TYPE zoo_assurance_validate_policy_snapshot(
    ZOO_DOMAIN_PROFILE_ENUM profile,
    const ZOO_ASSURANCE_POLICY_SNAPSHOT_STRUCT* snapshot)
{
    ZOO_DOMAIN_ASSURANCE_CLASS_ENUM min_class;

    if (!snapshot)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (!zoo_domain_policy_is_partition_allowed(profile, snapshot->partition_id))
    {
        return ZOO_SMB_ERROR_INVALID_STATE;
    }

    min_class = zoo_domain_policy_get_min_assurance_class(profile);
    if (snapshot->assurance_class < min_class)
    {
        return ZOO_SMB_ERROR_INVALID_STATE;
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Check protocol wire compatibility between local and peer endpoints.
 * @param context Protocol compatibility context.
 * @return ZOO_ASSURANCE_PROTOCOL_COMPATIBILITY_ENUM Compatibility result.
 */
ZOO_ASSURANCE_PROTOCOL_COMPATIBILITY_ENUM zoo_assurance_check_protocol_compatibility(
    const ZOO_ASSURANCE_PROTOCOL_CONTEXT_STRUCT* context)
{
    if (!context)
    {
        return ZOO_ASSURANCE_PROTOCOL_INCOMPATIBLE;
    }

    if (context->local_wire_major != context->peer_wire_major)
    {
        return ZOO_ASSURANCE_PROTOCOL_INCOMPATIBLE;
    }

    if (context->peer_wire_minor > context->local_wire_minor)
    {
        return ZOO_ASSURANCE_PROTOCOL_INCOMPATIBLE;
    }

    return ZOO_ASSURANCE_PROTOCOL_COMPATIBLE;
}

/**
 * @brief Evaluate startup admission using profile policy and protocol checks.
 * @param context Startup context for policy and compatibility evaluation.
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK when startup is allowed, error otherwise.
 */
ZOO_ERROR_TYPE zoo_assurance_evaluate_startup(const ZOO_ASSURANCE_STARTUP_CONTEXT_STRUCT* context)
{
    ZOO_ASSURANCE_POLICY_SNAPSHOT_STRUCT snapshot;

    if (!context)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (zoo_assurance_resolve_policy(context->domain_profile, &snapshot) != ZOO_SMB_OK)
    {
        return ZOO_SMB_ERROR_OPERATION_FAILED;
    }

    if (zoo_assurance_validate_policy_snapshot(context->domain_profile, &snapshot) != ZOO_SMB_OK)
    {
        return ZOO_SMB_ERROR_INVALID_STATE;
    }

    if (snapshot.startup_decision == ZOO_DOMAIN_STARTUP_DECISION_ALLOW)
    {
        return ZOO_SMB_OK;
    }

    if (zoo_assurance_check_protocol_compatibility(&context->protocol) != ZOO_ASSURANCE_PROTOCOL_COMPATIBLE)
    {
        return ZOO_SMB_ERROR_VERSION_MISMATCH;
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Evaluate peer admission constraints for runtime communication.
 * @param context Admission context containing identity and partition inputs.
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK when admission is allowed, error otherwise.
 */
ZOO_ERROR_TYPE zoo_assurance_evaluate_admission(
    const ZOO_ASSURANCE_ADMISSION_CONTEXT_STRUCT* context)
{
    if (!context)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (!zoo_domain_policy_is_partition_allowed(context->domain_profile, context->partition_id))
    {
        return ZOO_SMB_ERROR_INVALID_STATE;
    }

    if (zoo_domain_policy_requires_identity(context->domain_profile))
    {
        if (!context->local_peer_id || context->local_peer_id[0] == '\0')
        {
            return ZOO_SMB_ERROR_INVALID_STATE;
        }
        if (!context->remote_peer_id || context->remote_peer_id[0] == '\0')
        {
            return ZOO_SMB_ERROR_INVALID_STATE;
        }
    }

    return ZOO_SMB_OK;
}
