/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: zoo_smb_ring_buffer
 * File name: zoo_smb_ring_buffer.c
 * Description: High-performance ring buffer implementation for ZOO SMB
 * Features:
 * - Lock-free single producer/single consumer
 * - Memory alignment for cache efficiency
 * - Zero-copy operations where possible
 * - Variable-length message support
 * - Memory barrier support for multi-core systems
 * - Overflow handling strategies
 * - Optimized for shared memory transport
 *
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-06-16     weiwang.sun       created from ring_queue
 * 2.0       2025-06-23     weiwang.sun    Optimized for SHM transport
 * 2.1       2025-06-23     weiwang.sun    Unified with SMB protocol and memory pool
 ******************************************************************************/

#include "zoo_smb_ring_buffer.h"
#include "zoo_log.h"
#include "zoo_util.h"
#include "zoo_memory_pool.h"
#include "zoo_smb_protocol.h"
#include "zoo_smb_types.h"
#include "zoo_crc32.h"
#include <string.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>

/* ========== Internal Macros ========== */

/**
 * @brief Align size to ring buffer alignment boundary
 * @param size Size to align
 * @return Aligned size
 */
#define RING_ALIGN(size) (((size) + ZOO_SMB_RING_BUFFER_ALIGNMENT - 1) & ~(ZOO_SMB_RING_BUFFER_ALIGNMENT - 1))

/* ========== Internal Data Structures ========== */

/**
 * @brief Ring buffer control structure
 * @details Located at the beginning of shared memory segment
 *          Uses atomic operations for thread-safe access
 *          Aligned to cache line boundary for optimal performance
 */
typedef struct __attribute__((aligned(64)))
{
    /* Header information */
    uint32_t magic;      /**< Ring buffer magic number for validation */
    uint32_t version;    /**< Version number for compatibility checking */
    uint32_t total_size; /**< Total buffer size including control structure */
    uint32_t data_size;  /**< Data area size in bytes */

    /* Control variables - using atomic operations for thread safety */
    atomic_uint_fast32_t write_pos; /**< Current write position in buffer */
    atomic_uint_fast32_t read_pos;  /**< Current read position in buffer */
    atomic_uint_fast32_t write_seq; /**< Write sequence number for ordering */
    atomic_uint_fast32_t read_seq;  /**< Read sequence number for ordering */

    /* State flags */
    atomic_bool initialized; /**< Buffer initialization completion flag */
    atomic_bool corrupted;   /**< Buffer corruption detection flag */

    /* Statistics counters */
    atomic_uint_fast64_t total_writes; /**< Total write operations performed */
    atomic_uint_fast64_t total_reads;  /**< Total read operations performed */
    atomic_uint_fast64_t write_bytes;  /**< Total bytes written to buffer */
    atomic_uint_fast64_t read_bytes;   /**< Total bytes read from buffer */
    atomic_uint_fast64_t overruns;     /**< Buffer overrun event count */
    atomic_uint_fast64_t underruns;    /**< Buffer underrun event count */

    /* Synchronization counters */
    atomic_uint_fast32_t active_writers; /**< Number of active writers */
    atomic_uint_fast32_t active_readers; /**< Number of active readers */

    /* Configuration options */
    ZOO_BOOL allow_overwrite;  /**< Allow overwriting unread data when buffer is full */
    uint32_t reserved[15]; /**< Reserved space for future extensions */

} ZOO_SMB_RING_BUFFER_CONTROL;

/**
 * @brief Ring buffer handle structure
 * @details Used by processes to access and manage the ring buffer
 *          Contains both shared control structure and local state
 */
typedef struct ZOO_SMB_RING_BUFFER_STRUCT
{
    ZOO_SMB_RING_BUFFER_CONTROL* control; /**< Pointer to shared control structure */
    uint8_t* data;                        /**< Pointer to ring buffer data area */
    uint32_t data_size;                   /**< Size of data area in bytes */
    ZOO_BOOL is_owner;                        /**< True if this process created the buffer */
    ZOO_BOOL is_attached;                     /**< True if successfully attached to buffer */

    /* Local state tracking */
    uint32_t last_read_pos; /**< Last read position cached by this handle */
    uint32_t last_read_seq; /**< Last read sequence cached by this handle */

    /* Memory management information */
    void* memory_base;  /**< Base memory pointer for cleanup operations */
    size_t memory_size; /**< Total memory size allocated */
    ZOO_BOOL owns_memory;   /**< True if this handle owns and should free memory */

} ZOO_SMB_RING_BUFFER_STRUCT;

/**
 * @brief Ring buffer comprehensive statistics structure
 * @details Provides detailed performance and utilization metrics
 */
typedef struct
{
    uint64_t total_writes;    /**< Total write operations completed */
    uint64_t total_reads;     /**< Total read operations completed */
    uint64_t write_bytes;     /**< Total bytes written successfully */
    uint64_t read_bytes;      /**< Total bytes read successfully */
    uint64_t overruns;        /**< Number of buffer overrun events */
    uint64_t underruns;       /**< Number of buffer underrun events */
    uint32_t active_writers;  /**< Current number of active writers */
    uint32_t active_readers;  /**< Current number of active readers */
    uint32_t available_space; /**< Available space for writing in bytes */
    uint32_t used_space;      /**< Currently used space in bytes */
    float utilization;        /**< Buffer utilization as percentage (0.0-100.0) */
    ZOO_BOOL is_full;             /**< True if buffer is at capacity */
    ZOO_BOOL is_empty;            /**< True if buffer contains no data */
    ZOO_BOOL is_corrupted;        /**< True if buffer corruption detected */
} ZOO_SMB_RING_BUFFER_STATISTICS_STRUCT;

/* ========== Internal Helper Functions ========== */

/**
 * @brief Calculate the next position in a circular ring buffer
 * @param pos Current position in the buffer
 * @param increment Number of bytes to advance
 * @param size Total size of the buffer
 * @return Next position after wrapping around if necessary
 * @note Handles wrap-around automatically for circular buffer behavior
 */
static inline uint32_t ring_next_pos(uint32_t pos, uint32_t increment, uint32_t size)
{
    return (pos + increment) % size;
}

/**
 * @brief Calculate available space between write and read positions
 * @param write_pos Current write position in buffer
 * @param read_pos Current read position in buffer
 * @param size Total buffer size in bytes
 * @return Number of bytes available for writing
 * @note Returns (size - 1) maximum to distinguish between full and empty states
 */
