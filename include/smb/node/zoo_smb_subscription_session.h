#ifndef ZOO_SMB_SUBSCRIPTION_SESSION_H
#define ZOO_SMB_SUBSCRIPTION_SESSION_H

#include "zoo_smb_types.h"
#include "zoo_smb_node.h"

/**
 * @brief Lifecycle state for one subscriber-side subscription session.
 */
typedef enum ZOO_SMB_SUBSCRIPTION_SESSION_STATE_ENUM
{
    ZOO_SMB_SUBSCRIPTION_SESSION_STATE_INIT = 0,       /**< Ready to negotiate subscription. */
    ZOO_SMB_SUBSCRIPTION_SESSION_STATE_PENDING_SERVICE,/**< Waiting for target service to become available. */
    ZOO_SMB_SUBSCRIPTION_SESSION_STATE_WAITING_SUBACK, /**< SUB sent; waiting for SUBACK or timeout. */
    ZOO_SMB_SUBSCRIPTION_SESSION_STATE_ACTIVE,         /**< Subscription negotiated and active. */
    ZOO_SMB_SUBSCRIPTION_SESSION_STATE_DEGRADED,       /**< Temporary degraded state; retry expected. */
    ZOO_SMB_SUBSCRIPTION_SESSION_STATE_FAILED,         /**< Terminal negotiation failure. */
    ZOO_SMB_SUBSCRIPTION_SESSION_STATE_CANCELLED       /**< Session disabled by unsubscribe. */
} ZOO_SMB_SUBSCRIPTION_SESSION_STATE_ENUM;

/**
 * @brief Mutable runtime session object used by subscriber/session-manager logic.
 */
typedef struct ZOO_SMB_SUBSCRIPTION_SESSION_STRUCT
{
    void* usr_data;                                 /**< Callback context passed to handler. */
    uint32_t msg_id;                                /**< Message ID bound to this session. */
    int32_t id;                                     /**< Stable handle returned to caller. */
    ZOO_SMB_MSG_HANDLER handler;                    /**< User callback for matching PUB payloads. */
    uint64_t request_id;                            /**< In-flight SUB request ID, 0 when none. */
    uint64_t next_retry_at_ms;                      /**< Absolute next retry time in milliseconds. */
    uint32_t retry_attempts;                        /**< Number of consecutive send/retry attempts. */
    ZOO_BOOL desired_active;                        /**< Desired state from API intent. */
    ZOO_SMB_SUBSCRIPTION_SESSION_STATE_ENUM state;  /**< Current session lifecycle state. */
} ZOO_SMB_SUBSCRIPTION_SESSION_STRUCT;

typedef ZOO_SMB_SUBSCRIPTION_SESSION_STRUCT* ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE;

/**
 * @brief Creates and initializes a subscription session object.
 *
 * @param msg_id Message ID to subscribe to.
 * @param handler User message callback. Must not be NULL.
 * @param user_data Opaque callback context.
 * @param id Stable session handle value exposed to API callers.
 * @param now_ms Current timestamp in milliseconds for retry scheduling.
 * @param service_available Initial target service availability.
 * @return Session handle on success; NULL on invalid params or allocation failure.
 */
ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE zoo_smb_subscription_session_create(
    IN uint32_t msg_id,
    IN ZOO_SMB_MSG_HANDLER handler,
    IN void* user_data,
    IN int32_t id,
    IN uint64_t now_ms,
    IN ZOO_BOOL service_available);

/**
 * @brief Destroys a session object allocated by create.
 *
 * @param session Session handle. NULL is accepted.
 */
void zoo_smb_subscription_session_destroy(IN ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session);

/**
 * @brief Marks a session as cancelled and clears in-flight request linkage.
 *
 * @param session Session handle. NULL is accepted.
 */
void zoo_smb_subscription_session_mark_cancelled(IN ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session);

/**
 * @brief Checks whether a session matches the specified message ID and handler.
 *
 * @param session Session handle to compare.
 * @param msg_id Message ID to compare.
 * @param handler Handler pointer to compare.
 * @return ZOO_TRUE if both handler and message ID match; otherwise ZOO_FALSE.
 */
ZOO_BOOL zoo_smb_subscription_session_matches_handler(
    IN const ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session,
    IN uint32_t msg_id,
    IN ZOO_SMB_MSG_HANDLER handler);

#endif
