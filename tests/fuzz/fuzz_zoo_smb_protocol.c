/*******************************************************************************
 * Copyright (C) 2026, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: FUZZ_ZOO_SMB_PROTOCOL
 * File name: fuzz_zoo_smb_protocol.c
 * Description: Deterministic protocol fuzz entry point for parser, header,
 *              and serialization surfaces.
 * Traceability coverage:
 * - REQ-PERF-003: release-quality regression harness includes protocol fuzz smoke execution.
 * - REQ-SAFE-001: protocol entrypoints are exercised under malformed and mutated inputs.
 * - REQ-SEC-001: threat model assumes malformed-input adversaries at protocol boundary.
 * - REQ-SAFE-003: requirement-linked verification evidence for graded releases.
 * History recorder:
 * Version   date           author            context
 * 1.0       2026-04-11     github.copilot    created
 ******************************************************************************/

#include "zoo_memory_pool.h"
#include "zoo_smb_message.h"
#include "zoo_smb_protocol.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct
{
    uint32_t seed_cases;
    uint32_t mutated_cases;
    uint32_t header_cases;
    uint32_t deserialize_cases;
    uint32_t serialize_cases;
    uint32_t discovery_cases;
} FUZZ_REPORT_STRUCT;

typedef struct
{
    ZOO_SMB_MSG_STRUCT msg;
    uint8_t payload[512];
} MSG_STORAGE_STRUCT;

static uint32_t fuzz_next(uint32_t* state)
{
    *state = (*state * 1103515245u) + 12345u;
    return *state;
}

static void fuzz_header_case(FUZZ_REPORT_STRUCT* report, const uint8_t* bytes, size_t len)
{
    ZOO_SMB_MSG_HEADER_STRUCT header;
    memset(&header, 0, sizeof(header));
    memcpy(&header, bytes, len < sizeof(header) ? len : sizeof(header));
    (void)zoo_smb_protocol_is_header_valid(&header);
    (void)zoo_smb_protocol_is_header_valid_with_msg_type(&header, ZOO_SMB_MSG_TYPE_REQ);
    (void)zoo_smb_protocol_swap_header(&header);
    report->header_cases += 1u;
}

static void fuzz_deserialize_case(FUZZ_REPORT_STRUCT* report, const uint8_t* bytes, size_t len)
{
    MSG_STORAGE_STRUCT storage;
    memset(&storage, 0, sizeof(storage));
    (void)zoo_smb_protocol_deserialize(bytes, len, &storage.msg, sizeof(storage));
    report->deserialize_cases += 1u;
}

static void fuzz_roundtrip_case(FUZZ_REPORT_STRUCT* report)
{
    static const char payload[] = "fuzz-payload";
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));

    ZOO_SMB_MSG_STRUCT* msg = zoo_smb_create_message(
        ZOO_SMB_MSG_TYPE_REQ,
        "fuzz_sender",
        "fuzz/topic",
        payload,
        sizeof(payload),
        77u,
        88u);
    if (!msg)
    {
        return;
    }

    size_t written = zoo_smb_protocol_serialize(msg, buffer, sizeof(buffer));
    if (written > 0u)
    {
        fuzz_deserialize_case(report, buffer, written);
    }
    report->serialize_cases += 1u;
    zoo_smb_destroy_message(msg);
}

static void fuzz_discovery_case(FUZZ_REPORT_STRUCT* report, const uint8_t* bytes, size_t len)
{
    ZOO_SMB_MSG_STRUCT msg;
    char service_name[32];
    char address[32];
    char topic[32];
    char multicast_addr[32];
    char host_fingerprint[64];
    char shm_endpoint[64];
    uint16_t port = 0;
    uint16_t multicast_port = 0;
    uint32_t capabilities = 0;
    ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type = ZOO_SMB_TRANSPORT_TYPE_MIN;

    memset(&msg, 0, sizeof(msg));
    memset(service_name, 0, sizeof(service_name));
    memset(address, 0, sizeof(address));
    memset(topic, 0, sizeof(topic));
    memset(multicast_addr, 0, sizeof(multicast_addr));
    memset(host_fingerprint, 0, sizeof(host_fingerprint));
    memset(shm_endpoint, 0, sizeof(shm_endpoint));

    if (!bytes || len == 0)
    {
        return;
    }

    msg.payload = zoo_allocate_from_pool(len + 1u);
    if (!msg.payload)
    {
        return;
    }

    memcpy(msg.payload, bytes, len);
    msg.payload[len] = '\0';
    msg.header.payload_size = (uint32_t)len;

    (void)zoo_smb_protocol_parse_discovery_payload_message_ex2(
        &msg,
        service_name,
        address,
        &port,
        topic,
        &transport_type,
        multicast_addr,
        &multicast_port,
        host_fingerprint,
        &capabilities,
        shm_endpoint);
    zoo_free_to_pool(msg.payload);
    report->discovery_cases += 1u;
}

