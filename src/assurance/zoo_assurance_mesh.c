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