static inline uint32_t ring_space(uint32_t write_pos, uint32_t read_pos, uint32_t size)
{
    if (write_pos >= read_pos)
    {
        return size - (write_pos - read_pos) - 1;
    }
    else
    {
        return read_pos - write_pos - 1;
    }
}

/**
 * @brief Calculate used space between write and read positions
 * @param write_pos Current write position in buffer
 * @param read_pos Current read position in buffer
 * @param size Total buffer size in bytes
 * @return Number of bytes currently used in the buffer
 * @note Handles both wrapped and non-wrapped buffer states
 */
static inline uint32_t ring_used(uint32_t write_pos, uint32_t read_pos, uint32_t size)
{
    if (write_pos >= read_pos)
    {
        return write_pos - read_pos;
    }
    else
    {
        return size - (read_pos - write_pos);
    }
}

/**
 * @brief Execute a full memory barrier for cross-core synchronization
 * @note Ensures all memory operations are visible across all threads and cores
 *       Uses sequential consistency for maximum safety
 */
static inline void memory_barrier(void)
{
    atomic_thread_fence(memory_order_seq_cst);
}

/**
 * @brief Load buffer positions atomically for consistent state reading
 * @param buffer Ring buffer handle
 * @param write_pos Output pointer for write position
 * @param read_pos Output pointer for read position
 * @note Uses acquire semantics to ensure visibility of all prior writes
 */
static inline void load_buffer_positions(ZOO_SMB_RING_BUFFER_HANDLE buffer, 
                                        uint32_t* write_pos, 
                                        uint32_t* read_pos)
{
    *write_pos = atomic_load_explicit(&buffer->control->write_pos, memory_order_acquire);
    *read_pos = atomic_load_explicit(&buffer->control->read_pos, memory_order_acquire);
}

/**
 * @brief Validate buffer handle for safe operations
 * @param buffer Ring buffer handle to validate
 * @return ZOO_SMB_OK if valid, appropriate error code otherwise
 * @note Performs comprehensive validation including null checks and corruption detection
 */
static inline ZOO_ERROR_TYPE validate_buffer_for_operation(ZOO_SMB_RING_BUFFER_HANDLE buffer)
{
    ZOO_SMB_VALIDATE_PTR(buffer, ZOO_SMB_ERROR_INVALID_PARAM);
    
    if (!zoo_smb_ring_buffer_is_valid(buffer))
    {
        return ZOO_SMB_ERROR_RINGBUF_CORRUPTED;
    }
    
    return ZOO_SMB_OK;
}

/**
 * @brief Update write operation statistics efficiently
 * @param buffer Ring buffer handle
 * @param bytes_written Number of bytes written in this operation
 * @note Uses relaxed memory ordering for performance, suitable for statistics
 */
static inline void update_write_statistics(ZOO_SMB_RING_BUFFER_HANDLE buffer, size_t bytes_written)
{
    atomic_fetch_add_explicit(&buffer->control->total_writes, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&buffer->control->write_bytes, bytes_written, memory_order_relaxed);
    atomic_fetch_sub_explicit(&buffer->control->active_writers, 1, memory_order_relaxed);
}

/**
 * @brief Update read operation statistics efficiently
 * @param buffer Ring buffer handle
 * @param bytes_read Number of bytes read in this operation
 * @note Uses relaxed memory ordering for performance, suitable for statistics
 */
static inline void update_read_statistics(ZOO_SMB_RING_BUFFER_HANDLE buffer, size_t bytes_read)
{
    atomic_fetch_add_explicit(&buffer->control->total_reads, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&buffer->control->read_bytes, bytes_read, memory_order_relaxed);
}

/**
 * @brief Create a standardized ring buffer message header from payload data
 * @param data Pointer to message payload data
 * @param size Size of the payload data in bytes
 * @param sequence Message sequence number for ordering
 * @return Fully initialized message header structure with CRC32 checksum
 * @note Uses SMB protocol header format for compatibility
 */
static ZOO_SMB_MSG_HEADER_STRUCT create_ring_header(const void* data, size_t size, uint32_t sequence)
{
    ZOO_SMB_MSG_HEADER_STRUCT header = {0};

    header.magic = ZOO_SMB_MSG_MAGIC_NUMBER;
    header.version = ZOO_SMB_PROTOCOL_VERSION;
    header.msg_type = ZOO_SMB_MSG_TYPE_PUB;  // Default to publish message type
    header.flags = 0;
    header.payload_size = size;
    header.sequence = sequence;
    header.timestamp = time(NULL);
    header.crc32 = zoo_calc_crc32(data, size);

    return header;
}

/**
 * @brief Validate ring buffer message header integrity and format
 * @param header Pointer to message header to validate
 * @return ZOO_TRUE if header is valid and safe to process, ZOO_FALSE otherwise
 * @note Checks magic number and protocol version for compatibility
 */
static ZOO_BOOL validate_ring_header(const ZOO_SMB_MSG_HEADER_STRUCT* header)
{
    return (header->magic == ZOO_SMB_MSG_MAGIC_NUMBER &&
            header->version == ZOO_SMB_PROTOCOL_VERSION);
}

/* ========== Public API Implementation ========== */

/**
 * @brief Calculate total memory size required for a ring buffer with given data size
 * @param data_size Desired data area size in bytes
 * @return Total memory size needed including control structure and alignment padding
 * @note Automatically clamps data_size to valid range [MIN_SIZE, MAX_SIZE]
 *       Applies cache-line alignment for optimal performance
 * @see ZOO_SMB_RING_BUFFER_MIN_SIZE, ZOO_SMB_RING_BUFFER_MAX_SIZE
 */
size_t zoo_smb_ring_buffer_get_required_size(size_t data_size)
{
    // Clamp data size to valid range
    if (data_size < ZOO_SMB_RING_BUFFER_MIN_SIZE)
    {
        data_size = ZOO_SMB_RING_BUFFER_MIN_SIZE;
    }

    if (data_size > ZOO_SMB_RING_BUFFER_MAX_SIZE)
    {
        data_size = ZOO_SMB_RING_BUFFER_MAX_SIZE;
    }

    // Apply alignment for cache efficiency
    data_size = RING_ALIGN(data_size);
    size_t control_size = RING_ALIGN(sizeof(ZOO_SMB_RING_BUFFER_CONTROL));

    return control_size + data_size;
}

