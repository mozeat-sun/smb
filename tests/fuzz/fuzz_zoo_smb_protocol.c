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
            "  \"entrypoints\": [\"zoo_smb_protocol_is_header_valid\", \"zoo_smb_protocol_is_header_valid_with_msg_type\", \"zoo_smb_protocol_swap_header\", \"zoo_smb_protocol_deserialize\", \"zoo_smb_protocol_serialize\"],\n"
            "  \"seed_cases\": %u,\n"
            "  \"mutated_cases\": %u,\n"
            "  \"header_cases\": %u,\n"
            "  \"deserialize_cases\": %u,\n"
            "  \"serialize_cases\": %u\n"
            "}\n",
            report->seed_cases,
            report->mutated_cases,
            report->header_cases,
            report->deserialize_cases,
            report->serialize_cases);
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
        report.mutated_cases += 1u;
    }

    for (size_t i = 0; i < 32u; ++i)
    {
        fuzz_roundtrip_case(&report);
    }

    write_report(&report);
    printf("fuzz_zoo_smb_protocol seeds=%u mutated=%u header=%u deserialize=%u serialize=%u\n",
           report.seed_cases,
           report.mutated_cases,
           report.header_cases,
           report.deserialize_cases,
           report.serialize_cases);

    fflush(NULL);
    _exit(0);
}