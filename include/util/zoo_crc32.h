/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: utility
 * Component id: CRC32
 * File name: zoo_crc32.h
 * Description: CRC32 checksum calculation for ZOO
 *              Provides functions to calculate CRC32 checksums for data buffers.
 *              Used for data integrity verification in message transmission.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-07-14     weiwang.sun         created
 ******************************************************************************/
#ifndef ZOO_CRC32_H
#define ZOO_CRC32_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "zoo.h"

    /**
     * @brief Calculate CRC32 checksum for the given data.
     * @param data Pointer to the data.
     * @param size Size of the data in bytes.
     * @return CRC32 checksum.
     */
    ZOO_UINT32 zoo_calc_crc32(const void* data, ZOO_SIZE_T size);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_CRC32_H */