/**
 * @brief Create a new ring buffer in pre-allocated memory region
 * @param memory Pointer to memory area where ring buffer will be created
 * @param memory_size Total size of available memory in bytes
 * @param allow_overwrite If ZOO_TRUE, allows overwriting unread data when buffer is full
 * @return Ring buffer handle on success, NULL on failure
 * @note The provided memory must remain valid for the entire lifetime of the ring buffer
 *       Memory layout: [Control Structure][Alignment Padding][Data Area]
 * @warning Caller is responsible for memory management and synchronization
 */
ZOO_SMB_RING_BUFFER_HANDLE zoo_smb_ring_buffer_create_in_memory(void* memory,
                                                                size_t memory_size,
                                                                ZOO_BOOL allow_overwrite)
{
    // Validate input parameters
    if (!memory || memory_size < sizeof(ZOO_SMB_RING_BUFFER_CONTROL) + ZOO_SMB_RING_BUFFER_MIN_SIZE)
    {
        ZOO_LOG_ERROR("Invalid parameters for ring buffer creation: memory=%p, size=%zu", 
                          memory, memory_size);
        return NULL;
    }

    // Allocate handle structure from memory pool
    ZOO_SMB_RING_BUFFER_STRUCT* rb = (ZOO_SMB_RING_BUFFER_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_RING_BUFFER_STRUCT));
    if (!rb)
    {
        ZOO_LOG_ERROR("Failed to allocate ring buffer handle from memory pool");
        return NULL;
    }

    // Initialize handle structure
    memset(rb, 0, sizeof(ZOO_SMB_RING_BUFFER_STRUCT));

    // Calculate memory layout
    size_t control_size = RING_ALIGN(sizeof(ZOO_SMB_RING_BUFFER_CONTROL));
    rb->control = (ZOO_SMB_RING_BUFFER_CONTROL*)memory;
    rb->data = (uint8_t*)memory + control_size;
    rb->data_size = memory_size - control_size;
    rb->is_owner = ZOO_TRUE;
    rb->is_attached = ZOO_TRUE;
    rb->memory_base = memory;
    rb->memory_size = memory_size;
    rb->owns_memory = ZOO_FALSE;  // Memory provided by caller

    // Initialize control structure with zeros
    memset(rb->control, 0, sizeof(ZOO_SMB_RING_BUFFER_CONTROL));
    
    // Set header information
    rb->control->magic = ZOO_SMB_RING_BUFFER_MAGIC;
    rb->control->version = ZOO_SMB_RING_BUFFER_VERSION;
    rb->control->total_size = memory_size;
    rb->control->data_size = rb->data_size;
    rb->control->allow_overwrite = allow_overwrite;

    // Initialize atomic variables (zeros already set by memset)
    atomic_init(&rb->control->write_pos, 0);
    atomic_init(&rb->control->read_pos, 0);
    atomic_init(&rb->control->write_seq, 0);
    atomic_init(&rb->control->read_seq, 0);
    atomic_init(&rb->control->initialized, ZOO_FALSE);
    atomic_init(&rb->control->corrupted, ZOO_FALSE);
    atomic_init(&rb->control->total_writes, 0);
    atomic_init(&rb->control->total_reads, 0);
    atomic_init(&rb->control->write_bytes, 0);
    atomic_init(&rb->control->read_bytes, 0);
    atomic_init(&rb->control->overruns, 0);
    atomic_init(&rb->control->underruns, 0);
    atomic_init(&rb->control->active_writers, 0);
    atomic_init(&rb->control->active_readers, 0);

    // Apply memory barrier and mark as initialized
    memory_barrier();
    atomic_store(&rb->control->initialized, ZOO_TRUE);

    ZOO_LOG_DEBUG("Created ring buffer in memory: addr=%p, total_size=%zu, data_size=%u",
                      memory, memory_size, rb->data_size);

    return rb;
}

/**
 * @brief Attach to an existing ring buffer in shared memory
 * @param memory Pointer to memory containing a valid ring buffer
 * @return Ring buffer handle on success, NULL on failure
 * @note The existing ring buffer must be properly initialized and uncorrupted
 *       This function performs comprehensive validation before attachment
 * @warning Multiple processes can attach to the same buffer for IPC
 */
ZOO_SMB_RING_BUFFER_HANDLE zoo_smb_ring_buffer_attach(void* memory)
{
    ZOO_SMB_VALIDATE_PTR(memory, NULL);

    ZOO_SMB_RING_BUFFER_CONTROL* control = (ZOO_SMB_RING_BUFFER_CONTROL*)memory;

    // Validate magic number
    if (control->magic != ZOO_SMB_RING_BUFFER_MAGIC)
    {
        ZOO_LOG_ERROR("Invalid ring buffer magic: 0x%x, expected: 0x%x", 
                          control->magic, ZOO_SMB_RING_BUFFER_MAGIC);
        return NULL;
    }

    // Validate version compatibility
    if (control->version != ZOO_SMB_RING_BUFFER_VERSION)
    {
        ZOO_LOG_ERROR("Ring buffer version mismatch: %d, expected: %d",
                          control->version, ZOO_SMB_RING_BUFFER_VERSION);
        return NULL;
    }

    // Check initialization status
    if (!atomic_load(&control->initialized))
    {
        ZOO_LOG_ERROR("Ring buffer not initialized");
        return NULL;
    }

    // Check corruption status
    if (atomic_load(&control->corrupted))
    {
        ZOO_LOG_ERROR("Ring buffer is corrupted");
        return NULL;
    }

    // Allocate handle structure from memory pool
    ZOO_SMB_RING_BUFFER_STRUCT* rb = (ZOO_SMB_RING_BUFFER_STRUCT*)zoo_allocate_from_pool(sizeof(ZOO_SMB_RING_BUFFER_STRUCT));
    if (!rb)
    {
        ZOO_LOG_ERROR("Failed to allocate ring buffer handle from memory pool");
        return NULL;
    }

    // Initialize handle structure
    memset(rb, 0, sizeof(ZOO_SMB_RING_BUFFER_STRUCT));

    // Setup handle for attachment
    size_t control_size = RING_ALIGN(sizeof(ZOO_SMB_RING_BUFFER_CONTROL));
    rb->control = control;
    rb->data = (uint8_t*)memory + control_size;
    rb->data_size = control->data_size;
    rb->is_owner = ZOO_FALSE;
    rb->is_attached = ZOO_TRUE;
    rb->memory_base = memory;
    rb->memory_size = control->total_size;
    rb->owns_memory = ZOO_FALSE;
    
    // Cache current positions for this handle
    rb->last_read_pos = atomic_load(&control->read_pos);
    rb->last_read_seq = atomic_load(&control->read_seq);

    ZOO_LOG_DEBUG("Attached to ring buffer: addr=%p, data_size=%u", memory, rb->data_size);

    return rb;
}

