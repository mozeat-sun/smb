/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Timer
 * Component id: TIMER
 * File name: zoo_timer.h
 * Description: Timer utility interface for ZOO
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-30     weiwang.sun       created
 ******************************************************************************/

#ifndef ZOO_TIMER_H
#define ZOO_TIMER_H
#include "zoo.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Timer callback function type.
     *
     * @param user_data Pointer to user-defined data.
     */
    typedef void (*ZOO_TIMER_CALLBACK)(void *user_data);

    /**
     * New singleton epoll-based timer manager API
     *
     * - There is exactly one timer manager instance. The manager is created and
     *   started on the first call to `zoo_timer_add`.
     * - Timers are scheduled using Linux `timerfd` and `epoll` (POSIX only).
     * - Each call to `zoo_timer_add` returns a 64-bit timer id that can be used
     *   to cancel the timer via `zoo_timer_remove`.
     */

    /**
     * @brief Add a timer to the singleton timer manager.
     *
     * @param delay_ms    Initial delay in milliseconds before the timer fires.
     * @param interval_ms Interval in milliseconds for periodic timers. Set 0 for one-shot.
     * @param callback    Callback invoked when the timer expires.
     * @param user_data   User data pointer passed back to the callback.
     * @return A non-zero 64-bit timer id on success; 0 on failure or unsupported platform.
     */
    uint64_t zoo_timer_add(uint64_t delay_ms, uint64_t interval_ms, ZOO_TIMER_CALLBACK callback, void *user_data);

    /**
     * @brief Remove a previously added timer.
     *
     * @param timer_id Timer id returned by `zoo_timer_add`.
     * @return ZOO_OK (0) on success or an error code on failure.
     */
    int zoo_timer_remove(uint64_t timer_id);

    /**
     * @brief Shutdown the global timer manager and free resources.
     *
     * This stops the background thread and closes any remaining timers. After
     * calling this, the first call to `zoo_timer_add` will recreate and start
     * the manager again.
     */
    void zoo_timer_shutdown(void);

    /* Backwards-compatible APIs (Deprecated): kept for compatibility with
     * existing code. Prefer the new singleton functions above. */
    typedef struct ZOO_TIMER_STRUCT *ZOO_TIMER_HANDLE;
    ZOO_TIMER_HANDLE zoo_timer_create(
        ZOO_UINT32 interval_ms,
        ZOO_BOOL repeat,
        ZOO_TIMER_CALLBACK callback,
        void *user_data);
    ZOO_BOOL zoo_timer_start(ZOO_TIMER_HANDLE timer);
    void zoo_timer_stop(ZOO_TIMER_HANDLE timer);
    void zoo_timer_destroy(ZOO_TIMER_HANDLE timer);

#ifdef __cplusplus
}
#endif

#endif /* ZOO_TIMER_H */