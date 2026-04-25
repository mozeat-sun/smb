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
    if (!context)
    {
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    if (!zoo_domain_profile_uses_assurance_mesh(context->domain_profile))
    {
        return ZOO_SMB_OK;
    }

    if (zoo_assurance_check_protocol_compatibility(&context->protocol) != ZOO_ASSURANCE_PROTOCOL_COMPATIBLE)
    {
        return ZOO_SMB_ERROR_VERSION_MISMATCH;
    }

    return ZOO_SMB_OK;
}