/**
 * @brief Destroy a ring buffer and free all associated resources
 * @param buffer Ring buffer handle to destroy
 * @note If this handle is the owner, marks buffer as corrupted to prevent further use
 *       Frees memory only if this handle allocated it
 *       Safe to call with NULL handle
 */
void zoo_smb_ring_buffer_destroy(ZOO_SMB_RING_BUFFER_HANDLE buffer)
{
    if (!buffer)
        return;

    // Mark buffer as corrupted if we are the owner
    if (buffer->is_owner && buffer->control)
    {
        atomic_store(&buffer->control->initialized, ZOO_FALSE);
        atomic_store(&buffer->control->corrupted, ZOO_TRUE);
        ZOO_LOG_DEBUG("Ring buffer marked as corrupted by owner");
    }

    // Free memory if we own it
    if (buffer->owns_memory && buffer->memory_base)
    {
        zoo_free_to_pool(buffer->memory_base);
        ZOO_LOG_DEBUG("Ring buffer memory freed");
    }

    // Free handle structure
    zoo_free_to_pool(buffer);

    ZOO_LOG_DEBUG("Ring buffer handle destroyed");
}

/**
 * @brief Write data to the ring buffer with automatic message framing
 * @param buffer Ring buffer handle
 * @param data Pointer to data to write
 * @param size Size of data in bytes
 * @return ZOO_SMB_OK on success, appropriate error code on failure
 * @note Thread-safe for single producer scenario
 *       Automatically adds SMB protocol header with CRC32 checksum
 *       Handles wrap-around and buffer full conditions
 * @warning Does not support multiple concurrent writers
 */
