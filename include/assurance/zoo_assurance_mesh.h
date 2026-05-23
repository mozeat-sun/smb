/*******************************************************************************
 * Copyright (C) 2026, ZOO Ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Assurance
 * Component id: ZOO_ASSURANCE_MESH
 * File name: zoo_assurance_mesh.h
 * Description: Assurance mesh startup and protocol policy APIs
 ******************************************************************************/

#ifndef ZOO_ASSURANCE_MESH_H
#define ZOO_ASSURANCE_MESH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "zoo_smb_error.h"
#include "domain/zoo_domain_profile.h"
#include "domain/zoo_domain_policy.h"

typedef enum
{
    ZOO_ASSURANCE_PROTOCOL_COMPATIBLE = 0,
    ZOO_ASSURANCE_PROTOCOL_INCOMPATIBLE
} ZOO_ASSURANCE_PROTOCOL_COMPATIBILITY_ENUM;

typedef struct
{
    uint16_t local_wire_major;
    uint16_t local_wire_minor;
    uint16_t peer_wire_major;
    uint16_t peer_wire_minor;
} ZOO_ASSURANCE_PROTOCOL_CONTEXT_STRUCT;

typedef struct
{
    ZOO_DOMAIN_PROFILE_ENUM domain_profile;
    ZOO_ASSURANCE_PROTOCOL_CONTEXT_STRUCT protocol;
} ZOO_ASSURANCE_STARTUP_CONTEXT_STRUCT;

typedef struct
{
    ZOO_DOMAIN_STARTUP_DECISION_ENUM startup_decision;
    ZOO_DOMAIN_ASSURANCE_CLASS_ENUM assurance_class;
    uint32_t partition_id;
} ZOO_ASSURANCE_POLICY_SNAPSHOT_STRUCT;

typedef struct
{
    ZOO_DOMAIN_PROFILE_ENUM domain_profile;
    const char* local_peer_id;
    const char* remote_peer_id;
    uint32_t partition_id;
} ZOO_ASSURANCE_ADMISSION_CONTEXT_STRUCT;

/**
 * @brief Check protocol compatibility between local and peer wire versions.
 * @param context Protocol version context.
 * @return ZOO_ASSURANCE_PROTOCOL_COMPATIBILITY_ENUM Compatibility result.
 */
ZOO_ASSURANCE_PROTOCOL_COMPATIBILITY_ENUM zoo_assurance_check_protocol_compatibility(
    const ZOO_ASSURANCE_PROTOCOL_CONTEXT_STRUCT* context);

/**
 * @brief Resolve policy snapshot for a specific domain profile.
 * @param profile Domain profile.
 * @param snapshot Output policy snapshot.
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK on success, error otherwise.
 */
ZOO_ERROR_TYPE zoo_assurance_resolve_policy(
    ZOO_DOMAIN_PROFILE_ENUM profile,
    ZOO_ASSURANCE_POLICY_SNAPSHOT_STRUCT* snapshot);

/**
 * @brief Validate a policy snapshot against domain policy constraints.
 * @param profile Domain profile.
 * @param snapshot Policy snapshot to validate.
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK when valid, error otherwise.
 */
ZOO_ERROR_TYPE zoo_assurance_validate_policy_snapshot(
    ZOO_DOMAIN_PROFILE_ENUM profile,
    const ZOO_ASSURANCE_POLICY_SNAPSHOT_STRUCT* snapshot);

/**
 * @brief Evaluate startup allowance according to policy and protocol checks.
 * @param context Startup context.
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK when startup is allowed.
 */
ZOO_ERROR_TYPE zoo_assurance_evaluate_startup(const ZOO_ASSURANCE_STARTUP_CONTEXT_STRUCT* context);

/**
 * @brief Evaluate admission policy for runtime peer communication.
 * @param context Admission context.
 * @return ZOO_ERROR_TYPE ZOO_SMB_OK when admission is allowed.
 */
ZOO_ERROR_TYPE zoo_assurance_evaluate_admission(
    const ZOO_ASSURANCE_ADMISSION_CONTEXT_STRUCT* context);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_ASSURANCE_MESH_H */
