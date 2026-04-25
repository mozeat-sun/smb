#ifndef ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_H
#define ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_H

#include "zoo_smb_qos.h"
#include "zoo_smb.h"
#include "zoo_list.h"
#include "zoo_smb_subscription_session.h"

/**
 * @brief Opaque handle for the multi-session subscription manager.
 */
typedef struct ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_STRUCT* ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE;

/**
 * @brief Callback used to visit active session entries.
 *
 * @param session Current session in iteration.
 * @param message Message context passed by caller.
 */
typedef void (*ZOO_SMB_SUBSCRIPTION_SESSION_VISITOR)(
    IN const ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session,
    IN const ZOO_SMB_MSG_STRUCT* message);

/**
 * @brief Creates a session manager instance.
 *
 * @return Manager handle on success; NULL on allocation failure.
 */
ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE zoo_smb_subscription_session_manager_create(void);

/**
 * @brief Destroys manager and all managed sessions.
 *
 * @param manager Manager handle. NULL is accepted.
 * @param qos_entity QoS entity used to clean up in-flight request states.
 */
void zoo_smb_subscription_session_manager_destroy(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity);

/**
 * @brief Finds a session by user callback pointer.
 *
 * @param manager Manager handle.
 * @param handler Handler pointer to search.
 * @return Matching session handle or NULL if not found.
 */
ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE zoo_smb_subscription_session_manager_find_by_handler(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_MSG_HANDLER handler);

/**
 * @brief Finds a session by stable API handle ID.
 *
 * @param manager Manager handle.
 * @param id Session ID returned by subscribe API.
 * @return Matching session handle or NULL if not found.
 */
ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE zoo_smb_subscription_session_manager_find_by_id(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN int32_t id);

/**
 * @brief Finds a session by in-flight SUB request ID.
 *
 * @param manager Manager handle.
 * @param request_id Request ID associated with an in-flight subscription message.
 * @return Matching session handle or NULL if not found.
 */
ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE zoo_smb_subscription_session_manager_find_by_request_id(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN uint64_t request_id);

/**
 * @brief Adds a session to manager ownership.
 *
 * @param manager Manager handle.
 * @param session Session to add.
 * @return ZOO_SMB_OK on success, error code otherwise.
 */
ZOO_ERROR_TYPE zoo_smb_subscription_session_manager_add(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session);

/**
 * @brief Removes a session from manager ownership and destroys it.
 *
 * @param manager Manager handle.
 * @param qos_entity QoS entity used to clear request tracking state.
 * @param session Session to remove.
 */
void zoo_smb_subscription_session_manager_remove(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity,
    IN ZOO_SMB_SUBSCRIPTION_SESSION_HANDLE session);

/**
 * @brief Iterates sessions and invokes visitor callback.
 *
 * @param manager Manager handle.
 * @param visitor Visitor callback.
 * @param message Caller-provided message context passed to each callback.
 */
void zoo_smb_subscription_session_manager_visit(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_SUBSCRIPTION_SESSION_VISITOR visitor,
    IN const ZOO_SMB_MSG_STRUCT* message);

/**
 * @brief Applies service availability change to all desired-active sessions.
 *
 * @param manager Manager handle.
 * @param qos_entity QoS entity used for in-flight state cleanup.
 * @param service_available Latest service availability state.
 * @param now_ms Current timestamp in milliseconds.
 * @param node Subscriber node associated with sessions.
 */
void zoo_smb_subscription_session_manager_on_service_change(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity,
    IN ZOO_BOOL service_available,
    IN uint64_t now_ms,
    IN ZOO_SMB_NODE_HANDLE node);

/**
 * @brief Applies SUBACK outcome to the session identified by request ID.
 *
 * @param manager Manager handle.
 * @param request_id SUB request identifier.
 * @param matched ZOO_TRUE when compatibility negotiation succeeded.
 */
void zoo_smb_subscription_session_manager_on_suback(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN uint64_t request_id,
    IN ZOO_BOOL matched);

/**
 * @brief Reconciles session intent to runtime state.
 *
 * @param manager Manager handle.
 * @param node Subscriber node used to send SUB messages.
 * @param qos_entity QoS entity with policy and request-status context.
 * @param service_available Current target service availability.
 * @param now_ms Current timestamp in milliseconds.
 * @param has_pending_work Output flag indicating whether more reconcile work remains.
 * @param sleep_ms Output recommended delay before next reconcile attempt.
 */
void zoo_smb_subscription_session_manager_reconcile(
    IN ZOO_SMB_SUBSCRIPTION_SESSION_MANAGER_HANDLE manager,
    IN ZOO_SMB_NODE_HANDLE node,
    IN ZOO_SMB_QOS_ENTITY_HANDLE qos_entity,
    IN ZOO_BOOL service_available,
    IN uint64_t now_ms,
    OUT ZOO_BOOL* has_pending_work,
    OUT uint32_t* sleep_ms);

#endif