ZOO_ERROR_TYPE zoo_smb_ring_buffer_write(ZOO_SMB_RING_BUFFER_HANDLE buffer,
                                             const void* data,
                                             size_t size)
{
    // 参数验证
    ZOO_ERROR_TYPE result = validate_buffer_for_operation(buffer);
    if (ZOO_SMB_IS_ERROR(result))
    {
        return result;
    }
    
    ZOO_SMB_VALIDATE_PTR(data, ZOO_SMB_ERROR_INVALID_PARAM);

    // 验证数据大小
    if (size == 0 || size > buffer->data_size - ZOO_SMB_RING_MSG_HEADER_SIZE)
    {
        ZOO_LOG_ERROR("Invalid data size: %zu, max allowed: %u", 
                          size, buffer->data_size - ZOO_SMB_RING_MSG_HEADER_SIZE);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    uint32_t total_size = ZOO_SMB_RING_MSG_HEADER_SIZE + size;
    
    // 增加活跃写者计数
    atomic_fetch_add_explicit(&buffer->control->active_writers, 1, memory_order_relaxed);

    // 加载当前位置
    uint32_t write_pos = atomic_load_explicit(&buffer->control->write_pos, memory_order_acquire);
    uint32_t read_pos = atomic_load_explicit(&buffer->control->read_pos, memory_order_acquire);

    // 验证位置合理性
    if (write_pos >= buffer->data_size || read_pos >= buffer->data_size)
    {
        ZOO_LOG_ERROR("Invalid buffer positions before write: write_pos=%u, read_pos=%u, data_size=%u", 
                          write_pos, read_pos, buffer->data_size);
        atomic_fetch_sub_explicit(&buffer->control->active_writers, 1, memory_order_relaxed);
        atomic_store_explicit(&buffer->control->corrupted, ZOO_TRUE, memory_order_release);
        return ZOO_SMB_ERROR_RINGBUF_CORRUPTED;
    }

    // 检查可用空间
    uint32_t available = ring_space(write_pos, read_pos, buffer->data_size);
    
    if (available < total_size)
    {
        if (!buffer->control->allow_overwrite)
        {
            ZOO_LOG_WARN("Ring buffer full: available=%u, needed=%u", available, total_size);
            atomic_fetch_sub_explicit(&buffer->control->active_writers, 1, memory_order_relaxed);
            return ZOO_SMB_ERROR_RINGBUF_FULL;
        }

        // 关键修复：覆盖写入时同步更新读位置
        uint32_t bytes_to_discard = total_size - available;
        uint32_t messages_discarded = 0;
        uint32_t new_read_pos = read_pos;

        // 安全地跳过被覆盖的消息
        while (bytes_to_discard > 0 && messages_discarded < 1000) // 防止无限循环
        {
            // 检查当前位置是否有有效的消息头
            if (ring_used(write_pos, new_read_pos, buffer->data_size) < sizeof(ZOO_SMB_MSG_HEADER_STRUCT))
            {
                // 没有完整的消息头，直接跳过剩余字节
                new_read_pos = ring_next_pos(new_read_pos, bytes_to_discard, buffer->data_size);
                break;
            }

            // 尝试读取消息头以确定消息大小
            ZOO_SMB_MSG_HEADER_STRUCT peek_header;
            uint32_t peek_pos = new_read_pos;
            ZOO_BOOL header_valid = ZOO_TRUE;

            // 安全地读取消息头
            for (size_t i = 0; i < sizeof(ZOO_SMB_MSG_HEADER_STRUCT); i++)
            {
                if (peek_pos >= buffer->data_size)
                {
                    header_valid = ZOO_FALSE;
                    break;
                }
                ((uint8_t*)&peek_header)[i] = buffer->data[peek_pos];
                peek_pos = (peek_pos + 1) % buffer->data_size;
            }

            if (!header_valid || !validate_ring_header(&peek_header))
            {
                // 消息头无效，跳过一个字节继续寻找
                new_read_pos = ring_next_pos(new_read_pos, 1, buffer->data_size);
                bytes_to_discard = (bytes_to_discard > 1) ? bytes_to_discard - 1 : 0;
                continue;
            }

            // 计算完整消息大小
            uint32_t msg_total_size = sizeof(ZOO_SMB_MSG_HEADER_STRUCT) + peek_header.payload_size;
            
            // 验证消息大小合理性
            if (peek_header.payload_size > buffer->data_size || msg_total_size > buffer->data_size)
            {
                ZOO_LOG_WARN("Invalid message size during overwrite: payload=%u, skipping 1 byte", 
                                 peek_header.payload_size);
                new_read_pos = ring_next_pos(new_read_pos, 1, buffer->data_size);
                bytes_to_discard = (bytes_to_discard > 1) ? bytes_to_discard - 1 : 0;
                continue;
            }

            // 跳过整个消息
            new_read_pos = ring_next_pos(new_read_pos, msg_total_size, buffer->data_size);
            bytes_to_discard = (bytes_to_discard > msg_total_size) ? bytes_to_discard - msg_total_size : 0;
            messages_discarded++;

            ZOO_LOG_DEBUG("Discarded message during overwrite: size=%u, sequence=%u, remaining=%u", 
                              msg_total_size, peek_header.sequence, bytes_to_discard);
        }

        // 原子地更新读位置
        atomic_store_explicit(&buffer->control->read_pos, new_read_pos, memory_order_release);
        
        // 更新统计信息
        atomic_fetch_add_explicit(&buffer->control->overruns, 1, memory_order_relaxed);
        
        ZOO_LOG_WARN("Ring buffer overwrite: discarded %u messages, old_read_pos=%u, new_read_pos=%u", 
                         messages_discarded, read_pos, new_read_pos);

        // 更新读位置以供后续使用
        read_pos = new_read_pos;
    }

    // 获取序列号（仅在即将成功写入时）
    uint32_t sequence = atomic_load_explicit(&buffer->control->write_seq, memory_order_relaxed);
    ZOO_SMB_MSG_HEADER_STRUCT header = create_ring_header(data, size, sequence);

    // 验证写入位置不会越界
    if (write_pos >= buffer->data_size)
    {
        ZOO_LOG_ERROR("Write position out of bounds: %u >= %u", write_pos, buffer->data_size);
        atomic_fetch_sub_explicit(&buffer->control->active_writers, 1, memory_order_relaxed);
        atomic_store_explicit(&buffer->control->corrupted, ZOO_TRUE, memory_order_release);
        return ZOO_SMB_ERROR_RINGBUF_CORRUPTED;
    }

    // 写入消息头（处理环形包装）
    if (write_pos + sizeof(ZOO_SMB_MSG_HEADER_STRUCT) <= buffer->data_size)
    {
        memcpy(buffer->data + write_pos, &header, sizeof(ZOO_SMB_MSG_HEADER_STRUCT));
    }
    else
    {
        uint32_t first_part = buffer->data_size - write_pos;
        memcpy(buffer->data + write_pos, &header, first_part);
        memcpy(buffer->data, (uint8_t*)&header + first_part, sizeof(ZOO_SMB_MSG_HEADER_STRUCT) - first_part);
    }

    uint32_t new_write_pos = ring_next_pos(write_pos, sizeof(ZOO_SMB_MSG_HEADER_STRUCT), buffer->data_size);

    // 写入载荷数据（处理环形包装）
    if (size > 0)
    {
        if (new_write_pos + size <= buffer->data_size)
        {
            memcpy(buffer->data + new_write_pos, data, size);
        }
        else
        {
            uint32_t first_part = buffer->data_size - new_write_pos;
            memcpy(buffer->data + new_write_pos, data, first_part);
            memcpy(buffer->data, (uint8_t*)data + first_part, size - first_part);
        }
    }

    new_write_pos = ring_next_pos(new_write_pos, size, buffer->data_size);

    // 验证新写位置的合理性
    if (new_write_pos >= buffer->data_size)
    {
        ZOO_LOG_ERROR("Calculated write position out of bounds: %u >= %u", new_write_pos, buffer->data_size);
        atomic_fetch_sub_explicit(&buffer->control->active_writers, 1, memory_order_relaxed);
        atomic_store_explicit(&buffer->control->corrupted, ZOO_TRUE, memory_order_release);
        return ZOO_SMB_ERROR_RINGBUF_CORRUPTED;
    }

    // 原子地更新写位置
    atomic_store_explicit(&buffer->control->write_pos, new_write_pos, memory_order_release);

    // 仅在成功写入后才递增序列号
    atomic_fetch_add_explicit(&buffer->control->write_seq, 1, memory_order_relaxed);

    // 确保所有写操作对其他线程可见
    atomic_thread_fence(memory_order_seq_cst);

    // 更新统计信息
    atomic_fetch_add_explicit(&buffer->control->total_writes, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&buffer->control->write_bytes, size, memory_order_relaxed);
    atomic_fetch_sub_explicit(&buffer->control->active_writers, 1, memory_order_relaxed);

    ZOO_LOG_TRACE("Ring buffer write completed: size=%zu, sequence=%u, new_pos=%u", 
                      size, sequence, new_write_pos);

    return ZOO_SMB_OK;
}

/**
 * @brief Read message payload from ring buffer, automatically skipping message header
 * @param buffer Ring buffer handle
 * @param data Output buffer for payload data
 * @param expected_size Expected payload size in bytes
 * @return ZOO_SMB_OK on success, appropriate error code on failure
 * @note Thread-safe for single consumer scenario
 *       Automatically validates message header and CRC32 checksum
 *       Advances read position only after successful validation
 * @warning Does not support multiple concurrent readers
 */
ZOO_ERROR_TYPE zoo_smb_ring_buffer_read(ZOO_SMB_RING_BUFFER_HANDLE buffer,
                                            void* data,
                                            size_t expected_size)
{
    // 参数验证
    ZOO_ERROR_TYPE result = validate_buffer_for_operation(buffer);
    if (ZOO_SMB_IS_ERROR(result))
    {
        return result;
    }
    
    ZOO_SMB_VALIDATE_PTR(data, ZOO_SMB_ERROR_INVALID_PARAM);

    // 增加活跃读者计数
    atomic_fetch_add_explicit(&buffer->control->active_readers, 1, memory_order_relaxed);

    // 加载当前位置
    uint32_t write_pos = atomic_load_explicit(&buffer->control->write_pos, memory_order_acquire);
    uint32_t read_pos = atomic_load_explicit(&buffer->control->read_pos, memory_order_acquire);

    // 验证位置合理性
    if (write_pos >= buffer->data_size || read_pos >= buffer->data_size)
    {
        ZOO_LOG_ERROR("Invalid buffer positions during read: write_pos=%u, read_pos=%u, data_size=%u", 
                          write_pos, read_pos, buffer->data_size);
        atomic_fetch_sub_explicit(&buffer->control->active_readers, 1, memory_order_relaxed);
        atomic_store_explicit(&buffer->control->corrupted, ZOO_TRUE, memory_order_release);
        return ZOO_SMB_ERROR_RINGBUF_CORRUPTED;
    }

    uint32_t used_bytes = ring_used(write_pos, read_pos, buffer->data_size);

    // 检查是否有足够数据读取消息头
    if (used_bytes < sizeof(ZOO_SMB_MSG_HEADER_STRUCT))
    {
        ZOO_LOG_TRACE("Not enough data for message header: used=%u, needed=%zu", 
                          used_bytes, sizeof(ZOO_SMB_MSG_HEADER_STRUCT));
        atomic_fetch_sub_explicit(&buffer->control->active_readers, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&buffer->control->underruns, 1, memory_order_relaxed);
        return ZOO_SMB_ERROR_RINGBUF_EMPTY;
    }

    // 安全地读取消息头
    ZOO_SMB_MSG_HEADER_STRUCT ring_header;
    memset(&ring_header, 0, sizeof(ring_header)); // 清零防止垃圾数据
    uint32_t current_pos = read_pos;
    
    // 逐字节读取消息头以处理环形包装
    for (size_t i = 0; i < sizeof(ZOO_SMB_MSG_HEADER_STRUCT); i++)
    {
        if (current_pos >= buffer->data_size)
        {
            ZOO_LOG_ERROR("Position overflow during header read: pos=%u, size=%u", 
                              current_pos, buffer->data_size);
            atomic_fetch_sub_explicit(&buffer->control->active_readers, 1, memory_order_relaxed);
            atomic_store_explicit(&buffer->control->corrupted, ZOO_TRUE, memory_order_release);
            return ZOO_SMB_ERROR_RINGBUF_CORRUPTED;
        }
        ((uint8_t*)&ring_header)[i] = buffer->data[current_pos];
        current_pos = (current_pos + 1) % buffer->data_size;
    }

    // 增强的消息头验证
    if (!validate_ring_header(&ring_header))
    {
        ZOO_LOG_ERROR("Invalid ring message header: magic=0x%x, version=%u, at position=%u", 
                          ring_header.magic, ring_header.version, read_pos);
        atomic_fetch_sub_explicit(&buffer->control->active_readers, 1, memory_order_relaxed);
        atomic_store_explicit(&buffer->control->corrupted, ZOO_TRUE, memory_order_release);
        return ZOO_SMB_ERROR_RINGBUF_CORRUPTED;
    }

    // 载荷大小合理性检查
    if (ring_header.payload_size > buffer->data_size - sizeof(ZOO_SMB_MSG_HEADER_STRUCT))
    {
        ZOO_LOG_ERROR("Invalid payload size in header: %u, max allowed: %u", 
                          ring_header.payload_size, 
                          buffer->data_size - (uint32_t)sizeof(ZOO_SMB_MSG_HEADER_STRUCT));
        atomic_fetch_sub_explicit(&buffer->control->active_readers, 1, memory_order_relaxed);
        atomic_store_explicit(&buffer->control->corrupted, ZOO_TRUE, memory_order_release);
        return ZOO_SMB_ERROR_RINGBUF_CORRUPTED;
    }

    // 验证载荷大小匹配期望
    if (ring_header.payload_size != expected_size)
    {
        ZOO_LOG_ERROR("Ring message size mismatch: expected=%zu, actual=%u, sequence=%u", 
                          expected_size, ring_header.payload_size, ring_header.sequence);
        atomic_fetch_sub_explicit(&buffer->control->active_readers, 1, memory_order_relaxed);
        return ZOO_SMB_ERROR_INVALID_SIZE;
    }

    // 检查是否有完整消息
    size_t total_msg_size = sizeof(ZOO_SMB_MSG_HEADER_STRUCT) + ring_header.payload_size;
    if (used_bytes < total_msg_size)
    {
        ZOO_LOG_ERROR("Incomplete ring message: used=%u, needed=%zu, sequence=%u", 
                          used_bytes, total_msg_size, ring_header.sequence);
        atomic_fetch_sub_explicit(&buffer->control->active_readers, 1, memory_order_relaxed);
        return ZOO_SMB_ERROR_INVALID_STATE;
    }

    // 安全地读取载荷数据
    if (ring_header.payload_size > 0)
    {
        if (current_pos + ring_header.payload_size <= buffer->data_size)
        {
            // 不需要包装的情况
            memcpy(data, buffer->data + current_pos, ring_header.payload_size);
        }
        else
        {
            // 需要包装的情况
            uint32_t first_part = buffer->data_size - current_pos;
            if (first_part > ring_header.payload_size)
            {
                ZOO_LOG_ERROR("Invalid wrap calculation: first_part=%u > payload=%u", 
                                  first_part, ring_header.payload_size);
                atomic_fetch_sub_explicit(&buffer->control->active_readers, 1, memory_order_relaxed);
                atomic_store_explicit(&buffer->control->corrupted, ZOO_TRUE, memory_order_release);
                return ZOO_SMB_ERROR_RINGBUF_CORRUPTED;
            }
            
            memcpy(data, buffer->data + current_pos, first_part);
            memcpy((uint8_t*)data + first_part, buffer->data, ring_header.payload_size - first_part);
        }

        current_pos = (current_pos + ring_header.payload_size) % buffer->data_size;
    }

    // CRC32校验
    uint32_t calculated_crc = zoo_calc_crc32(data, ring_header.payload_size);
    if (calculated_crc != ring_header.crc32)
    {
        ZOO_LOG_ERROR("Ring message CRC32 mismatch: calculated=0x%08X, expected=0x%08X, sequence=%u", 
                          calculated_crc, ring_header.crc32, ring_header.sequence);
        atomic_fetch_sub_explicit(&buffer->control->active_readers, 1, memory_order_relaxed);
        atomic_store_explicit(&buffer->control->corrupted, ZOO_TRUE, memory_order_release);
        return ZOO_SMB_ERROR_RINGBUF_CORRUPTED;
    }

    // 验证新的读取位置合理性
    if (current_pos >= buffer->data_size)
    {
        ZOO_LOG_ERROR("Invalid new read position: %u >= %u", current_pos, buffer->data_size);
        atomic_fetch_sub_explicit(&buffer->control->active_readers, 1, memory_order_relaxed);
        atomic_store_explicit(&buffer->control->corrupted, ZOO_TRUE, memory_order_release);
        return ZOO_SMB_ERROR_RINGBUF_CORRUPTED;
    }

    // 原子地更新读取位置
    atomic_store_explicit(&buffer->control->read_pos, current_pos, memory_order_release);

    // 确保读操作对其他线程可见
    atomic_thread_fence(memory_order_seq_cst);

    // 更新统计信息
    atomic_fetch_add_explicit(&buffer->control->total_reads, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&buffer->control->read_bytes, ring_header.payload_size, memory_order_relaxed);
    atomic_fetch_sub_explicit(&buffer->control->active_readers, 1, memory_order_relaxed);

    ZOO_LOG_TRACE("Ring buffer read completed: payload_size=%u, sequence=%u, new_pos=%u", 
                      ring_header.payload_size, ring_header.sequence, current_pos);

    return ZOO_SMB_OK;
}

/**
 * @brief Get the size of the next message payload without consuming it
 * @param buffer Ring buffer handle
 * @return Size of next message payload in bytes, 0 if no complete message, UINT32_MAX on error
 * @note Thread-safe for peek operations
 *       Does not advance read position
 *       Validates message header for integrity
 * @retval 0 No complete message available
 * @retval UINT32_MAX Corruption or validation error detected
 * @retval >0 Size of next message payload in bytes
 */
size_t zoo_smb_ring_buffer_get_next_message_size(ZOO_SMB_RING_BUFFER_HANDLE buffer)
{
    // Basic validation
    if (!buffer || !zoo_smb_ring_buffer_is_valid(buffer))
    {
        return 0;
    }

    // Load current positions with acquire semantics
    uint32_t write_pos = atomic_load_explicit(&buffer->control->write_pos, memory_order_acquire);
    uint32_t read_pos = atomic_load_explicit(&buffer->control->read_pos, memory_order_acquire);

    uint32_t used_bytes = ring_used(write_pos, read_pos, buffer->data_size);

    // Check if we have enough data for message header
    if (used_bytes < sizeof(ZOO_SMB_MSG_HEADER_STRUCT))
    {
        ZOO_LOG_TRACE("Not enough data for ring message header: used=%u", used_bytes);
        return 0;
    }

    // Peek at message header without advancing position
    ZOO_SMB_MSG_HEADER_STRUCT ring_header;
    uint32_t peek_pos = read_pos;
    
    // Read header bytes with wrap-around handling
    for (size_t i = 0; i < sizeof(ZOO_SMB_MSG_HEADER_STRUCT); i++)
    {
        ((uint8_t*)&ring_header)[i] = buffer->data[peek_pos];
        peek_pos = (peek_pos + 1) % buffer->data_size;
    }

    // Validate magic number
    if (ring_header.magic != ZOO_SMB_MSG_MAGIC_NUMBER)
    {
        ZOO_LOG_WARN("Invalid ring message magic: 0x%08x, expected: 0x%08x", 
                          ring_header.magic, ZOO_SMB_MSG_MAGIC_NUMBER);
        return UINT32_MAX; // Signal corruption
    }

    // Validate protocol version
    if (ring_header.version != ZOO_SMB_PROTOCOL_VERSION)
    {
        ZOO_LOG_WARN("Invalid ring message version: %u, expected: %u", 
                          ring_header.version, ZOO_SMB_PROTOCOL_VERSION);
        return UINT32_MAX; // Signal corruption
    }

    size_t payload_size = ring_header.payload_size;
    
    // Check if we have complete message
    size_t total_msg_size = sizeof(ZOO_SMB_MSG_HEADER_STRUCT) + payload_size;
    if (used_bytes < total_msg_size)
    {
        ZOO_LOG_TRACE("Incomplete ring message: need %zu bytes, have %u", 
                          total_msg_size, used_bytes);
        return 0; // Message not complete yet
    }

    ZOO_LOG_TRACE("Ring message available: payload_size=%zu, sequence=%u", 
                      payload_size, ring_header.sequence);

    return payload_size;
}

/**
 * @brief Get the total amount of data available for reading in the ring buffer
 * @param buffer Ring buffer handle
 * @return Number of bytes available for reading, 0 if buffer is empty or invalid
 * @note Includes both message headers and payload data
 *       Thread-safe for status checking
 *       Uses atomic operations for consistent state reading
 */
size_t zoo_smb_ring_buffer_get_available_data_size(ZOO_SMB_RING_BUFFER_HANDLE buffer)
{
    // Validate buffer handle
    if (!buffer || !zoo_smb_ring_buffer_is_valid(buffer))
    {
        return 0;
    }

    // Load positions atomically
    uint32_t write_pos, read_pos;
    load_buffer_positions(buffer, &write_pos, &read_pos);

    uint32_t used_bytes = ring_used(write_pos, read_pos, buffer->data_size);

    ZOO_LOG_TRACE("Ring buffer available data: write_pos=%u, read_pos=%u, used=%u",
                      write_pos, read_pos, used_bytes);

    return used_bytes;
}

/**
 * @brief Check if the ring buffer contains any data available for reading
 * @param buffer Ring buffer handle
 * @return ZOO_TRUE if the buffer contains data, ZOO_FALSE if empty or invalid
 * @note More efficient than checking available_data_size > 0
 *       Thread-safe for status checking
 */
ZOO_BOOL zoo_smb_ring_buffer_has_data(ZOO_SMB_RING_BUFFER_HANDLE buffer)
{
    // Validate buffer handle
    if (!buffer || !zoo_smb_ring_buffer_is_valid(buffer))
    {
        return ZOO_FALSE;
    }

    // Compare positions for data availability
    uint32_t write_pos, read_pos;
    load_buffer_positions(buffer, &write_pos, &read_pos);

    return (write_pos != read_pos);
}

/**
 * @brief Check if the ring buffer is completely empty
 * @param buffer Ring buffer handle
 * @return ZOO_TRUE if the buffer is empty, ZOO_FALSE if it contains data or is invalid
 * @note Thread-safe for status checking
 *       Equivalent to !zoo_smb_ring_buffer_has_data() but more explicit
 */
ZOO_BOOL zoo_smb_ring_buffer_is_empty(ZOO_SMB_RING_BUFFER_HANDLE buffer)
{
    ZOO_SMB_VALIDATE_PTR(buffer, ZOO_FALSE);
    if (!zoo_smb_ring_buffer_is_valid(buffer))
    {
        return ZOO_FALSE; // Invalid buffer is not empty, it's an error condition
    }

    uint32_t write_pos, read_pos;
    load_buffer_positions(buffer, &write_pos, &read_pos);

    ZOO_BOOL is_empty = (write_pos == read_pos);
    
    ZOO_LOG_TRACE("Ring buffer empty check: write_pos=%u, read_pos=%u, is_empty=%s",
                      write_pos, read_pos, is_empty ? "ZOO_TRUE" : "ZOO_FALSE");

    return is_empty;
}

/**
 * @brief Perform comprehensive validation of ring buffer handle and state
 * @param buffer Ring buffer handle to validate
 * @return ZOO_TRUE if the ring buffer is valid and safe to use, ZOO_FALSE otherwise
 * @note Checks magic numbers, version compatibility, initialization state,
 *       corruption flags, position bounds, and data integrity
 *       Safe to call frequently as it uses efficient atomic operations
 */
ZOO_BOOL zoo_smb_ring_buffer_is_valid(ZOO_SMB_RING_BUFFER_HANDLE buffer)
{
    // Basic handle validation
    if (!buffer)
    {
        ZOO_LOG_TRACE("Ring buffer validation failed: NULL handle");
        return ZOO_FALSE;
    }

    // Control structure validation
    if (!buffer->control)
    {
        ZOO_LOG_TRACE("Ring buffer validation failed: NULL control structure");
        return ZOO_FALSE;
    }

    // Data pointer validation
    if (!buffer->data)
    {
        ZOO_LOG_TRACE("Ring buffer validation failed: NULL data pointer");
        return ZOO_FALSE;
    }

    // Magic number validation
    if (buffer->control->magic != ZOO_SMB_RING_BUFFER_MAGIC)
    {
        ZOO_LOG_WARN("Ring buffer validation failed: invalid magic 0x%x, expected 0x%x",
                         buffer->control->magic, ZOO_SMB_RING_BUFFER_MAGIC);
        return ZOO_FALSE;
    }

    // Version compatibility validation
    if (buffer->control->version != ZOO_SMB_RING_BUFFER_VERSION)
    {
        ZOO_LOG_WARN("Ring buffer validation failed: version mismatch %d, expected %d",
                         buffer->control->version, ZOO_SMB_RING_BUFFER_VERSION);
        return ZOO_FALSE;
    }

    // Initialization state validation
    if (!atomic_load_explicit(&buffer->control->initialized, memory_order_acquire))
    {
        ZOO_LOG_TRACE("Ring buffer validation failed: not initialized");
        return ZOO_FALSE;
    }

    // Corruption state validation
    if (atomic_load_explicit(&buffer->control->corrupted, memory_order_acquire))
    {
        ZOO_LOG_WARN("Ring buffer validation failed: buffer is corrupted");
        return ZOO_FALSE;
    }

    // Data size bounds validation
    if (buffer->data_size < ZOO_SMB_RING_BUFFER_MIN_SIZE)
    {
        ZOO_LOG_WARN("Ring buffer validation failed: data size too small %u, min %u",
                         buffer->data_size, ZOO_SMB_RING_BUFFER_MIN_SIZE);
        return ZOO_FALSE;
    }

    if (buffer->data_size > ZOO_SMB_RING_BUFFER_MAX_SIZE)
    {
        ZOO_LOG_WARN("Ring buffer validation failed: data size too large %u, max %u",
                         buffer->data_size, ZOO_SMB_RING_BUFFER_MAX_SIZE);
        return ZOO_FALSE;
    }

    // Position bounds validation
    uint32_t write_pos = atomic_load_explicit(&buffer->control->write_pos, memory_order_acquire);
    uint32_t read_pos = atomic_load_explicit(&buffer->control->read_pos, memory_order_acquire);

    if (write_pos >= buffer->data_size)
    {
        ZOO_LOG_WARN("Ring buffer validation failed: write position out of bounds %u >= %u",
                         write_pos, buffer->data_size);
        return ZOO_FALSE;
    }

    if (read_pos >= buffer->data_size)
    {
        ZOO_LOG_WARN("Ring buffer validation failed: read position out of bounds %u >= %u",
                         read_pos, buffer->data_size);
        return ZOO_FALSE;
    }

    ZOO_LOG_TRACE("Ring buffer validation passed: magic=0x%x, version=%d, size=%u, write_pos=%u, read_pos=%u",
                      buffer->control->magic, buffer->control->version, buffer->data_size, write_pos, read_pos);

    return ZOO_TRUE;
}

/**
 * @brief Clear the ring buffer by resetting all positions and statistics
 * @param buffer Ring buffer handle to clear
 * @return ZOO_SMB_OK on success, appropriate error code on failure
 * @note Atomically resets read/write positions and sequence numbers
 *       Clears all performance statistics
 *       Thread-safe operation with full memory barrier
 * @warning All unread data will be lost after this operation
 */
ZOO_ERROR_TYPE zoo_smb_ring_buffer_clear(ZOO_SMB_RING_BUFFER_HANDLE buffer)
{
    // Validate buffer for operation
    ZOO_ERROR_TYPE result = validate_buffer_for_operation(buffer);
    if (ZOO_SMB_IS_ERROR(result))
    {
        return result;
    }

    // Reset positions and sequences atomically
    atomic_store_explicit(&buffer->control->write_pos, 0, memory_order_release);
    atomic_store_explicit(&buffer->control->read_pos, 0, memory_order_release);
    atomic_store_explicit(&buffer->control->write_seq, 0, memory_order_release);
    atomic_store_explicit(&buffer->control->read_seq, 0, memory_order_release);

    // Reset all statistics efficiently
    const uint64_t zero = 0;
    atomic_store_explicit(&buffer->control->total_writes, zero, memory_order_relaxed);
    atomic_store_explicit(&buffer->control->total_reads, zero, memory_order_relaxed);
    atomic_store_explicit(&buffer->control->write_bytes, zero, memory_order_relaxed);
    atomic_store_explicit(&buffer->control->read_bytes, zero, memory_order_relaxed);
    atomic_store_explicit(&buffer->control->overruns, zero, memory_order_relaxed);
    atomic_store_explicit(&buffer->control->underruns, zero, memory_order_relaxed);

    // Update handle's cached positions
    buffer->last_read_pos = 0;
    buffer->last_read_seq = 0;

    // Apply full memory barrier to ensure all updates are visible
    atomic_thread_fence(memory_order_seq_cst);

    ZOO_LOG_DEBUG("Ring buffer cleared: data_size=%u", buffer->data_size);
    return ZOO_SMB_OK;
}