static void write_report(const FUZZ_REPORT_STRUCT* report)
{
    const char* artifact_dir = getenv("FUZZ_ARTIFACT_DIR");
    if (!artifact_dir || artifact_dir[0] == '\0')
    {
        return;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/fuzz_protocol_coverage.json", artifact_dir);

    FILE* fp = fopen(path, "w");
    if (!fp)
    {
        return;
    }

    fprintf(fp,
            "{\n"
            "  \"target\": \"fuzz_zoo_smb_protocol\",\n"
            "  \"executed\": true,\n"
            "  \"entrypoints\": [\"zoo_smb_protocol_is_header_valid\", \"zoo_smb_protocol_is_header_valid_with_msg_type\", \"zoo_smb_protocol_swap_header\", \"zoo_smb_protocol_deserialize\", \"zoo_smb_protocol_serialize\", \"zoo_smb_protocol_parse_discovery_payload_message_ex2\"],\n"
            "  \"seed_cases\": %u,\n"
            "  \"mutated_cases\": %u,\n"
            "  \"header_cases\": %u,\n"
            "  \"deserialize_cases\": %u,\n"
            "  \"serialize_cases\": %u,\n"
            "  \"discovery_cases\": %u\n"
            "}\n",
            report->seed_cases,
            report->mutated_cases,
            report->header_cases,
            report->deserialize_cases,
            report->serialize_cases,
            report->discovery_cases);
    fclose(fp);
}

int main(void)
{
    static const uint8_t seeds[][32] = {
        {0},
        {0xFF, 0xFF, 0xFF, 0xFF},
        {0x5A, 0x4F, 0x4F, 0x01, 0x00, 0x00, 0x00, 0x01},
        {0x01, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20}
    };
    FUZZ_REPORT_STRUCT report;
    uint32_t rng_state = 0x12345678u;
    uint8_t mutated[128];

    memset(&report, 0, sizeof(report));
    (void)zoo_create_memory_pool(4u * 1024u * 1024u);

    for (size_t i = 0; i < sizeof(seeds) / sizeof(seeds[0]); ++i)
    {
        fuzz_header_case(&report, seeds[i], sizeof(seeds[i]));
        fuzz_deserialize_case(&report, seeds[i], sizeof(seeds[i]));
        fuzz_discovery_case(&report, seeds[i], sizeof(seeds[i]));
        report.seed_cases += 1u;
    }

    for (size_t i = 0; i < 256u; ++i)
    {
        for (size_t j = 0; j < sizeof(mutated); ++j)
        {
            mutated[j] = (uint8_t)(fuzz_next(&rng_state) & 0xFFu);
        }
        fuzz_header_case(&report, mutated, sizeof(mutated));
        fuzz_deserialize_case(&report, mutated, sizeof(mutated));
        fuzz_discovery_case(&report, mutated, sizeof(mutated));
        report.mutated_cases += 1u;
    }

    for (size_t i = 0; i < 32u; ++i)
    {
        fuzz_roundtrip_case(&report);
        {
            const char* discovery_payload =
                "svc_meta topic1 127.0.0.1:1234 3 239.1.1.1:5678 hf=127.0.0.1 cap=3 shm=svc_meta";
            fuzz_discovery_case(&report,
                                (const uint8_t*)discovery_payload,
                                strlen(discovery_payload));
        }
    }

    write_report(&report);
    printf("fuzz_zoo_smb_protocol seeds=%u mutated=%u header=%u deserialize=%u serialize=%u discovery=%u\n",
           report.seed_cases,
           report.mutated_cases,
           report.header_cases,
           report.deserialize_cases,
           report.serialize_cases,
           report.discovery_cases);

    fflush(NULL);
    _exit(0);
}