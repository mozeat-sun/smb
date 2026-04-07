/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: dispatcher
 * Component id: dispatcher
 * File name: zoo_dispatcher.h
 * Description: 高性能消息调度器模块，用于ZOO框架的消息处理
 *              Message dispatcher module for the ZOO framework providing
 *              high-performance message processing, queue management, and
 *              cross-platform message routing capabilities.
 *              
 *              主要功能 (Main Features):
 *              - 消息队列管理 (Message queue management)
 *              - 多线程消息调度 (Multi-threaded message dispatching)
 *              - 消息优先级排序 (Message priority sorting)
 *              - 异步消息处理 (Asynchronous message processing)
 *              - 跨平台支持 (Cross-platform support)
 *              
 * Cross-Platform Support:
 *              - Linux (x86_64, ARM64) - GNU/Linux系统支持
 *              - Windows (x86_64) - Windows操作系统支持  
 *              - macOS (x86_64, ARM64) - Apple macOS系统支持
 *              Uses ZOO platform abstraction layer for portability
 *              使用ZOO平台抽象层确保可移植性
 *              
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-19     weiwang.sun       created
 * 1.1       2025-08-04     AI Assistant      added cross-platform support
 ******************************************************************************/

#ifndef ZOO_DISPATCHER_H
#define ZOO_DISPATCHER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "../../platform/inc/zoo.h"
#include "../../platform/inc/zoo_platform.h"
#include "../../buffer/inc/zoo_queue.h"
#include "../../platform/inc/zoo_error.h"

    /**
     * @brief 消息调度器句柄类型 (Message dispatcher handle type)
     * @details 表示消息调度器实例的不透明句柄，用于所有调度器操作
     *          Opaque handle representing a message dispatcher instance
     *          used for all dispatcher operations
     */
    typedef struct ZOO_DISPATCHER_STRUCT* ZOO_DISPATCHER_HANDLE;

    /**
     * @brief 创建消息调度器实例 (Create a message dispatcher instance)
     * @details 创建一个新的消息调度器，绑定到指定的消息队列
     *          Creates a new message dispatcher bound to the specified message queue
     *          
     * @param queue 消息队列句柄 (Message queue handle) - 必须是有效的已创建队列
     *              Must be a valid, previously created queue handle
     *              
     * @return ZOO_DISPATCHER_HANDLE 成功时返回调度器句柄，失败时返回NULL
     *         Returns dispatcher handle on success, NULL on failure
     *         
     * @note 调度器创建后需要调用zoo_start_dispatcher启动消息处理
     *       After creation, call zoo_start_dispatcher to begin message processing
     *       
     * @see zoo_start_dispatcher, zoo_destroy_dispatcher
     */
    ZOO_DISPATCHER_HANDLE zoo_create_dispatcher(
        ZOO_QUEUE_HANDLE queue);

    /**
     * @brief 设置调度器的排序策略 (Sets the sorting strategy for the dispatcher)
     * @details 配置消息调度器处理消息时使用的排序策略，影响消息处理顺序
     *          Configures the sorting strategy used by the message dispatcher
     *          when processing messages, affecting message processing order
     *          
     * @param dispatcher_handle 调度器句柄 (Dispatcher handle) - 必须是有效的调度器实例
     *                         Must be a valid dispatcher instance
     * @param sort_strategy 排序策略枚举 (Sorting strategy enum):
     *                     - ZOO_QUEUE_SORT_STRATEGY_FIFO: 先进先出 (First In First Out)
     *                     - ZOO_QUEUE_SORT_STRATEGY_PRIORITY: 优先级排序 (Priority based)
     *                     - ZOO_QUEUE_SORT_STRATEGY_TIMESTAMP: 时间戳排序 (Timestamp based)
     *                     - ZOO_QUEUE_SORT_STRATEGY_NONE: 无排序 (No sorting)
     *                     
     * @note 排序策略可以在运行时动态更改，但会影响后续消息的处理顺序
     *       Sorting strategy can be changed dynamically at runtime, but affects
     *       subsequent message processing order
     *       
     * @see ZOO_QUEUE_SORT_STRATEGY_ENUM, zoo_create_dispatcher
     */
    void zoo_dispatcher_set_sort_strategy(
        ZOO_DISPATCHER_HANDLE dispatcher_handle,
        ZOO_QUEUE_SORT_STRATEGY_ENUM sort_strategy);
        
    /**
     * @brief 从消息调度器获取消息队列句柄 (Retrieves the message queue handle from a dispatcher)
     * @details 返回与给定消息调度器关联的消息队列句柄，允许直接访问底层消息队列
     *          Returns the message queue handle associated with the given dispatcher,
     *          allowing direct access to the underlying message queue operations
     *          
     * @param dispatcher_handle 消息调度器句柄 (Message dispatcher handle)
     *                         需要获取队列句柄的调度器实例
     *                         The dispatcher instance from which to retrieve the queue handle
     *                         
     * @return ZOO_QUEUE_HANDLE 与调度器关联的消息队列句柄，如果调度器句柄无效则返回NULL
     *         The message queue handle associated with the dispatcher,
     *         or NULL if the dispatcher handle is invalid
     *         
     * @note 返回的队列句柄不应直接释放，因为它由调度器管理
     *       The returned queue handle should not be freed directly as it is
     *       managed by the dispatcher
     *       
     * @warning 直接操作返回的队列可能会干扰调度器的正常运行
     *          Direct manipulation of the returned queue may interfere with
     *          dispatcher normal operation
     *          
     * @see zoo_create_dispatcher, zoo_destroy_dispatcher
     */
    ZOO_QUEUE_HANDLE zoo_dispatcher_get_queue(
        ZOO_DISPATCHER_HANDLE dispatcher_handle);

    /**
     * @brief 启动消息调度器 (Start the message dispatcher)
     * @details 启动消息调度器的处理线程，开始从队列中处理消息
     *          Starts the dispatcher's processing thread to begin handling messages from the queue
     *          
     * @param dispatcher_handle 调度器句柄 (Dispatcher handle) - 必须是有效的调度器实例
     *                         Must be a valid dispatcher instance
     *                         
     * @note 调度器启动后会在后台线程中持续处理队列消息，直到调用zoo_stop_dispatcher
     *       Once started, the dispatcher will continuously process queue messages in a background
     *       thread until zoo_stop_dispatcher is called
     *       
     * @warning 不要对同一个调度器实例多次调用此函数
     *          Do not call this function multiple times on the same dispatcher instance
     *          
     * @see zoo_stop_dispatcher, zoo_create_dispatcher
     */
    void zoo_start_dispatcher(
        ZOO_DISPATCHER_HANDLE dispatcher_handle);

    /**
     * @brief 停止消息调度器 (Stop the message dispatcher)
     * @details 停止消息调度器的处理线程，不再处理新的队列消息
     *          Stops the dispatcher's processing thread, no longer processing new queue messages
     *          
     * @param dispatcher_handle 调度器句柄 (Dispatcher handle) - 必须是有效的调度器实例
     *                         Must be a valid dispatcher instance
     *                         
     * @note 此函数会等待当前正在处理的消息完成，然后优雅地停止调度器
     *       This function waits for currently processing messages to complete,
     *       then gracefully stops the dispatcher
     *       
     * @note 停止后的调度器可以通过zoo_start_dispatcher重新启动
     *       A stopped dispatcher can be restarted using zoo_start_dispatcher
     *       
     * @see zoo_start_dispatcher, zoo_destroy_dispatcher
     */
    void zoo_stop_dispatcher(
        ZOO_DISPATCHER_HANDLE dispatcher_handle);

    /**
     * @brief 销毁消息调度器实例 (Destroy a message dispatcher instance)
     * @details 完全销毁消息调度器，释放所有相关资源
     *          Completely destroys the message dispatcher, freeing all associated resources
     *          
     * @param dispatcher_handle 调度器句柄 (Dispatcher handle) - 要销毁的调度器实例
     *                         The dispatcher instance to destroy
     *                         
     * @note 调用此函数前应确保调度器已停止运行（调用zoo_stop_dispatcher）
     *       Ensure the dispatcher is stopped (call zoo_stop_dispatcher) before calling this function
     *       
     * @note 销毁后句柄变为无效，不应再使用
     *       After destruction, the handle becomes invalid and should not be used again
     *       
     * @warning 不会自动销毁关联的消息队列，需要单独释放队列资源
     *          Does not automatically destroy the associated message queue,
     *          queue resources must be freed separately
     *          
     * @see zoo_create_dispatcher, zoo_stop_dispatcher
     */
    void zoo_destroy_dispatcher(
        ZOO_DISPATCHER_HANDLE dispatcher_handle);

    /**
     * @brief 队列变化时的调度器回调函数 (Dispatcher callback function for queue changes)
     * @details 当消息队列发生变化时，调度器调用此函数来处理和分发消息给观察者
     *          When the message queue changes, the dispatcher calls this function
     *          to process and dispatch messages to observers
     *          
     * @param queue_handle 消息队列句柄指针 (Pointer to message queue handle)
     *                    发生变化的消息队列
     *                    The message queue that has changed
     *                    
     * @param observer 观察者指针 (Pointer to observer)
     *                将接收分发消息的观察者实例
     *                The observer instance that will receive dispatched messages
     *                
     * @param msg 消息指针 (Pointer to message)
     *           要分发的消息数据
     *           The message data to be dispatched
     *           
     * @param context 上下文指针 (Context pointer)
     *               消息处理的上下文信息
     *               Context information for message processing
     *               
     * @param handler 队列处理器 (Queue handler)
     *               用于处理消息的回调函数
     *               Callback function used to handle the message
     *               
     * @note 此函数通常由队列观察者机制内部调用，不建议直接调用
     *       This function is typically called internally by the queue observer mechanism,
     *       direct calls are not recommended
     *       
     * @see ZOO_QUEUED_HANDLER, zoo_start_dispatcher
     */
    void zoo_dispatcher_on_queue_changed(void* queue_handle, void* observer, void* msg, void * context, ZOO_QUEUED_HANDLER handler);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_DISPATCHER_H */